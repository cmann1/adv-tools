#include '../../../lib/utils/chr.cpp';
#include '../../../lib/utils/vk_from_name.cpp';
#include '../../../lib/string.cpp';

#include 'ConfigState.cpp';
#include 'ShortcutKey.cpp';

class Config
{
	
	bool EnableShortcuts;
	float ToolbarIconSize;
	uint UIIconColour;
	ShortcutKey KeyPrevTool;
	ShortcutKey KeyNextTool;
	ShortcutKey KeyToggleUI;
	ShortcutKey KeyTogglePanels;
	ShortcutKey KeyToggleToolbars;
	ShortcutKey KeyPreviewLayer;
	
	private AdvToolScript@ script;
	private dictionary values;
	
	Config(AdvToolScript@ script)
	{
		@this.script = script;
	}
	
	bool load()
	{
		if(!load_embed(Settings::ConfigEmbedKey, Settings::ConfigFile))
		{
			init();
			return false;
		}
		
		values.deleteAll();
		const string data = get_embed_value(Settings::ConfigEmbedKey);
		
		//const uint t = get_time_us();
		
		int state = ConfigState::Start;
		string key = '', value = '';
		
		for(uint i = 0, length = data.length; i < length; i++)
		{
			int chr = data[i];
			
			if(chr == chr::CarriageReturn)
			{
				if(i < length - 1 && data[i + 1] == chr::NewLine)
					continue;
				
				chr = chr::NewLine;
			}
			
			switch(state)
			{
				case ConfigState::Start:
				{
					switch(chr)
					{
						case chr::SemiColon:
							state = ConfigState::Comment;
							break;
						case chr::Equals:
							state = ConfigState::Value;
							break;
						case chr::NewLine:
							break;
						default:
							key = string::chr(chr);
							state = ConfigState::Key;
					}
					break;
				}
				case ConfigState::Comment:
				{
					if(chr == chr::NewLine)
					{
						state = ConfigState::Start;
					}
					break;
				}
				case ConfigState::Key:
				{
					switch(chr)
					{
						case chr::NewLine:
							key = '';
							state = ConfigState::Start;
							break;
						case chr::Equals:
							state = ConfigState::Value;
							break;
						default:
							key += string::chr(chr);
							break;
					}
					break;
				}
				case ConfigState::Value:
				{
					switch(chr)
					{
						case chr::NewLine:
							add(key, value);
							key = '';
							value = '';
							state = ConfigState::Start;
							break;
						default:
							value += string::chr(chr);
							break;
					}
					break;
				}
			}
		}
		
		add(key, value);
		//puts((get_time_us() - t)/1000+'ms');
		
		init();
		return true;
	}
	
	private void init()
	{
		EnableShortcuts = get_bool('EnableShortcuts', true);
		ToolbarIconSize = round(get_float('ToolbarIconSize', 30));
		UIIconColour = get_colour('UIIconColour', 0xffffffff);
		KeyPrevTool.init(script).from_config('KeyPrevTool', 'Shift+W');
		KeyNextTool.init(script).from_config('KeyNextTool', 'Shift+E');
		KeyToggleUI.init(script).from_config('KeyToggleUI', 'BackSlash');
		KeyTogglePanels.init(script).from_config('KeyTogglePanels');
		KeyToggleToolbars.init(script).from_config('KeyToggleToolbars');
		if(!KeyToggleToolbars.is_set())
		{
			KeyToggleToolbars.set(KeyToggleUI.vk, KeyToggleUI.modifiers | ModifierKey::Shift, KeyToggleToolbars.priority);
		}
		if(!KeyTogglePanels.is_set())
		{
			KeyTogglePanels.set(KeyToggleUI.vk, KeyToggleUI.modifiers | ModifierKey::Alt, KeyTogglePanels.priority);
		}
		KeyPreviewLayer.init(script).from_config('KeyPreviewLayer');
	}
	
	private void add(string &in key, const string &in value)
	{
		key = string::trim(key);
		if(key == '')
			return;
		
		values[key] = value;
	}
	
	bool has_value(const string &in name, const bool ignore_empty=true)
	{
		if(!values.exists(name))
			return false;
		
		if(!ignore_empty)
			return true;
		
		return string::trim(string(values[name])) != '';
	}
	
	string get_string(const string &in name, const string &in default_value='')
	{
		if(!values.exists(name))
			return default_value;
		
		return string(values[name]);
	}
	
	bool get_bool(const string &in name, const bool default_value=false)
	{
		if(!values.exists(name))
			return default_value;
		
		return string(values[name]) == 'true';
	}
	
	float get_float(const string &in name, const float default_value=0)
	{
		if(!values.exists(name))
			return default_value;
		
		return parseFloat(string(values[name]));
	}
	
	int get_vk(const string &in name, const int default_value=-1)
	{
		if(!values.exists(name))
			return default_value;
		
		const string key_name = string::trim(string(values[name]));
		return key_name != '' ? VK::from_name(key_name) : -1;
	}
	
	uint get_colour(const string &in name, const uint default_value=0x00000000)
	{
		if(!values.exists(name))
			return default_value;
		
		return string::try_parse_rgb(string(values[name]), true, true);
	}
	
	bool compare_float(const string &in name, const float value)
	{
		return !values.exists(name) || get_float(name, value) == value;
	}
	
	bool compare_colour(const string &in name, const uint value)
	{
		if(!has_value(name))
			return true;
		
		return get_colour(name, value) == value;
	}
	
}
