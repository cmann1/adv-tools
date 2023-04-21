#include '../../../../lib/ui3/layouts/AnchorLayout.cpp';
#include '../../../../lib/ui3/elements/Button.cpp';
#include '../../../../lib/ui3/elements/ColourSwatch.cpp';
#include '../../../../lib/ui3/elements/colour_picker/ColourPicker.cpp';
#include '../../../../lib/ui3/elements/extra/Panel.cpp';
#include '../../../../lib/ui3/elements/Toolbar.cpp';
#include '../../../../lib/ui3/elements/Window.cpp';

#include 'FogTriggerHandlerData.cpp';

const string TRIGGERS_SPRITES_BASE = SPRITES_BASE + 'triggers/';
const string EMBED_spr_fog_adjust = TRIGGERS_SPRITES_BASE + 'fog_adjust.png';

class FogTriggerHandler : TriggerToolHandler
{
	
	private Toolbar@ toolbar;
	
	private ColourSwatch@ override_colour_swatch;
	private Checkbox@ override_colour_checkbox;
	private ColourPicker@ hsl_adjuster;
	private Panel@ filter_box;
	private Checkbox@ filter_layer_selected_checkbox;
	
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
		if(state == TriggerHandlerState::Idle)
		{
			check_edit_keys();
		}
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
		edit_window.name = 'FogToolTextProperties';
		edit_window.set_icon(SPRITE_SET, 'fog_adjust', Settings::IconSize, Settings::IconSize);
		edit_window.x = 200;
		edit_window.y = 20;
		@edit_window.layout = AnchorLayout(ui).set_padding(0);
		edit_window.close.on(EventCallback(on_cancel_click));
		
		// Override colour swatch
		//{
		@override_colour_swatch = ColourSwatch(ui);
		@override_colour_swatch.tooltip = PopupOptions(ui, 'Override colour');
		override_colour_swatch.change.on(EventCallback(on_override_colour_change));
		override_colour_swatch.activate.on(EventCallback(on_override_colour_change));
		edit_window.add_child(override_colour_swatch);
		
		@override_colour_checkbox = Checkbox(ui);
		override_colour_checkbox.checked = true;
		override_colour_checkbox.anchor_left.next_to(override_colour_swatch);
		//override_colour_checkbox.layout_align_middle(override_colour_swatch);
		override_colour_checkbox.change.on(EventCallback(on_override_colour_checked_change));
		edit_window.add_child(override_colour_checkbox);
		
		Label@ override_label = script.create_label('Override colour', edit_window);
		override_label.anchor_left.sibling(override_colour_checkbox).padding(NAN);
		//override_label.layout_align_middle(override_colour_swatch);
		@override_colour_checkbox.label = override_label;
		//}
		
		// HSL
		//{
		@hsl_adjuster = ColourPicker(ui);
		hsl_adjuster.show_rgb = false;
		hsl_adjuster.show_alpha = false;
		hsl_adjuster.show_buttons = false;
		hsl_adjuster.show_hex = false;
		hsl_adjuster.show_swatches = false;
		hsl_adjuster.anchor_top.next_to(override_colour_swatch);
		hsl_adjuster.anchor_left.pixel(0);
		hsl_adjuster.anchor_right.pixel(0);
		hsl_adjuster.fit_to_contents(true);
		hsl_adjuster.change.on(EventCallback(on_adjust_colour_change));
		edit_window.add_child(hsl_adjuster);
		//}
		
		// Filter box
		
		@filter_box = Panel(ui);
		filter_box.anchor_top.next_to(hsl_adjuster, style.spacing * 2);
		filter_box.anchor_left.pixel(0);
		filter_box.anchor_right.pixel(0);
		filter_box.height = 100;
		filter_box.border_colour = style.normal_border_clr;
		filter_box.border_size = style.border_size;
		filter_box.title = 'Filter';
		filter_box.only_title_border = true;
		filter_box.collapsible = true;
		filter_box.show_collapse_arrow = true;
		@filter_box.layout = AnchorLayout(ui).set_padding(0, 0, style.spacing * 2, 0);
		filter_box.collapse.on(EventCallback(on_filter_box_collapsed));
		edit_window.add_child(filter_box);
		
		// Filter layer
		
		@filter_layer_selected_checkbox = Checkbox(ui);
		filter_layer_selected_checkbox.checked = false;
		filter_layer_selected_checkbox.anchor_left.pixel(0);
		filter_layer_selected_checkbox.change.on(EventCallback(on_filter_layer_change));
		filter_box.add_child(filter_layer_selected_checkbox);
		
		Label@ filter_layer_selected_label = script.create_label('Selected layer only', filter_box);
		filter_layer_selected_label.anchor_left.sibling(filter_layer_selected_checkbox).padding(NAN);
		filter_layer_selected_label.anchor_top.sibling(filter_layer_selected_checkbox, 0.5).align_v(0.5);
		@filter_layer_selected_checkbox.label = filter_layer_selected_label;
		
		filter_box.fit_to_contents(true);
		
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
		
		edit_window.fit_to_contents();
		update_properties_for_override();
	}
	
	protected void update_edit_properties() override
	{
		if(!is_editing)
			return;
		
		hsl_adjuster.h = 0.5;
		hsl_adjuster.s = 1;
		hsl_adjuster.l = 1;
		
		update_properties_for_override();
	}
	
	protected void update_properties_for_override()
	{
		override_colour_swatch.disabled = !override_colour_checkbox.checked;
		hsl_adjuster.disabled = override_colour_checkbox.checked;
	}
	
	// //////////////////////////////////////////////////////////
	// Editing
	// //////////////////////////////////////////////////////////
	
	// TODO: Implement
	// TODO: Might also have to check for mouse dragging sliders etc.
	protected bool sub_ui_active() override
	{
		return script.ui.is_mouse_active;
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
		if(ignore_edit_ui_events)
			return;
		
		puts('on_override_colour_change');
	}
	
	private void on_override_colour_checked_change(EventInfo@ event)
	{
		if(ignore_edit_ui_events)
			return;
		
		update_properties_for_override();
	}
	
	private void on_adjust_colour_change(EventInfo@ event)
	{
		if(ignore_edit_ui_events)
			return;
		
		puts('on_adjust_colour_change');
	}
	
	private void on_filter_box_collapsed(EventInfo@ event)
	{
		if(ignore_edit_ui_events)
			return;
		
		filter_box.fit_to_contents();
		edit_window.fit_to_contents();
	}
	
	private void on_filter_layer_change(EventInfo@ event)
	{
		if(ignore_edit_ui_events)
			return;
		
		
	}
	
}
