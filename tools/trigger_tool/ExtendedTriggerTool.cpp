#include 'TextTriggerHandler.cpp';
#include 'FogTriggerHandler.cpp';

class ExtendedTriggerTool : Tool
{
	
	entity@ selected_trigger;
	string selected_type;
	array<entity@> hidden_text_triggers;
	
	array<entity@> clipboard;
	
	TriggerToolHandler@ active_trigger_handler;
	array<TriggerToolHandler@> trigger_handlers;
	
	ExtendedTriggerTool(AdvToolScript@ script)
	{
		super(script, 'Triggers');
		
		init_shortcut_key(VK::T);
		
		add_trigger_handler(TextTriggerHandler(script, this));
		add_trigger_handler(FogTriggerHandler(script, this));
	}
	
	void create(ToolGroup@ group) override
	{
		set_icon('editor', 'triggersicon');
	}
	
	void build_sprites(message@ msg) override
	{
		for(uint i = 0; i < trigger_handlers.length; i++)
		{
			trigger_handlers[i].build_sprites(msg);
		}
	}
	
	void on_settings_loaded() override
	{
		for(uint i = 0; i < trigger_handlers.length; i++)
		{
			trigger_handlers[i].on_settings_loaded();
		}
	}
	
	private void add_trigger_handler(TriggerToolHandler@ handler)
	{
		trigger_handlers.insertLast(handler);
	}
	
	// //////////////////////////////////////////////////////////
	// Tool Callbacks
	// //////////////////////////////////////////////////////////
	
	protected void on_editor_loaded_impl() override
	{
		if(@active_trigger_handler != null)
		{
			active_trigger_handler.editor_loaded();
		}
	}
	
	protected void on_editor_unloaded_impl() override
	{
		if(@active_trigger_handler != null)
		{
			active_trigger_handler.editor_unloaded();
		}
		
		update_selected_trigger(null);
	}
	
	protected void on_deselect_impl() override
	{
		update_selected_trigger(null);
	}
	
	protected void step_impl() override
	{
		update_selected_trigger(script.editor.selected_trigger);
		
		if(!script.ui.is_mouse_active && script.scene_focus)
		{
			if(@selected_trigger != null)
			{
				if(script.ctrl.down && script.input.key_check_pressed_vk(VK::C))
				{
					copy_trigger(selected_trigger, script.alt.down);
				}
				if(script.consume_gvb(GVB::Delete))
				{
					script.g.remove_entity(selected_trigger);
					update_selected_trigger(null);
				}
			}
			if(script.ctrl.down && script.input.key_check_pressed_vk(VK::V))
			{
				entity@ trigger = paste_triggers(script.mouse.x, script.mouse.y);
				if(@trigger != null)
				{
					@script.editor.selected_trigger = trigger;
				}
			}
		}
		
		check_hidden_triggers();
		
		if(@active_trigger_handler != null)
		{
			active_trigger_handler.step();
		}
	}
	
	protected void draw_impl(const float sub_frame)
	{
		const uint hidden_clr =0x886666ff;
		for(uint i = 0; i < hidden_text_triggers.length; i++)
		{
			entity@ e = hidden_text_triggers[i];
			float x, y;
			script.transform(e.x(), e.y(), e.layer(), 5, 22, 22, x, y);
			script.g.draw_rectangle_world(22, 22, x - 10, y - 10, x + 10, y + 10, 0, hidden_clr);
			script.circle(
				22, 22,
				x, y, e.vars().get_var('width').get_int32(), 48,
				4, hidden_clr);
		}
		
		if(@active_trigger_handler != null)
		{
			active_trigger_handler.draw(sub_frame);
		}
	}
	
	// //////////////////////////////////////////////////////////
	// Methods
	// //////////////////////////////////////////////////////////
	
	void add_hidden_trigger(entity@ trigger)
	{
		if(@trigger == null)
			return;
		
		for(uint i = 0; i < hidden_text_triggers.length; i++)
		{
			if(hidden_text_triggers[i].is_same(trigger))
				return;
		}
		
		hidden_text_triggers.insertLast(trigger);
	}
	
