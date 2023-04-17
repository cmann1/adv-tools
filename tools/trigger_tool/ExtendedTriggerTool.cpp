#include 'TextTriggerTool.cpp';

class ExtendedTriggerTool : Tool
{
	
	entity@ selected_trigger;
	string selected_type;
	
	array<entity@> clipboard;
	
	ExtendedTriggerTool(AdvToolScript@ script)
	{
		super(script, 'Triggers');
		
		init_shortcut_key(VK::T);
	}
	
	void create(ToolGroup@ group) override
	{
		set_icon('editor', 'triggersicon');
	}
	
	// //////////////////////////////////////////////////////////
	// Tool Callbacks
	// //////////////////////////////////////////////////////////
	
	protected void step_impl() override
	{
		update_selected_trigger(script.editor.selected_trigger);
		
		if(!script.ui.is_mouse_active && @script.ui.focus == null && !script.input.is_polling_keyboard())
		{
			if(script.ctrl.down && script.input.key_check_pressed_vk(VK::C))
			{
				copy_trigger(selected_trigger, script.alt.down);
			}
			if(script.ctrl.down && script.input.key_check_pressed_vk(VK::V))
			{
				entity@ trigger = paste_triggers(script.mouse.x, script.mouse.y);
				if(@trigger != null)
				{
					@script.editor.selected_trigger = trigger;
				}
			}
			if(script.ctrl.down && script.input.key_check_pressed_vk(VK::H))
			{
				if(selected_type == 'text_trigger')
				{
					varvalue@ hide_var = selected_trigger.vars().get_var('hide');
					hide_var.set_bool(!hide_var.get_bool());
				}
			}
		}
	}
	
	// //////////////////////////////////////////////////////////
	// Tool Callbacks
	// //////////////////////////////////////////////////////////
	
	protected void on_deselect_impl() override
	{
		update_selected_trigger(null);
	}
	
	protected void on_editor_unloaded_impl() override
	{
		update_selected_trigger(null);
	}
	
	// //////////////////////////////////////////////////////////
	// Methods
	// //////////////////////////////////////////////////////////
	
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
	}
	
}
