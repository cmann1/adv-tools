class UndoProp : callback_base
{
	
	PropTool@ tool;
	array<UndoPropData> props;
	bool position;
	bool rotation;
	bool scale;
	
	UndoProp() {}
	
	UndoProp(PropTool@ tool, array<PropData@>@ selected_props, int selected_props_count,
		bool rotation = false, bool scale = false
	) {
		@this.tool = tool;
		this.rotation = rotation;
		this.scale = scale;
		
		props.resize(selected_props_count);
		for(int i = 0; i < selected_props_count; i++)
		{
			props[i].init(selected_props[i], rotation, scale);
		}
	}
	
	void update(array<PropData@>@ selected_props, int selected_props_count) {
		for(int i = 0; i < selected_props_count; i++)
		{
			props[i].update(selected_props[i], rotation, scale);
		}
	}
	
	void undo() {
		for(uint i = 0; i < props.length; i++)
		{
			props[i].undo(rotation, scale);
		}
		
		if (rotation || scale)
		{
			tool.update_all_props();
		}
		else
		{
			tool.update_prop_positions();
			tool.recalculate_selection_bounds();
		}
	}
	
	void redo() {
		for(uint i = 0; i < props.length; i++)
		{
			props[i].redo(rotation, scale);
		}
		
		if (rotation || scale)
		{
			tool.update_all_props();
		}
		else
		{
			tool.update_prop_positions();
			tool.recalculate_selection_bounds();
		}
	}
}

class UndoPropData
{
	
	prop@ prop;
	
	float start_x, start_y;
	float end_x, end_y;
	float start_rotation;
	float end_rotation;
	float start_scale_x, start_scale_y;
	float end_scale_x, end_scale_y;
	
	void init(PropData@ data, bool rotation, bool scale)
	{
		@prop = data.prop;
		start_x = end_x = data.x;
		start_y = end_y = data.y;
		if (rotation)
		{
			start_rotation = end_rotation = data.prop.rotation();
		}
		if (scale)
		{
			start_scale_x = end_scale_x = data.prop.scale_x();
			start_scale_y = end_scale_y = data.prop.scale_y();
		}
	}
	
	void update(PropData@ data, bool rotation, bool scale)
	{
		end_x = data.x;
		end_y = data.y;
		if (rotation)
		{
			end_rotation = data.prop.rotation();
		}
		if (scale)
		{
			end_scale_x = data.prop.scale_x();
			end_scale_y = data.prop.scale_y();
		}
	}
	
	void undo(bool rotation, bool scale)
	{
		prop.x(start_x);
		prop.y(start_y);
		if (rotation)
		{
			prop.rotation(start_rotation);
		}
		if (scale)
		{
			prop.scale_x(start_scale_x);
			prop.scale_y(start_scale_y);
		}
	}
	
	void redo(bool rotation, bool scale)
	{
		prop.x(end_x);
		prop.y(end_y);
		if (rotation)
		{
			prop.rotation(end_rotation);
		}
		if (scale)
		{
			prop.scale_x(end_scale_x);
			prop.scale_y(end_scale_y);
		}
	}
	
}
