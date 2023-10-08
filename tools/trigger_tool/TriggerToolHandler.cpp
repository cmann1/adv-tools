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
	
	protected bool is_editing;
	/* All UI events should return if this is true to avoid script changes triggering updates.
	 * Can also be set by sub classes before/after change UI properties. */
	protected bool ignore_edit_ui_events;
	
	protected PopupOptions@ selected_popup;
	protected Container@ selected_popup_dummy_overlay;
	
	protected Button@ edit_button;
	protected Window@ edit_window;
	
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
	// Selection
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
		
		if(@new_trigger != null)
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
					// This is the only selected trigger.
					if(!can_deslect)
					{
						return 0;
					}
					
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
	
	/**
	 * @brief Some standard logic for editing multiple triggers.
	 */
	protected void do_selection_change_for_editing(
		const bool show_popup,
		const bool primary, const bool added, const bool removed)
	{
		if(is_editing && select_list.length == 0)
		{
			stop_editing(true);
		}
		
		if(primary && !added && is_editing)
		{
			do_update_edit_properties();
			return;
		}
		
		if(!primary && select_list.length != 0)
		{
			if(is_editing)
			{
				do_update_edit_properties();
				if(added)
				{
					store_all_triggers_data(true);
				}
			}
			return;
		}
		
		if(@selected_trigger != null)
		{
			if(@selected_popup == null)
			{
				create_selected_popup();
			}
			
			if(show_popup)
			{
				show_selected_popup();
			}
			
			if(is_editing)
			{
				stop_editing(true, false);
				start_editing();
			}
		}
		else
		{
			if(show_popup)
			{
				show_selected_popup(false);
			}
			
			stop_editing(true);
		}
	}
	
	/**
	 * @brief Stores trigger data before editing so changes can be reverted.
	 * @param only_latest If true only the last selected trigger's data will be stored.
	 *        Useful if e.g. store_all_triggers_data() has already been called.
	 */
	protected void store_all_triggers_data(const bool only_latest=false)
	{
		if(select_list.length == 0)
			return;
		
		if(only_latest)
		{
			select_list[select_list.length - 1].store_all();
		}
		else
		{
			for(uint i = 0; i < select_list.length; i++)
			{
				select_list[i].store_all();
			}
		}
	}
	
	/**
	 * @brief Restores all selected triggers data. `store_all_triggers_data` Must have been called at least once before this.
	 */
	protected void restore_all_triggers_data()
	{
		for(uint i = 0; i < select_list.length; i++)
		{
			select_list[i].restore_all();
		}
	}
	
	/**
	 * @brief 
	 * @return True if a trigger was either added to or removed from the selection.
	 */
	protected bool check_mouse_multi_select()
	{
		if(!has_selection || script.alt.down || !script.shift.down || !script.mouse.left_press)
			return false;
		
		script.input.key_clear_gvb(GVB::LeftClick);
		
		entity@ new_trigger = pick_trigger();
		if(@new_trigger != null)
		{
			select_trigger(new_trigger);
			return true;
		}
		
		return false;
	}
	
	// //////////////////////////////////////////////////////////
	// Selection Popup
	// //////////////////////////////////////////////////////////
	
	private void create_selected_popup()
	{
		if(@selected_popup != null)
			return;
		
		Element@ content = create_selected_popup_content();
		
		if(@content == null)
			return;
		
		@selected_popup_dummy_overlay = Container(script.ui);
		//selected_popup_dummy_overlay.background_colour = 0x55ff0000;
		selected_popup_dummy_overlay.mouse_self = false;
		selected_popup_dummy_overlay.is_snap_target = false;
		
		@selected_popup = PopupOptions(script.ui, content, true, PopupPosition::Above, PopupTriggerType::Manual, PopupHideType::Manual);
		selected_popup.as_overlay = false;
		selected_popup.spacing = 0;
		selected_popup.padding = 0;
		selected_popup.background_colour = 0;
		selected_popup.border_colour = 0;
		selected_popup.shadow_colour = 0;
		selected_popup.background_blur = false;
	}
	
	/**
	 * @brief Override to create the content for the selected popup.
	 */
	protected Element@ create_selected_popup_content()
	{
		puts('Warning: create_selected_popup_content not implemented.');
		return null;
	}
	
	/**
	 * @brief Call to show or hide the selection popup. `create_selected_popup` must have been called at least once
	 *        before calling this.
	 * @param show
	 */
	protected void show_selected_popup(const bool show=true)
	{
		if(show)
		{
			script.ui.add_child(selected_popup_dummy_overlay);
			script.ui.move_to_back(selected_popup_dummy_overlay);
			script.ui.show_tooltip(selected_popup, selected_popup_dummy_overlay);
			update_selected_popup_position();
		}
		else
		{
			script.ui.hide_tooltip(selected_popup, script.in_editor);
			script.ui.remove_child(selected_popup_dummy_overlay);
		}
	}
	
	/**
	 * @brief Call at the end of every `step` if this handler displays the selection popup.
	 * @param required_state If set will automatically show and hide the popup according to the current state.
	 */
	protected void update_selected_popup_position(const int required_state=TriggerHandlerState::Undefined)
	{
		if(@selected_popup == null)
			return;
		
		if(required_state != TriggerHandlerState::Undefined)
		{
			if(!selected_popup.popup_visible && state == TriggerHandlerState::Idle)
			{
				script.ui.show_tooltip(selected_popup, selected_popup_dummy_overlay);
			}
			else if(selected_popup.popup_visible && state != TriggerHandlerState::Idle)
			{
				script.ui.hide_tooltip(selected_popup);
			}
		}
		
		if(@selected_popup == null || select_list.length == 0 || !selected_popup.popup_visible)
		{
			selected_popup.interactable = false;
			return;
		}
		
		float x = 0, y = 0;
		
		for(uint i = 0; i < select_list.length; i++)
		{
			TriggerHandlerData@ data = select_list[i];
			x += data.trigger.x();
			y += data.trigger.y();
		}
		
		x /= select_list.length;
		y /= select_list.length;
		
		float x1, y1, x2, y2;
		const float size = select_list.length == 1 ? 10 : 0;
		script.world_to_hud(x - size, y - size, x1, y1);
		script.world_to_hud(x + size, y + size, x2, y2);
		
		if(select_list.length == 1)
		{
			y1 -= script.ui.style.spacing;
		}
		else
		{
			y1 += selected_popup.popup._height * 0.5;
		}
		
		selected_popup_dummy_overlay.x = x1;
		selected_popup_dummy_overlay.y = y1;
		selected_popup_dummy_overlay.width = x2 - x1;
		selected_popup_dummy_overlay.height = y2 - y1;
		selected_popup_dummy_overlay.visible = true;
		selected_popup_dummy_overlay.force_calculate_bounds();
		
		selected_popup.interactable = !script.space.down;
	}
	
	// //////////////////////////////////////////////////////////
	// Editing
	// //////////////////////////////////////////////////////////
	
	/**
	 * @brief Checks the Enter and Escape to start and stop editing.
	 * @return 0 if nothing happened, 1, if editing was started, and -1 if editing was cancelled.
	 */
	protected int check_edit_keys()
	{
		// Start editing with Enter.
		if(!is_editing && @selected_trigger != null && script.scene_focus && script.consume_pressed_gvb(GVB::Return))
		{
			start_editing();
			return 1;
		}
		
		// Accept/Cancel with Enter/Escape when the textbox isn't focused.
		if(is_editing && script.scene_focus && !sub_ui_active())
		{
			if(script.escape_press)
			{
				stop_editing(false);
				return -1;
			}
			else if(script.ctrl.down && script.return_press)
			{
				stop_editing(true);
				return -1;
			}
		}
		
		return 0;
	}
	
	/**
	 * @brief Override to determine of a sub component is active, e.g. a colour picker or layer button.
	 */
	protected bool sub_ui_active()
	{
		return false;
	}
	
	protected void start_editing()
	{
		const bool is_window_created = @edit_window == null;
		if(is_window_created)
		{
			create_edit_window();
			script.ui.add_child(edit_window);
			script.window_manager.register_element(edit_window);
		}
		
		store_all_triggers_data();
		
		is_editing = true;
		do_update_edit_properties();
		
		if(is_window_created)
		{
			edit_window.fit_to_contents(true);
			edit_window.centre();
			script.window_manager.force_immediate_reposition(edit_window);
		}
		
		if(@edit_button != null)
		{
			edit_button.selected = true;
		}
		
		edit_window.show();
		edit_window.parent.move_to_front(edit_window);
	}
	
	protected void stop_editing(const bool accept, const bool update_ui=true)
	{
		if(!is_editing)
			return;
		
		if(!accept)
		{
			restore_all_triggers_data();
		}
		
		is_editing = false;
		
		if(update_ui)
		{
			if(@edit_button != null)
			{
				edit_button.selected = false;
			}
			
			if(@edit_window != null)
			{
				edit_window.hide(script.in_editor);
			}
		}
	}
	
	/**
	 * @brief Override to create the edit window.
	 */
	protected void create_edit_window()
	{
		puts('Warning: create_edit_window not implemented.');
	}
	
	protected void do_update_edit_properties() final
	{
		ignore_edit_ui_events = true;
		update_edit_properties();
		ignore_edit_ui_events = false;
	}
	
	/**
	 * @brief Override to update the edit controls.
	 */
	protected void update_edit_properties()
	{
		
	}
	
	// //////////////////////////////////////////////////////////
	// Utility
	// //////////////////////////////////////////////////////////
	
	protected bool has_selection { get const { return select_list.length > 0; } }
	
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
			if(!should_handle(e, type))
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
	
	/**
	 * @brief Draw lines from all the selected triggers to the popup.
	 */
	protected void draw_selected_popup_connections()
	{
		if(@selected_popup != null && selected_popup.popup_visible)
		{
			draw_selected_ui_connections(selected_popup.popup);
		}
	}
	
	/**
	 * @brief Draw lines from all the selected triggers to the given UI element.
	 */
	protected void draw_selected_ui_connections(Element@ element)
	{
		if(@element == null)
			return;
		
		for(uint i = 0; i < select_list.length; i++)
		{
			TriggerHandlerData@ data = select_list[i];
			draw_line_to_ui(data.trigger.x(), data.trigger.y(), element);
		}
	}
	
	protected void draw_line_to_ui(const float x, const float y, Element@ element)
	{
		if(@element == null || !element.visible)
			return;
		
		float x1, y1, x2, y2;
		script.world_to_hud(x, y, x1, y1, false);
		
		if(x1 < element.x1 || x1 > element.x2 || y1 < element.y1 || y1 > element.y2)
		{
			const float line_width = 4;
			const uint colour = multiply_alpha(script.ui.style.normal_bg_clr, 0.5);
			
			closest_point_to_rect(x1, y1,
				element.x1 + line_width, element.y1 + line_width,
				element.x2 - line_width, element.y2 - line_width,
				x2, y2);
			
			script.ui.style.draw_line(x1, y1, x2, y2, line_width, colour);
		}
	}
	
	protected void debug_select_list()
	{
		string output = 'Selected Triggers:';
		
		for(uint i = 0; i < select_list.length; i++)
		{
			TriggerHandlerData@ data = select_list[i];
			
			if(output != '')
			{
				output += '\n';
			}
			
			output += '   ' + i + ': ' + data.trigger_type + ' [' + data.trigger.id() + ']';
		}
		
		script.debug.print(output, 'TriggerToolHandlerSelectList');
	}
	
}
