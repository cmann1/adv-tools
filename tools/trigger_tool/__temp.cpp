const string EMBED_spr_icon_text	= SPRITES_BASE + 'icon_text.png';

class TextTool : Tool
{
	
	private const array<int>@ font_sizes;
	private int selected_font_size;
	private bool ignore_next_font_size_update;
	
	void create_properties_container()
	{
		if(@properties_container != null)
			return;
		
		UI@ ui = script.ui;
		Style@ style = ui.style;
		
		@properties_container = Container(ui);
		properties_container.y = text_box.y + text_box.height + ui.style.spacing;
		properties_container.width = text_box.width;
		properties_container.anchor_left.pixel(0);
		properties_container.anchor_right.pixel(0);
		properties_container.anchor_bottom.pixel(0);
		
		text_box.anchor_bottom.sibling(properties_container).padding(style.spacing);
		
		@rotation_wheel = RotationWheel(ui);
		properties_container.add_child(rotation_wheel);
		
		// Colour swatch
		
		@colour_swatch = ColourSwatch(ui);
		colour_swatch.width  = rotation_wheel.height;
		colour_swatch.height = rotation_wheel.height;
		@colour_swatch.tooltip = PopupOptions(ui, 'Colour');
		colour_swatch.change.on(EventCallback(on_colour_change));
		properties_container.add_child(colour_swatch);
		
		// Layer button
		
		@layer_button = LayerButton(ui);
		layer_button.height = rotation_wheel.height;
		layer_button.auto_close = false;
		layer_button.x = colour_swatch.x + colour_swatch.width + style.spacing;
		@layer_button.tooltip = PopupOptions(ui, 'Layer');
		layer_button.change.on(EventCallback(on_layer_select));
		layer_button.select.on(EventCallback(on_layer_select));
		properties_container.add_child(layer_button);
		
		@layer_select = layer_button.layer_select;
		layer_select.multi_select = false;
		layer_select.min_select = 1;
		layer_select.min_select_layers = 1;
		
		// ROtation wheel
		
		rotation_wheel.x = layer_button.x + layer_button.width + style.spacing;
		rotation_wheel.start_angle = 0;
		rotation_wheel.allow_range = false;
		rotation_wheel.auto_tooltip = true;
		rotation_wheel.tooltip_precision = 0;
		rotation_wheel.tooltip_prefix = 'Rotation: ';
		rotation_wheel.change.on(EventCallback(on_rotation_change));
		
		// Scale slider
		
		@scale_slider = NumberSlider(ui, 0, NAN, NAN, 0.01);
		scale_slider.x = colour_swatch.x;
		scale_slider.y = colour_swatch.y + colour_swatch.height + style.spacing;
		scale_slider.width = (rotation_wheel.x + rotation_wheel.width) - scale_slider.x;
		@scale_slider.tooltip = PopupOptions(ui, 'Scale');
		scale_slider.change.on(EventCallback(on_scale_change));
		properties_container.add_child(scale_slider);
		
		// Font select
		
		@font_select = Select(ui);
		font_select.anchor_right.pixel(0);
		properties_container.add_child(font_select);
		
		font_select.add_value(font::ENVY_BOLD, 'Envy Bold');
		font_select.add_value(font::SANS_BOLD, 'Sans Bold');
		font_select.add_value(font::CARACTERES, 'Caracteres');
		font_select.add_value(font::PROXIMANOVA_REG, 'ProximaNovaReg');
		font_select.add_value(font::PROXIMANOVA_THIN, 'ProximaNovaThin');
		
		font_select.width = 200;
		font_select.x = properties_container.width - font_select.width;
		font_select.y = colour_swatch.y;
		font_select.change.on(EventCallback(on_font_change));
		
		Label@ font_label = create_label('Font');
		font_label.anchor_right.sibling(font_select).padding(style.spacing);
		font_label.y = font_select.y;
		font_label.height = font_select.height;
		
		// Font size select
		
		@font_size_select = Select(ui);
		font_size_select.anchor_right.pixel(0);
		properties_container.add_child(font_size_select);
		
		font_size_select.width = 85;
		font_size_select.x = properties_container.width - font_size_select.width;
		font_size_select.y = font_select.y + font_select.height + style.spacing;
		font_size_select.change.on(EventCallback(on_font_size_change));
		
		Label@ font_size_label = create_label('Size');
		font_size_label.anchor_right.sibling(font_size_select).padding(style.spacing);
		font_size_label.x = font_size_select.x - font_size_label.width - style.spacing;
		font_size_label.y = font_size_select.y;
		font_size_label.height = font_size_select.height;
		
		properties_container.fit_to_contents(true);
		@properties_container.layout = AnchorLayout(script.ui).set_padding(0);
		
		window.add_child(properties_container);
	}
	
