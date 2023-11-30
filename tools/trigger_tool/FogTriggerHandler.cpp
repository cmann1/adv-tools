#include '../../../../lib/utils/colour.cpp';

#include '../../../../lib/ui3/layouts/AnchorLayout.cpp';
#include '../../../../lib/ui3/elements/Button.cpp';
#include '../../../../lib/ui3/elements/ColourSwatch.cpp';
#include '../../../../lib/ui3/elements/colour_picker/ColourPicker.cpp';
#include '../../../../lib/ui3/elements/colour_picker/ColourSlider.cpp';
#include '../../../../lib/ui3/elements/MultiButton.cpp';
#include '../../../../lib/ui3/elements/extra/Panel.cpp';
#include '../../../../lib/ui3/elements/Toolbar.cpp';
#include '../../../../lib/ui3/elements/Window.cpp';

#include 'ContrastType.cpp';
#include 'FogTriggerHandlerData.cpp';

const string TRIGGERS_SPRITES_BASE = SPRITES_BASE + 'triggers/';
const string EMBED_spr_fog_adjust = TRIGGERS_SPRITES_BASE + 'fog_adjust.png';

class FogTriggerHandler : TriggerToolHandler
{
	
	private Toolbar@ toolbar;
	
	private ColourSwatch@ override_colour_swatch;
	private Checkbox@ override_colour_checkbox;
	private ColourPicker@ hsl_adjuster;
	private ColourSlider@ contrast_slider;
	private TextBox@ contrast_input;
	private MultiButton@ contrast_type_button;
	private Panel@ filter_box;
	private Checkbox@ filter_layer_selected_checkbox;
	private ColourSwatch@ filter_colour_swatch;
	private ColourSwatch@ filter_colour_tolerance;
	private LayerButton@ filter_layer_button;
	private LayerSelector@ filter_layer_select;
	private Checkbox@ filter_default_sub_layer_checkbox;
	private Checkbox@ filter_sky_top_checkbox;
	private Checkbox@ filter_sky_mid_checkbox;
	private Checkbox@ filter_sky_bot_checkbox;
	
	/* True if any of the selected triggers have sub layers. */
	private bool has_sub_layers;
	private int last_filtered_layer = -1;
	private ContrastType contrast_type = Simple1;
	
	FogTriggerHandler(AdvToolScript@ script, ExtendedTriggerTool@ tool)
	{
		super(script, tool, 'FogTool');
		
		on_settings_loaded();
	}
	
	void on_settings_loaded() override
	{
		show_popup = script.config.get_bool('ShowFogEditButton', true);
		can_edit_with_enter = script.config.get_bool('EditFogWithEnter', true);
		
		if(!show_popup)
		{
			show_selected_popup(false);
		}
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
		
		if(is_editing && last_filtered_layer != script.layer && filter_layer_selected_checkbox.checked)
		{
			reset_colour();
			do_adjust();
		}
		
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
		has_sub_layers = false;
		
		for(uint i = 0; i < select_list.length; i++)
		{
			FogTriggerHandlerData@ data = selected(i);
			if(data.has_sub_layers)
			{
				has_sub_layers = true;
				break;
			}
		}
		
		update_filter_layer_selector();
		
		do_selection_change_for_editing(primary, added, removed);
	}
	
	protected void start_editing() override
	{
		TriggerToolHandler::start_editing();
		
		if(override_colour_checkbox.checked)
		{
			do_adjust();
		}
	}
	
	protected void stop_editing(const bool accept, const bool update_ui=true) override
	{
		if(!is_editing)
			return;
		
		TriggerToolHandler::stop_editing(accept, update_ui);
		
		if(!accept && select_list.length > 0)
		{
			script.cam.change_fog(selected(0).fog, 0);
		}
	}
	
