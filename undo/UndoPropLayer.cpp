class UndoPropLayer : callback_base
{
	
	PropTool@ tool;
	array<UndoPropLayerData> props;
	
	UndoPropLayer(PropTool@ tool, array<PropData@>@ selected_props, int selected_props_count)
	{
		@this.tool = tool;
		
		props.resize(selected_props_count);
		for(int i = 0; i < selected_props_count; i++)
		{
			props[i].init(selected_props[i]);
		}
	}
	
	UndoPropLayer(PropTool@ tool, PropData@ data)
	{
		@this.tool = tool;
		
		props.resize(1);
		props[0].init(data);
	}
	
	void update(array<PropData@>@ selected_props, int selected_props_count) {
		for(int i = 0; i < selected_props_count; i++)
		{
			props[i].update(selected_props[i]);
		}
	}
	
	void update(PropData@ data) {
		props[0].update(data);
	}
	
	bool can_update(array<PropData@>@ selected_props, int selected_props_count)
	{
		if(uint(selected_props_count) != props.length)
			return false;
		
		for(int i = 0; i < selected_props_count; i++)
		{
			if(selected_props[i].prop.id() != props[i].prop.id())
				return false;
		}
		
		return  true;
	}
	
	bool can_update(PropData@ data)
	{
		return props.length == 1 && data.prop.id() == props[0].prop.id();
	}
	
	void undo() {
		for(uint i = 0; i < props.length; i++)
		{
			UndoPropLayerData@ data = @props[i];
			move_layer(data.prop, data.end_layer, data.start_layer, data.start_sublayer);
		}
		
		tool.update_all_props();
		tool.invalidate_selection_layer_change();
		tool.try_update_info();
	}
	
	void redo() {
		for(uint i = 0; i < props.length; i++)
		{
			UndoPropLayerData@ data = @props[i];
			move_layer(data.prop, data.start_layer, data.end_layer, data.end_sublayer);
		}
		tool.update_all_props();
		tool.invalidate_selection_layer_change();
		tool.try_update_info();
	}
	
	void move_layer(prop@ prop, int old_layer, int new_layer, int sublayer)
	{
		if(new_layer <= 5 && old_layer > 5 || new_layer > 5 && old_layer <= 5)
		{
			tool.script.g.remove_prop(prop);
			prop.layer(new_layer);
			tool.script.g.add_prop(prop);
		}
		else
		{
			prop.layer(new_layer);
		}
		
		prop.sub_layer(sublayer);
	}
	
}

class UndoPropLayerData
{
	prop@ prop;
	uint start_layer, start_sublayer;
	uint end_layer, end_sublayer;
	
	void init(PropData@ data)
	{
		@prop = data.prop;
		start_layer = end_layer = data.layer;
		start_sublayer = end_sublayer = data.sub_layer;
	}
	
	void update(PropData@ data)
	{
		end_layer = data.layer;
		end_sublayer = data.sub_layer;
	}
}
