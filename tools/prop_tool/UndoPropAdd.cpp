class UndoPropAdd : callback_base
{
	
	PropTool@ tool;
	private bool add;
	array<prop@> props;
	
	UndoPropAdd(PropTool@ tool, bool add, array<PropData@>@ selected_props, int selected_props_count)
	{
		@this.tool = tool;
		this.add = add;
		
		props.resize(selected_props_count);
		for(int i = 0; i < selected_props_count; i++)
		{
			@props[i] = selected_props[i].prop;
		}
	}
	
	void add_prop(PropData@ data)
	{
		props.insertLast(data.prop);
	}
	
	void undo() {
		if(add)
		{
			for(uint i = 0; i < props.length; i++)
			{
				tool.script.g.remove_prop(props[i]);
			}
		}
		else
		{
			for(uint i = 0; i < props.length; i++)
			{
				tool.script.g.add_prop(props[i]);
			}
		}
	}
	
	void redo() {
		if(add)
		{
			for(uint i = 0; i < props.length; i++)
			{
				tool.script.g.add_prop(props[i]);
			}
		}
		else
		{
			for(uint i = 0; i < props.length; i++)
			{
				tool.script.g.remove_prop(props[i]);
			}
		}
	}
	
}