	private Label@ create_label(const string text)
	{
		Label@ label = Label(script.ui, text);
		label.set_padding(script.ui.style.spacing, script.ui.style.spacing, 0, 0);
		label.align_v = GraphicAlign::Middle;
		label.fit_to_contents();
		properties_container.add_child(label);
		
		return label;
	}
	
	// //////////////////////////////////////////////////////////
	// Methods
	// //////////////////////////////////////////////////////////
	
	private void select(entity@ e)
	{
		if(is_z_trigger)
		{
			create_properties_container();
			update_properties();
			properties_container.visible = true;
		}
		else
		{
			if(@properties_container != null)
			{
				properties_container.visible = false;
			}
		}
	}
	
	private void update_properties()
	{
		ignore_events = true;
		
		colour_swatch.colour = vars.get_var('colour').get_int32();
		layer_select.set_selected_layer(vars.get_var('layer').get_int32(), false);
		layer_select.set_selected_sub_layer(vars.get_var('sublayer').get_int32(), false);
		rotation_wheel.degrees = float(vars.get_var('text_rotation').get_int32());
		scale_slider.value = vars.get_var('text_scale').get_float();
		
		selected_font_size = vars.get_var('font_size').get_int32();
		font_select.selected_value = vars.get_var('font').get_string();
		update_font_sizes();
		
		ignore_events = false;
	}
	
	private void update_font_sizes()
	{
		font_size_select.clear();
		@font_sizes = font::get_valid_sizes(font_select.selected_value);
		
		if(@font_sizes == null)
			return;
		
		int selected_index = -1;
		
		for(uint i = 0; i < font_sizes.length(); i++)
		{
			const int size = font_sizes[i];
			font_size_select.add_value(size + '', size + '');
			
			if(size == selected_font_size)
			{
				selected_index = i;
			}
		}
		
		font_size_select.selected_index = max(0, selected_index);
	}
	
	// //////////////////////////////////////////////////////////
	// Events
	// //////////////////////////////////////////////////////////
	
	void on_colour_change(EventInfo@ event)
	{
		if(ignore_events)
			return;
		
		vars.get_var('colour').set_int32(colour_swatch.colour);
	}
	
	void on_layer_select(EventInfo@ event)
	{
		if(ignore_events)
			return;
		
		vars.get_var('layer').set_int32(layer_select.get_selected_layer());
		vars.get_var('sublayer').set_int32(layer_select.get_selected_sub_layer());
	}
	
	void on_rotation_change(EventInfo@ event)
	{
		if(ignore_events)
			return;
		
		vars.get_var('text_rotation').set_int32(int(rotation_wheel.degrees));
	}
	
	void on_scale_change(EventInfo@ event)
	{
		if(ignore_events)
			return;
		
		vars.get_var('text_scale').set_float(scale_slider.value);
	}
	
	void on_font_change(EventInfo@ event)
	{
		if(ignore_events)
			return;
		
		vars.get_var('font').set_string(font_select.selected_value);
		ignore_next_font_size_update = true;
		update_font_sizes();
	}
	
	void on_font_size_change(EventInfo@ event)
	{
		if(ignore_events)
			return;
		
		if(font_size_select.selected_index == -1)
			return;
		
		vars.get_var('font_size').set_int32(font_sizes[font_size_select.selected_index]);
		
		if(!ignore_next_font_size_update)
		{
			selected_font_size = font_sizes[font_size_select.selected_index];
		}
		else
		{
			ignore_next_font_size_update = false;
		}
	}
	
}