	private void do_adjust()
	{
		const bool filter_default_sub_layers = filter_default_sub_layer_checkbox.checked;
		const bool all_layers =
			filter_layer_select.num_layers_selected() == 0 || filter_layer_select.num_layers_selected() == 21;
		const bool all_sub_layers = (filter_layer_select.num_sub_layers_selected() == 0 || filter_layer_select.num_sub_layers_selected() == 25);
		
		const bool has_tolerance = this.has_tolerance();
		const float adjust_h = hsl_adjuster.h - 0.5;
		const float adjust_s = hsl_adjuster.s * 2 - 1;
		const float adjust_v = hsl_adjuster.l * 2 - 1;
		const float filter_h = filter_colour_swatch.h;
		const float filter_s = filter_colour_swatch.s;
		const float filter_v = filter_colour_swatch.l;
		const float filter_a = filter_colour_swatch.a;
		const float tolerance_h = filter_colour_tolerance.h;
		const float tolerance_s = filter_colour_tolerance.s;
		const float tolerance_v = filter_colour_tolerance.l;
		const float tolerance_a = filter_colour_tolerance.a;
		const int selected_layer = filter_layer_selected_checkbox.checked
			? script.layer : -1;
		
		float contrast = get_contrast_percent();
		const bool has_contrast = !approximately(contrast, 0);
		const bool average_contrast = contrast_type == Average;
		
		if(has_contrast)
		{
			// Convert from -1>1 to ranges needed for algorithm
			switch(contrast_type)
			{
				// 0>2
				case Simple1:
					contrast += 1; break;
				// -255>255
				case Simple2:
					contrast *= 255; break;
				// 0>255
				case Average:
					contrast = contrast < 1 ? (1 + contrast) / (1 - contrast) : 255; break;
			}
		}
		
		float mean_brightness = 0;
		int mean_brightness_count = 0;
		
		const int AveragePass = 0;
		const int AdjustPass = 1;
		const int first_pass = average_contrast ? AveragePass : AdjustPass;
		
		const int SkyTop = -4;
		const int SkyMid = -3;
		const int SkyBot = -2;
		const int DefaultSubLayer = -1;
		
		for(int pass = first_pass; pass <= AdjustPass; pass++)
		{
			if(pass == AdjustPass && has_contrast && average_contrast && mean_brightness_count > 0)
			{
				mean_brightness /= mean_brightness_count;
			}
			
			for(uint i = 0; i < select_list.length; i++)
			{
				FogTriggerHandlerData@ data = selected(i);
				fog_setting@ fog = data.fog;
				fog_setting@ to_fog = data.edit_fog;
				
				for (int layer = SkyTop; layer <= 20; layer++)
				{
					if(layer == -1)
						continue;
					if(
						layer == SkyTop && !filter_sky_top_checkbox.checked ||
						layer == SkyMid && !filter_sky_mid_checkbox.checked ||
						layer == SkyBot && !filter_sky_bot_checkbox.checked)
						continue;
					// Sky layers
					if(selected_layer != -1 && layer < -1)
						continue;
					if(selected_layer != -1 && layer >= 0 && layer != selected_layer)
						continue;
					if(!all_layers && !filter_layer_select.is_layer_selected(layer))
						continue;
					
					const int sub_layer_start = layer >= 0 ? DefaultSubLayer : layer;
					const int sub_layer_end = layer >= 0 ? (data.has_sub_layers ? 24 : -1) : layer;
					
					for (int sub_layer = sub_layer_start; sub_layer <= sub_layer_end; sub_layer++)
					{
						if(data.has_sub_layers && (
							!all_sub_layers && sub_layer > DefaultSubLayer && !filter_layer_select.is_sub_layer_selected(sub_layer) ||
							sub_layer == DefaultSubLayer && !filter_default_sub_layers)
						)
							continue;
						
						uint clr;
						int r, g, b, a;
						float h, s, v;
						
						// Fetch the colour
						if(has_tolerance || has_contrast || pass == AveragePass || !override_colour_checkbox.checked)
						{
							switch(sub_layer)
							{
								case SkyTop: clr = fog.bg_top(); break;
								case SkyMid: clr = fog.bg_mid(); break;
								case SkyBot: clr = fog.bg_bot(); break;
								case DefaultSubLayer: clr = fog.layer_colour(layer); break;
								default: clr = fog.colour(layer, sub_layer); break;
							}
							
							int_to_rgba(clr, r, g, b, a);
							
							if(has_tolerance && pass == AdjustPass)
							{
								rgb_to_hsv(r, g, b, h, s, v);
								
								if(
									abs(h - filter_h) > tolerance_h ||
									abs(s - filter_s) > tolerance_s ||
									abs(v - filter_v) > tolerance_v ||
									abs(a - filter_a) > tolerance_a
								)
								{
									if(layer < 0)
										break;
									else
										continue;
								}
							}
							
							if(pass == AveragePass)
							{
								mean_brightness += (r + g + b) / 3.0;
								mean_brightness_count++;
								
								if(layer < 0)
									break;
								else
									continue;
							}
						}
						
						// Adjust
						if(!override_colour_checkbox.checked)
						{
							if(!has_tolerance)
							{
								rgb_to_hsv(r, g, b, h, s, v);
							}
							
							h = (h + adjust_h) % 1;
							if(h < 0)
							{
								h += 1;
							}
							
							s = clamp01(s + adjust_s);
							v = clamp01(v + adjust_v);
							
							clr = hsv_to_rgb(h, s, v) | (a << 24);
							
							if(has_contrast)
							{
								int_to_rgba(clr, r, g, b, a);
								float r1, g1, b1;
								
								switch(contrast_type)
								{
									case Simple1:
									{
										r1 = (r - 128) * contrast + 128;
										g1 = (g - 128) * contrast + 128;
										b1 = (b - 128) * contrast + 128;
									} break;
									case Simple2:
									{
										const float factor = (259 * (contrast + 255)) / (255 * (259 - contrast));
										r1 = (r - 128) * factor + 128;
										g1 = (g - 128) * factor + 128;
										b1 = (b - 128) * factor + 128;
									} break;
									case Average:
									{
										r1 = (r - mean_brightness) * contrast + mean_brightness;
										g1 = (g - mean_brightness) * contrast + mean_brightness;
										b1 = (b - mean_brightness) * contrast + mean_brightness;
									} break;
								}
								
								r = clamp(int(r1), 0, 255);
								g = clamp(int(g1), 0, 255);
								b = clamp(int(b1), 0, 255);
								clr = rgba(r, g, b, a);
							}
						}
						else
						{
							clr = override_colour_swatch.colour;
						}
						
						switch(sub_layer)
						{
							case SkyTop: to_fog.bg_top(clr); break;
							case SkyMid: to_fog.bg_mid(clr); break;
							case SkyBot: to_fog.bg_bot(clr); break;
							case DefaultSubLayer: to_fog.default_colour(layer, clr); break;
							default: to_fog.colour(layer, sub_layer, clr); break;
						}
						
						if(layer < 0)
							break;
					}
				}
				
				to_fog.copy_to(data.trigger);
				
				if(i == 0)
				{
					script.cam.change_fog(to_fog, 0);
				}
			}
		}
		
		last_filtered_layer = script.layer;
	}
	
