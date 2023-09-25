class Tool
{
	
	string name = '';
	string base_tool_name = '';
	string icon_sprite_set;
	string icon_sprite_name;
	float icon_width;
	float icon_height;
	float icon_offset_x;
	float icon_offset_y;
	float icon_rotation;
	
	bool selectable = true;
	
	AdvToolScript@ script;
	ToolGroup@ group;
	Button@ toolbar_button;
	
	array<Tool@>@ shortcut_key_group;
	ShortcutKey key;
	
	/** Setting this to true while a tool is performing an action while prevent other tool shortcut keys from being run. */
	bool active;
	
	protected array<Tool@> sub_tools;
	
	protected bool selected = false;
	
	private string default_key_config = '';
	private int default_key=-1;
	private int default_key_modifiers=ModifierKey::None;
	
	protected void construct(AdvToolScript@ script, const string & in base_tool_name, const string &in name)
	{
		@this.script = script;
		this.base_tool_name = base_tool_name;
		this.name = name;
		
		key.init(script);
	}
	
	Tool(AdvToolScript@ script)
	{
		construct(script, '', '');
	}
	
	Tool(AdvToolScript@ script, const string name)
	{
		construct(script, name, name);
	}
	
	Tool(AdvToolScript@ script, const string &in base_tool_name, const string &in name)
	{
		construct(script, base_tool_name, name);
	}
	
	/// The tool icon must be initialised during create to be properly added to the tool group menu.
	void create(ToolGroup@ group)
	{
	}
	
	/// Initialisation that may depend on other tools.
	void on_init()
	{
		
	}
	
	void build_sprites(message@ msg)
	{
		
	}
	
	Tool@ set_icon(
		const string sprite_set, const string sprite_name, const float width=-1, const float height=-1,
		const float offset_x=0, const float offset_y=0, const float rotation=0)
	{
		this.icon_sprite_set = sprite_set;
		this.icon_sprite_name = sprite_name;
		this.icon_width = width;
		this.icon_height = height;
		this.icon_offset_x = offset_x;
		this.icon_offset_y = offset_y;
		this.icon_rotation = rotation;
		
		return this;
	}
	
	Tool@ init_shortcut_key(
		const string &in config_name, const int default_key=-1,
		const int default_modifiers=ModifierKey::None, const int priority=0)
	{
		this.default_key_config = config_name;
		this.default_key = default_key;
		this.default_key_modifiers = default_modifiers;
		
		// First check this tool's name (with spaces stripped) for a key config
		key.from_config('Key' + string::replace(name, ' ', ''), '', priority);
		
		// Use the provided default
		if(key.vk <= 0)
		{
			key.set(default_key, default_modifiers, priority);
		}
		
		// If there isn't a config or default, check the parent tool config
		if(key.vk <= 0)
		{
			key.from_config('Key' + config_name);
		}
		
		return @this;
	}
	
	Tool@ init_shortcut_key(const int shortcut_key, const int modifiers=ModifierKey::None, const int priority=0)
	{
		return init_shortcut_key(base_tool_name, shortcut_key, modifiers, priority);
	}
	
	bool reload_shortcut_key()
	{
		const int prev_vk = key.vk;
		const int prev_mod = key.modifiers;
		init_shortcut_key(default_key_config, default_key, default_key_modifiers, key.priority);
		
		return prev_vk != key.vk || prev_mod != key.modifiers;
	}
	
	// //////////////////////////////////////////////////////////
	// Methods
	// //////////////////////////////////////////////////////////
	
	void register_sub_tool(Tool@ tool)
	{
		if(@tool == null || @tool == @this)
			return;
		
		const int index = sub_tools.findByRef(tool);
		if(index < 0)
		{
			sub_tools.insertLast(tool);
		}
	}
	
	// //////////////////////////////////////////////////////////
	// Callbacks
	// //////////////////////////////////////////////////////////
	
	void on_editor_loaded() final
	{
		on_editor_loaded_impl();
		
		for(uint i = 0; i < sub_tools.length; i++)
		{
			sub_tools[i].on_editor_loaded_impl();
		}
	}

	void on_editor_unloaded() final
	{
		on_editor_unloaded_impl();
		
		for(uint i = 0; i < sub_tools.length; i++)
		{
			sub_tools[i].on_editor_unloaded_impl();
		}
	}
	
	protected void on_editor_loaded_impl()
	{
		
	}
	
	protected void on_editor_unloaded_impl()
	{
		
	}
	
	Tool@ on_shortcut_key()
	{
		if(@shortcut_key_group == null)
			return this;
		
		Tool@ tool = this;
		
		// Cycle through tools with the same shortcut key
		for(uint i = 0, length = shortcut_key_group.length; i < length; i++)
		{
			Tool@ t = shortcut_key_group[i];
			
			if(t.selected)
			{
				@tool = shortcut_key_group[((int(i) - 1) % length + length) % length];
				break;
			}
		}
		
		return tool;
	}
	
	void on_reselect()
	{
		
	}
	
	bool on_before_select()
	{
		return true;
	}
	
	void on_select() final
	{
		selected = true;
		active = false;
		
		if(@group != null)
		{
			group.set_tool(this);
		}
		
		if(@toolbar_button != null)
		{
			toolbar_button.override_alpha = 1;
		}
		
		on_select_impl();
		
		for(uint i = 0; i < sub_tools.length; i++)
		{
			sub_tools[i].on_select_impl();
		}
	}
	
	void on_deselect() final
	{
		selected = false;
		active = false;
		
		if(@toolbar_button != null)
		{
			toolbar_button.override_alpha = -1;
		}
		
		on_deselect_impl();
		
		for(uint i = 0; i < sub_tools.length; i++)
		{
			sub_tools[i].on_deselect_impl();
		}
	}
	
	protected void on_select_impl()
	{
		
	}
	
	protected void on_deselect_impl()
	{
		
	}
	
	void step() final
	{
		step_impl();
		
		for(uint i = 0; i < sub_tools.length; i++)
		{
			sub_tools[i].step_impl();
		}
	}
	
	void draw(const float sub_frame) final
	{
		draw_impl(sub_frame);
		
		for(uint i = 0; i < sub_tools.length; i++)
		{
			sub_tools[i].draw_impl(sub_frame);
		}
	}
	
	void on_settings_loaded()
	{
		
	}
	
	protected void step_impl()
	{
		
	}
	
	protected void draw_impl(const float sub_frame)
	{
		
	}
	
}