	private void check_hidden_triggers()
	{
		if(hidden_text_triggers.length > 0)
		{
			hidden_text_triggers.resize(0);
		}
		
		if(@selected_trigger != null && selected_type == TextTriggerType::Normal)
		{
			if(selected_trigger.vars().get_var('hide').get_bool())
			{
				hidden_text_triggers.insertLast(selected_trigger);
			}
		}
		
		if(!script.ctrl.down || !script.alt.down)
			return;
		
		int i = script.g.get_entity_collision(
			script.view_y1, script.view_y2,
			script.view_x1, script.view_x2,
			ColType::Trigger);
		
		while(i-- > 0)
		{
			entity@ e = script.g.get_entity_collision_index(i);
			
			const string name = e.type_name();
			if(name != 'text_trigger')
				continue;
			
			if(e.vars().get_var('hide').get_bool())
			{
				hidden_text_triggers.insertLast(e);
			}
		}
	}
	
	private void copy_trigger(entity@ trigger, const bool append_clipbaord=false)
	{
		if(@trigger == null)
			return;
		
		if(!append_clipbaord)
		{
			clipboard.resize(0);
		}
		
		entity@ new_trigger = create_entity(selected_trigger.type_name());
		copy_vars(selected_trigger, new_trigger);
		
		clipboard.insertLast(new_trigger);
		
		if(clipboard.length > 1)
		{
			script.show_info_popup(
				clipboard.length + ' trigger' + (clipboard.length > 1 ? 's' : '') + ' in clipboard',
				null, PopupPosition::Below, 2);
		}
	}
	
	private entity@ paste_triggers(const float x, const float y)
	{
		if(clipboard.length == 0)
			return null;
		
		entity@ copy = null;
		
		// Space pasted triggers out in a square grid.
		const float spacing = 30;
		const int column_count = int(ceil(sqrt(clipboard.length)));
		const int row_count = int(ceil(float(clipboard.length) / column_count));
		const float ox = -(column_count - 1) * spacing * 0.5;
		const float oy = -(row_count - 1) * spacing * 0.5;
		int column = 0;
		int row = 0;
		
		for(uint i = 0; i < clipboard.length; i++)
		{
			entity@ item = clipboard[i];
			@copy = create_entity(item.type_name());
			copy.set_xy(x + ox + column * spacing, y + oy + row * spacing);
			copy_vars(item, copy);
			script.g.add_entity(copy);
			
			if(++column == column_count)
			{
				column = 0;
				row++;
			}
		}
		
		return copy;
	}
	
	private void update_selected_trigger(entity@ trigger)
	{
		if(@trigger == @selected_trigger)
			return;
		
		if(@trigger != null ? trigger.is_same(selected_trigger) : selected_trigger.is_same(trigger))
			return;
		
		@selected_trigger = trigger;
		selected_type = @selected_trigger != null ? selected_trigger.type_name() : '';
		
		TriggerToolHandler@ new_handler = null;
		
		for(uint i = 0; i < trigger_handlers.length; i++)
		{
			TriggerToolHandler@ handler = trigger_handlers[i];
			if(handler.should_handle(selected_trigger, selected_type))
			{
				@new_handler = handler;
				break;
			}
		}
		
		if(!update_trigger_handler(new_handler) && @active_trigger_handler != null)
		{
			active_trigger_handler.select(selected_trigger, selected_type);
		}
	}
	
	// Returns true if the new handler's select method was called.
	private bool update_trigger_handler(TriggerToolHandler@ new_handler)
	{
		if(@new_handler == @active_trigger_handler)
			return false;
		
		if(@active_trigger_handler != null)
		{
			active_trigger_handler.deselect();
		}
		
		@active_trigger_handler = new_handler;
		
		if(@active_trigger_handler != null)
		{
			active_trigger_handler.select(selected_trigger, selected_type);
			return true;
		}
		
		return @active_trigger_handler == null;
	}
	
}
