class ExtendedPropTool : Tool
{
	
	private PropTool@ prop_tool;
	private PropData@ pick_data;
	
	private ShortcutKey pick_key;
	
	ExtendedPropTool(AdvToolScript@ script)
	{
		super(script, 'Props');
		
		init_shortcut_key(VK::Q);
	}
	
	void create(ToolGroup@ group) override
	{
		set_icon('editor', 'propsicon');
	}
	
	void on_init() override
	{
		@prop_tool = cast<PropTool@>(script.get_tool('Prop Tool'));
		
		pick_key.init(script);
		reload_shortcut_key();
	}
	
	bool reload_shortcut_key() override
	{
		pick_key.from_config('KeyPickProp', 'Alt+MiddleClick');
		
		return Tool::reload_shortcut_key();
	}
	
	// //////////////////////////////////////////////////////////
	// Tool Callbacks
	// //////////////////////////////////////////////////////////
	
	protected void step_impl() override
	{
		if(script.mouse_in_scene && !script.space.down && !script.handles.mouse_over && pick_key.down())
		{
			pick_key.clear_gvb();
			@pick_data = null;
			prop_tool.clear_hovered_props();
			prop_tool.pick_props();
			prop_tool.clear_highlighted_props();
			
			if(prop_tool.highlighted_props_list_count > 0)
			{
				@pick_data = @prop_tool.highlighted_props_list[0];
				prop@ p = pick_data.prop;
				script.editor.set_prop(p.prop_set(), p.prop_group(), p.prop_index(), p.palette());
			}
			else
			{
				@pick_data = null;
			}
		}
		else
		{
			@pick_data = null;
		}
	}
	
	protected void draw_impl(const float sub_frame) override
	{
		if(@pick_data != null)
		{
			pick_data.draw(PropToolHighlight::Both);
		}
	}
	
	// //////////////////////////////////////////////////////////
	// Methods
	// //////////////////////////////////////////////////////////
	
}
