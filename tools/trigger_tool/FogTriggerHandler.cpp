#include 'FogTriggerHandlerData.cpp';

const string TRIGGERS_SPRITES_BASE = SPRITES_BASE + 'triggers/';
const string EMBED_spr_fog_adjust = TRIGGERS_SPRITES_BASE + 'fog_adjust.png';

class FogTriggerHandler : TriggerToolHandler
{
	
	private Toolbar@ toolbar;
	
	private Window@ window;
	private ColourSwatch@ override_colour_swatch;
	private Checkbox@ override_colour_checkbox;
	
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
	
	
	private void check_keys()
	{
		check_edit_keys();
	}
	
	private void check_mouse()
	{
		if(state == TriggerHandlerState::Idle)
		{
			check_mouse_multi_select();
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
		do_selection_change_for_editing(true, primary, added, removed);
	}
	
	// //////////////////////////////////////////////////////////
	// UI
	// //////////////////////////////////////////////////////////
	
	protected Element@ create_selected_popup_content() override
	{
		UI@ ui = script.ui;
		Style@ style = ui.style;
		
		@toolbar = Toolbar(ui, false, true);
		toolbar.name = 'TriggerToolFogToolbar';
		
		@edit_button = script.create_toolbar_button(toolbar, 'adjust', 'fog_adjust', 'Adjust Colours');
		edit_button.selectable = true;
		edit_button.mouse_click.on(EventCallback(on_toolbar_button_click));
		
		toolbar.fit_to_contents(true);
		
		return toolbar;
	}
	
	protected void create_edit_window() override
	{
		UI@ ui = script.ui;
		Style@ style = ui.style;
		
		@edit_window = Window(ui, 'Adjust Fog');
		edit_window.resizable = true;
		edit_window.name = 'FogToolTextProperties';
		edit_window.set_icon(SPRITE_SET, 'fog_adjust', Settings::IconSize, Settings::IconSize);
		edit_window.x = 200;
		edit_window.y = 20;
		edit_window.min_width = 450;
		edit_window.min_height = 350;
		edit_window.width  = edit_window.min_width;
		edit_window.height = edit_window.min_height;
		@edit_window.layout = AnchorLayout(ui).set_padding(0);
		edit_window.close.on(EventCallback(on_cancel_click));
		
		// Override colour swatch
		
		@override_colour_swatch = ColourSwatch(ui);
		@override_colour_swatch.tooltip = PopupOptions(ui, 'Override colour');
		override_colour_swatch.change.on(EventCallback(on_override_colour_change));
		override_colour_swatch.activate.on(EventCallback(on_override_colour_change));
		edit_window.add_child(override_colour_swatch);
		
		@override_colour_checkbox = Checkbox(ui);
		override_colour_checkbox.checked = true;
		override_colour_checkbox.anchor_left.after(override_colour_swatch);
		override_colour_checkbox.layout_align_middle(override_colour_swatch);
		override_colour_checkbox.change.on(EventCallback(on_override_colour_checked_change));
		edit_window.add_child(override_colour_checkbox);
		
		Label@ override_label = script.create_label('Override colour', edit_window);
		override_label.anchor_left.sibling(override_colour_checkbox).padding(style.spacing);
		override_label.layout_align_middle(override_colour_swatch);
		@override_colour_checkbox.label = override_label;
		
		// HSL
		
		// TOOD: Might need some more options for ColourPicker to hide inputs etc. or make smaller?
		
		Divider@ override_divider = Divider(ui, Orientation::Vertical);
		override_divider.anchor_top.after(override_colour_swatch);
		override_divider.anchor_left.pixel(0);
		override_divider.anchor_right.pixel(0);
		edit_window.add_child(override_divider);
		
		// Accept/Cancel buttons
		
		Button@ btn = Button(ui, 'Accept');
		btn.fit_to_contents();
		btn.mouse_click.on(EventCallback(on_accept_click));
		edit_window.add_button_left(btn);
		
		@btn = Button(ui, 'Cancel');
		btn.fit_to_contents();
		btn.mouse_click.on(EventCallback(on_cancel_click));
		edit_window.add_button_right(btn);
		
		//
		
		update_properties_for_override();
	}
	
	// TODO: Implement
	// TODO: Disable and grey out override/hsl based on the override checkbox
	protected void update_edit_properties() override
	{
		if(!is_editing)
			return;
		
	}
	
	protected void update_properties_for_override()
	{
		override_colour_swatch.disabled = !override_colour_checkbox.checked;
	}
	
	// //////////////////////////////////////////////////////////
	// Editing
	// //////////////////////////////////////////////////////////
	
	// TODO: Implement
	// TODO: Might also have to check for mouse dragging sliders etc.
	protected bool sub_ui_active() override
	{
		return false;
	}
	
	// //////////////////////////////////////////////////////////
	// Events
	// //////////////////////////////////////////////////////////
	
	private void on_toolbar_button_click(EventInfo@ event)
	{
		const string name = event.target.name;
		
		if(name == 'adjust')
		{
			if(edit_button.selected)
			{
				start_editing();
			}
			else
			{
				stop_editing(true);
			}
		}
	}
	
	private void on_accept_click(EventInfo@ event)
	{
		stop_editing(true);
	}
	
	private void on_cancel_click(EventInfo@ event)
	{
		stop_editing(false);
	}
	
	private void on_override_colour_change(EventInfo@ event)
	{
		
	}
	
	private void on_override_colour_checked_change(EventInfo@ event)
	{
		update_properties_for_override();
	}
	
}