	/**
	 * @brief When changing filters it's necessary to reset all fog to prevent previous changes from persisting.
	 */
	private void reset_colour()
	{
		for(uint i = 0; i < select_list.length; i++)
		{
			FogTriggerHandlerData@ data = selected(i);
			data.edit_fog.copy_from(data.fog);
		}
	}
	
	private bool has_tolerance()
	{
		return
			filter_colour_tolerance.h < 1 ||
			filter_colour_tolerance.s < 1 ||
			filter_colour_tolerance.l < 1 ||
			filter_colour_tolerance.a < 255;
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
		
		EventCallback@ on_change_delegate = EventCallback(on_adjust_property_change);
		EventCallback@ on_filter_change_delegate = EventCallback(on_filter_change);
		
		// Override colour swatch
		//{
		@override_colour_swatch = ColourSwatch(ui);
		@override_colour_swatch.tooltip = PopupOptions(ui, 'Override colour');
		override_colour_swatch.change.on(EventCallback(on_override_colour_change));
		override_colour_swatch.activate.on(EventCallback(on_override_colour_change));
		edit_window.add_child(override_colour_swatch);
		
		@override_colour_checkbox = Checkbox(ui);
		override_colour_checkbox.checked = false;
		override_colour_checkbox.anchor_left.next_to(override_colour_swatch);
		override_colour_checkbox.anchor_top.sibling(override_colour_swatch, 0.5).align_v(0.5);
		override_colour_checkbox.change.on(EventCallback(on_override_colour_checked_change));
		edit_window.add_child(override_colour_checkbox);
		
		Label@ override_label = script.create_label('Override colour', edit_window);
		override_label.anchor_left.next_to(override_colour_checkbox);
		override_label.anchor_top.sibling(override_colour_swatch, 0.5).align_v(0.5);
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
		hsl_adjuster.change.on(on_change_delegate);
		edit_window.add_child(hsl_adjuster);
		//}
		
		// Contrast type button
		//{
		@contrast_type_button = MultiButton(ui);
		contrast_type_button.add_label('simple1', 'C1', 'Contrast Algorithm');
		contrast_type_button.add_label('simple2', 'C2', 'Contrast Algorithm');
		contrast_type_button.add_label('average', 'C3', 'Contrast Algorithm');
		contrast_type_button.set_font('envy_bold', 20);
		contrast_type_button.height = override_colour_swatch.height;
		contrast_type_button.anchor_right.sibling(hsl_adjuster, 1);
		contrast_type_button.anchor_top.sibling(override_colour_swatch, 0.5).align_v(0.5);
		contrast_type_button.select.on(EventCallback(on_contrast_type_select));
		edit_window.add_child(contrast_type_button);
		//}
		
		// Contrast
		//{
		@contrast_slider = ColourSlider(ui);
		contrast_slider.type = TriColour;
		contrast_slider.colour = 0xff333333;
		contrast_slider.colour2 = 0xffdddddd;
		contrast_slider.colour3 = 0xff333333;
		contrast_slider.value = 0.5;
		contrast_slider.anchor_top.next_to(hsl_adjuster);
		contrast_slider.anchor_left.pixel(0);
		contrast_slider.change.on(EventCallback(on_contrast_slider_change));
		edit_window.add_child(contrast_slider);
		
		Label@ contrast_label = Label(ui, 'C');
		contrast_label.set_font(font::ENVY_BOLD, 20);
		contrast_label.text_align_h = TextAlign::Centre;
		contrast_label.align_v = GraphicAlign::Middle;
		contrast_label.mouse_enabled = false;
		contrast_label.width = 24;
		contrast_label.anchor_left.next_to(contrast_slider);
		contrast_label.anchor_top.sibling(contrast_slider, 1);
		contrast_label.anchor_bottom.sibling(contrast_slider, 1);
		edit_window.add_child(contrast_label);
		
		@contrast_input = TextBox(ui, '', font::ENVY_BOLD, 20);
		contrast_input.name = 'C';
		contrast_input.width = 65;
		contrast_input.character_validation = Decimal;
		contrast_input.allow_negative = true;
		contrast_input.anchor_left.next_to(contrast_label);
		contrast_input.anchor_right.pixel(0);
		contrast_input.anchor_top.sibling(contrast_slider, 1);
		contrast_input.anchor_bottom.sibling(contrast_slider, 1);
		contrast_input.change.on(EventCallback(on_contrast_input_change));
		edit_window.add_child(contrast_input);
		
		hsl_adjuster.navigation.add_last(contrast_input);
		//}
		
		// Filter box
		//{
		@filter_box = Panel(ui);
		filter_box.collapsed = true;
		filter_box.anchor_top.next_to(contrast_slider, style.spacing * 2);
		filter_box.anchor_left.pixel(0);
		filter_box.anchor_right.pixel(0);
		filter_box.width = 100;
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
		//}
		
		// Filter colour and threshold
		//{
		@filter_colour_swatch = ColourSwatch(ui);
		@filter_colour_swatch.tooltip = PopupOptions(ui, 'Filter by layer colour');
		filter_colour_swatch.change.on(EventCallback(on_filter_colour_change));
		filter_colour_swatch.activate.on(EventCallback(on_filter_colour_change));
		filter_box.add_child(filter_colour_swatch);
		
		@filter_colour_tolerance = ColourSwatch(ui);
		@filter_colour_tolerance.tooltip = PopupOptions(ui,
			'Layer colour filter tolerance.\n' +
			'How closely the layer colour HSL values must match\n' +
			'the filter colour:\n' +
			'   0,0,0 - Exact match\n' +
			'   1,1,1 - No filtering - all colours/layers');
		filter_colour_tolerance.show_rgb = false;
		filter_colour_tolerance.show_hex = false;
		filter_colour_tolerance.force_hsl = true;
		filter_colour_tolerance.set_hsl(1, 1, 1, 255);
		filter_colour_tolerance.anchor_left.next_to(filter_colour_swatch);
		filter_colour_tolerance.anchor_top.sibling(filter_colour_swatch, 1);
		filter_colour_tolerance.change.on(EventCallback(on_filter_tolerance_colour_change));
		filter_colour_tolerance.activate.on(EventCallback(on_filter_tolerance_colour_change));
		filter_box.add_child(filter_colour_tolerance);
		//}
		
		// Selected layer, Default sub layer, and sky
		//{
		const array<string> names = {'selected', 'default', 'sky_top', 'sky_mid', 'sky_bot'};
		const array<string> labels = {'Selected layer only', 'Default sub layer', 'Sky top', 'Sky mid', 'Sky bot'};
		const array<bool> checked = {false, true, true, true, true};
		const array<string> tooltip = {
			'If selected only the layer selected in the editor will be adjusted',
			'If selected the default/vanilla sub layer will be adjusted',
			'', '', ''};
		array<Checkbox@> checkboxes(labels.length);
		
		Element@ prev = filter_colour_swatch;
		
		for(uint i = 0; i < labels.length; i++)
		{
			Checkbox@ checkbox = Checkbox(ui);
			checkbox.name = names[i];
			checkbox.checked = checked[i];
			checkbox.anchor_top.next_to(prev);
			checkbox.anchor_left.pixel(0);
			checkbox.change.on(on_filter_change_delegate);
			filter_box.add_child(checkbox);
			
			Label@ label = script.create_label(labels[i], filter_box);
			label.anchor_left.next_to(checkbox);
			label.anchor_top.sibling(checkbox, 0.5).align_v(0.5);
			@label.tooltip = tooltip[i] != '' ? PopupOptions(ui, tooltip[i]) : null;
			@checkbox.label = label;
			
			@prev = checkbox;
			@checkboxes[i] = checkbox;
		}
		
		uint i = 0;
		@filter_layer_selected_checkbox = checkboxes[i++];
		@filter_default_sub_layer_checkbox = checkboxes[i++];
		@filter_sky_top_checkbox = checkboxes[i++];
		@filter_sky_mid_checkbox = checkboxes[i++];
		@filter_sky_bot_checkbox = checkboxes[i++];
		//}
		
		// Filter layer
		//{
		@filter_layer_button = LayerButton(ui);
		filter_layer_button.auto_close = false;
		@filter_layer_button.tooltip = PopupOptions(ui, 'Filter by layers');
		filter_layer_button.anchor_right.pixel(0);
		filter_layer_button.anchor_top.next_to(filter_colour_swatch);
		filter_layer_button.change.on(EventCallback(on_filter_layer_change));
		filter_layer_button.select.on(EventCallback(on_filter_layer_change));
		filter_box.add_child(filter_layer_button);
		
		@filter_layer_select = filter_layer_button.layer_select;
		filter_layer_select.multi_select = true;
		filter_layer_select.show_all_layers_toggle = true;
		filter_layer_select.show_all_sub_layers_toggle = true;
		filter_layer_select.set_ui_layers_visibility(false);
		//}
		
		filter_box.fit_to_contents();
		
		// Accept/Cancel buttons
		//{
		Button@ btn = Button(ui, 'Accept');
		btn.fit_to_contents();
		btn.mouse_click.on(EventCallback(on_accept_click));
		edit_window.add_button_left(btn);
		
		@btn = Button(ui, 'Cancel');
		btn.fit_to_contents();
		btn.mouse_click.on(EventCallback(on_cancel_click));
		edit_window.add_button_right(btn);
		//}
		
		//
		
		edit_window.fit_to_contents(true);
		
		update_properties_for_override();
		update_filter_layer_selector();
	}
	
