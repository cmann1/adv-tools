class TextTool : Tool
{
	
	private const array<int>@ font_sizes;
	private int selected_font_size;
	private bool ignore_next_font_size_update;
	
	// //////////////////////////////////////////////////////////
	// Methods
	// //////////////////////////////////////////////////////////
	
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
