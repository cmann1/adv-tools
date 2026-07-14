class UndoPropAdd : callback_base
{
	
	PropTool@ tool;
	private bool add;
	array<prop@> props;
	
	UndoPropAdd(PropTool@ tool, bool add, array<PropData@>@ selected_props = null, int selected_props_count = 0)
	{
		@this.tool = tool;
		this.add = add;
		
		if (@selected_props == null)
			return;
		
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
	
	void add_prop(prop@ prop)
	{
		props.insertLast(prop);
	}
	
	void undo()
	{
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
	
	void redo()
	{
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