	protected void update_edit_properties() override
	{
		if(!is_editing)
			return;
		
		hsl_adjuster.h = 0.5;
		hsl_adjuster.s = 0.5;
		hsl_adjuster.l = 0.5;
		contrast_slider.value = 0.5;
		
		update_properties_for_override();
		update_contrast_input();
	}
	
	protected void update_properties_for_override()
	{
		override_colour_swatch.disabled = !override_colour_checkbox.checked;
		hsl_adjuster.disabled = override_colour_checkbox.checked;
	}
	
	protected void update_filter_layer_selector()
	{
		if(@filter_layer_select == null)
			return;
		
		filter_layer_select.type = has_sub_layers
			? LayerSelectorType::Both : LayerSelectorType::Layers;
	}
	
	protected void update_contrast_slider()
	{
		contrast_slider.value = clamp01((contrast_input.float_value + 1) / 2.0);
	}
	
	protected void update_contrast_input()
	{
		contrast_input.float_value = get_contrast_percent();
	}
	
	protected float get_contrast_percent() const
	{
		return (contrast_slider.value - 0.5) * 2;
	}
	
	// //////////////////////////////////////////////////////////
	// Editing
	// //////////////////////////////////////////////////////////
	
	protected bool sub_ui_active() override
	{
		return
			script.ui.is_mouse_active ||
			@override_colour_swatch != null && override_colour_swatch.open ||
			@filter_colour_swatch != null && filter_colour_swatch.open ||
			@filter_layer_button != null && filter_layer_button.open;
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
	
	private void on_adjust_property_change(EventInfo@ event)
	{
		if(ignore_edit_ui_events)
			return;
		
		do_adjust();
	}
	
	private void on_override_colour_change(EventInfo@ event)
	{
		if(ignore_edit_ui_events)
			return;
		
		if(
			event.type != EventType::ACCEPT && event.type != EventType::OPEN && event.type != EventType::CLOSE ||
			event.type == EventType::CANCEL)
		{
			do_adjust();
		}
	}
	
	private void on_override_colour_checked_change(EventInfo@ event)
	{
		if(ignore_edit_ui_events)
			return;
		
		update_properties_for_override();
		do_adjust();
	}
	
	private void on_contrast_slider_change(EventInfo@ event)
	{
		if(ignore_edit_ui_events)
			return;
		
		ignore_edit_ui_events = true;
		update_contrast_input();
		ignore_edit_ui_events = false;
		do_adjust();
	}
	
	private void on_contrast_input_change(EventInfo@ event)
	{
		if(ignore_edit_ui_events)
			return;
		
		ignore_edit_ui_events = true;
		update_contrast_slider();
		ignore_edit_ui_events = false;
		do_adjust();
	}
	
	private void on_contrast_type_select(EventInfo@ event)
	{
		
		if(ignore_edit_ui_events)
			return;
		
		const string type = contrast_type_button.selected_name;
		
		if(type == 'simple1')
			contrast_type = Simple1;
		else if(type == 'simple2')
			contrast_type = Simple2;
		else if(type == 'average')
			contrast_type = Average;
		
		const bool has_contrast = !approximately(get_contrast_percent(), 0);
		if(has_contrast)
		{
			do_adjust();
		}
	}
	
	private void on_filter_box_collapsed(EventInfo@ event)
	{
		if(ignore_edit_ui_events)
			return;
		
		filter_box.fit_to_contents();
		edit_window.fit_to_contents();
	}
	
	private void on_filter_colour_change(EventInfo@ event)
	{
		if(ignore_edit_ui_events)
			return;
		if(!has_tolerance())
			return;
		
		if(
			event.type != EventType::ACCEPT && event.type != EventType::OPEN && event.type != EventType::CLOSE ||
			event.type == EventType::CANCEL)
		{
			reset_colour();
			do_adjust();
		}
	}
	
	private void on_filter_tolerance_colour_change(EventInfo@ event)
	{
		if(ignore_edit_ui_events)
			return;
		
		if(
			event.type != EventType::ACCEPT && event.type != EventType::OPEN && event.type != EventType::CLOSE ||
			event.type == EventType::CANCEL)
		{
			reset_colour();
			do_adjust();
		}
	}
	
	private void on_filter_layer_change(EventInfo@ event)
	{
		if(ignore_edit_ui_events)
			return;
		
		if(
			event.type != EventType::SELECT && event.type != EventType::OPEN && event.type != EventType::CLOSE ||
			event.type == EventType::CANCEL)
		{
			reset_colour();
			do_adjust();
		}
	}
	
	private void on_filter_change(EventInfo@ event)
	{
		if(ignore_edit_ui_events)
			return;
		
		reset_colour();
		do_adjust();
	}
	
}
