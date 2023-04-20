#include 'FogTriggerHandlerData.cpp';

const string TRIGGERS_SPRITES_BASE = SPRITES_BASE + 'triggers/';
const string EMBED_spr_fog_adjust = TRIGGERS_SPRITES_BASE + 'fog_adjust.png';

class FogTriggerHandler : TriggerToolHandler
{
	
	private Toolbar@ toolbar;
	private Button@ adjust_button;
	
	private bool is_adjusting;
	
	FogTriggerHandler(AdvToolScript@ script, ExtendedTriggerTool@ tool)
	{
		super(script, tool);
	}
	
	void build_sprites(message@ msg) override
	{
		build_sprite(msg, 'fog_adjust');
	}
	
	bool should_handle(entity@ trigger, const string &in type) override
	{
		return type == 'fog_trigger';
	}
	
	protected void select_impl(entity@ trigger, const string &in type) override
	{
		select_trigger(trigger, true);
	}
	
	void step() override
	{
		check_selected_triggers();
		check_keys();
		check_mouse();
		
		update_selected_popup_position(TriggerHandlerState::Idle);
	}
	
	void draw(const float sub_frame) override
	{
		draw_selected_popup_connections();
	}
	
	// //////////////////////////////////////////////////////////
	// Methods
	// //////////////////////////////////////////////////////////
	
	// TODO: Implement
	private void check_keys()
	{
		// TODO: Toggle
		// TODO: Might also have to check for mouse dragging sliders etc.
		// Toggle editing with Enter and Escape.
		//if(@selected_trigger != null && script.scene_focus && script.consume_pressed_gvb(GVB::Return))
		//{
		//	start_adjusting();
		//}
		//
		//// Accept/Cancel with Enter/Escape.
		//if(script.scene_focus && !sub_ui_active())
		//{
		//	if(script.escape_press)
		//	{
		//		stop_adjusting(false);
		//	}
		//	else if(script.ctrl.down && script.return_press)
		//	{
		//		stop_adjusting(true);
		//	}
		//}
	}
	
	private void check_mouse()
	{
		bool changed = false;
		
		if(state == TriggerHandlerState::Idle)
		{
			// Multi select
			if(has_selection && script.shift.down && script.mouse.left_press)
			{
				script.input.key_clear_gvb(GVB::LeftClick);
				
				entity@ new_trigger = pick_trigger();
				if(@new_trigger != null)
				{
					select_trigger(new_trigger);
				}
			}
		}
	}
	
	//
	
	private FogTriggerHandlerData@ selected(const int index)
	{
		return cast<FogTriggerHandlerData@>(select_list[index >= 0 ? index : select_list.length - 1]);
	}
	
	protected TriggerHandlerData@ create_handler_data(entity@ trigger) override
	{
		return FogTriggerHandlerData(trigger);
	}
	
	protected void on_selection_changed(const bool primary, const bool added, const bool removed) override
	{
		if(is_adjusting && select_list.length == 0)
		{
			stop_adjusting(true);
		}
		
		if(primary && !added && is_adjusting)
		{
			update_properties();
			return;
		}
		
		if(!primary && select_list.length != 0)
		{
			if(is_adjusting)
			{
				update_properties();
			}
			return;
		}
		
		if(@selected_trigger != null)
		{
			if(@selected_popup == null)
			{
				create_toolbar();
			}
			
			show_selected_popup();
			
			if(is_adjusting)
			{
				stop_adjusting(true, false);
				start_adjusting();
			}
		}
		else
		{
			show_selected_popup(false);
			stop_adjusting(true);
		}
	}
	
	private void start_adjusting()
	{
		
	}
	
	private void stop_adjusting(const bool accept, const bool update_ui=true)
	{
		if(!is_adjusting)
			return;
		
		is_adjusting = true;
		
		if(!accept)
		{
			restore_all_triggers_data();
		}
		
		//if(update_ui)
		//{
		//	edit_button.selected = false;
		//	if(@window != null)
		//	{
		//		window.hide(script.in_editor);
		//	}
		//}
	}
	
	// //////////////////////////////////////////////////////////
	// UI
	// //////////////////////////////////////////////////////////
	
	private void create_toolbar()
	{
		UI@ ui = script.ui;
		Style@ style = ui.style;
		
		@toolbar = Toolbar(ui, false, true);
		toolbar.name = 'TriggerToolFogToolbar';
		
		@adjust_button = toolbar.create_button(SPRITE_SET, 'fog_adjust', Settings::IconSize, Settings::IconSize);
		adjust_button.selectable = true;
		adjust_button.mouse_click.on(EventCallback(on_toolbar_button_click));
		
		toolbar.fit_to_contents(true);
		
		create_selected_popup(toolbar);
	}
	
	private void update_properties()
	{
		if(!is_adjusting)
			return;
		
		
	}
	
	// //////////////////////////////////////////////////////////
	// Events
	// //////////////////////////////////////////////////////////
	
	private void on_toolbar_button_click(EventInfo@ event)
	{
		const string name = event.target.name;
		
		if(name == 'adjust')
		{
			if(adjust_button.selected)
			{
				start_adjusting();
			}
			else
			{
				stop_adjusting(true);
			}
		}
	}
	
}
