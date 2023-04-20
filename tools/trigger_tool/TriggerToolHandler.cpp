#include 'TriggerHandlerData.cpp';
#include 'TriggerHandlerTypes.cpp';

class TriggerToolHandler
{
	
	protected AdvToolScript@ script;
	protected ExtendedTriggerTool@ tool;
	
	protected int state = TriggerHandlerState::Idle;
	
	/* The primary selected trigger. */
	protected entity@ selected_trigger;
	/* The primary selected trigger type. */
	protected string selected_type = '';
	/* The list of all selected triggers. */
	protected array<TriggerHandlerData@> select_list;
	
	TriggerToolHandler(AdvToolScript@ script, ExtendedTriggerTool@ tool)
	{
		@this.script = script;
		@this.tool = tool;
	}
	
	void build_sprites(message@ msg) { }
	
	bool should_handle(entity@ trigger, const string &in type)
	{
		return false;
	}
	
	void editor_loaded() {}
	
	void editor_unloaded() {}
	
	void select(entity@ trigger, const string &in type) final
	{
		select_impl(trigger, type);
	}
	void select_impl(entity@ trigger, const string &in type) { }
	
	void deselect() final
	{
		state = TriggerHandlerState::Idle;
		select_trigger(null);
		
		deselect_impl();
	}
	void deselect_impl() { }
	
	void step() {}
	
	void draw(const float sub_frame) { }
	
	// //////////////////////////////////////////////////////////
	// Methods
	// //////////////////////////////////////////////////////////
	
	/**
	 * @brief Must be overriden to return the correct TriggerHandlerData subclass.
	 * @param trigger
	 * @return 
	 */
	protected TriggerHandlerData@ create_handler_data(entity@ trigger)
	{
		return TriggerHandlerData(trigger);
	}
	
	/**
	 * @param new_trigger Set to null to deselect all.
	 * @param primary Set when the built in selected trigger has changed.
	 * @return 
	 */
	protected int select_trigger(entity@ new_trigger, bool primary=false)
	{
		if(@new_trigger == null)
		{
			primary = true;
		}
		
		// Don't allow deselecting if this is the only selected trigger.
		const bool can_deslect = select_list.length > 1;
		if(@new_trigger != null && (primary || can_deslect))
		{
			for(uint i = 0; i < select_list.length; i++)
			{
				TriggerHandlerData@ data = select_list[i];
				
				// The built in selected trigger has changed.
				// If it is already part of the selection, make it the new primary selected trigger.
				// Other wise start a new selection.
				if(primary)
				{
					if(!data.trigger.is_same(new_trigger))
						continue;
					
					if(i > 0)
					{
						select_list.removeAt(i);
						select_list.insertAt(0, data);
						reset_primary_selected_trigger(false);
					}
					
					return 0;
				}
				// Or remove from selection.
				else if(new_trigger.is_same(data.trigger))
				{
					select_list.removeAt(i);
					
					// The primary trigger was deselected.
					if(i == 0)
					{
						reset_primary_selected_trigger(true);
					}
					else
					{
						on_selection_changed(false, false, true);
					}
					
					return -1;
				}
			}
		}
		
		if(primary)
		{
			select_list.resize(0);
			@selected_trigger = new_trigger;
			selected_type = @selected_trigger != null ? selected_trigger.type_name() : '';
		}
		
		if(@new_trigger != null)
		{
			TriggerHandlerData@ data = create_handler_data(new_trigger);
			select_list.insertLast(data);
		}
		
		on_selection_changed(primary, @new_trigger != null, false);
		
		return 1;
	}
	
	/**
	 * @brief Called after the selection changes. Makes the first selected trigger the primary.
	 * @param removed
	 */
	protected void reset_primary_selected_trigger(const bool removed)
	{
		if(select_list.length == 0)
		{
			@selected_trigger = null;
			selected_type = '';
			on_selection_changed(false, false, true);
			return;
		}
		
		TriggerHandlerData@ data = select_list[0];
		@selected_trigger = data.trigger;
		selected_type = data.trigger_type;
		
		on_selection_changed(true, false, removed);
		
		@script.editor.selected_trigger = selected_trigger;
	}
	
	/**
	 * @brief Removes/unselects triggers that have been deleted. Call during every `step`.
	 */
	protected void check_selected_triggers()
	{
		if(select_list.length == 0)
			return;
		
		bool changed = false;
		bool changed_primary = false;
		
		for(int i = int(select_list.length) - 1; i >= 0; i--)
		{
			TriggerHandlerData@ data = @select_list[i];
			if(data.trigger.destroyed())
			{
				select_list.removeAt(i);
				
				if(i == 0)
				{
					changed_primary = true;
				}
				
				changed = true;
			}
		}
		
		
		if(changed)
		{
			if(changed_primary)
			{
				reset_primary_selected_trigger(true);
			}
			else
			{
				on_selection_changed(false, false, true);
			}
		}
	}
	
	/**
	 * @brief Override to be notified when the selection changes.
	 * @param primary Has the primary selected trigger changed.
	 * @param added Has a trigger been added to the selection.
	 * @param removed Has a trigger been removed from the selection.
	 */
	protected void on_selection_changed(const bool primary, const bool added, const bool removed) { }
	
	// //////////////////////////////////////////////////////////
	// Utility
	// //////////////////////////////////////////////////////////
	
	protected entity@ pick_trigger()
	{
		if(script.ui.is_mouse_over_ui || script.mouse_in_gui)
			return null;
		
		entity@ closest = null;
		float closest_dist = MAX_FLOAT;
		
		int i = script.g.get_entity_collision(
			script.mouse.y, script.mouse.y,
			script.mouse.x, script.mouse.x,
			ColType::Trigger);
		
		while(i-- > 0)
		{
			entity@ e = script.g.get_entity_collision_index(i);
			
			const string type = e.type_name();
			if(should_handle(e, type))
				continue;
			
			const float dist = dist_sqr(e.x(), e.y(), script.mouse.x, script.mouse.y);
			
			if(dist < closest_dist)
			{
				closest_dist = dist;
				@closest = e;
			}
		}
		
		return closest;
	}
	
	protected void draw_line_to_ui(const float x, const float y, Element@ element)
	{
		if(@element == null || !element.visible)
			return;
		
		float x1, y1, x2, y2;
		script.world_to_hud(x, y, x1, y1, false);
		
		if(x1 < element.x1 || x1 > element.x2 || y1 < element.y1 || y1 > element.y2)
		{
			const float line_width = 2;
			const uint colour = multiply_alpha(script.ui.style.normal_bg_clr, 0.5);
			
			closest_point_to_rect(x1, y1,
				element.x1 + line_width, element.y1 + line_width,
				element.x2 - line_width, element.y2 - line_width,
				x2, y2);
			
			script.ui.style.draw_line(x1, y1, x2, y2, line_width, colour);
		}
	}
	
}
