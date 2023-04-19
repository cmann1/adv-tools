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
		rotation_wheel.degrees = float(vars.get_var('text_rotation').get_int32());
		scale_slider.value = vars.get_var('text_scale').get_float();
		
		selected_font_size = vars.get_var('font_size').get_int32();
		font_select.selected_value = vars.get_var('font').get_string();
		update_font_sizes();
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
