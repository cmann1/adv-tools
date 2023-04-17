#include '../../../lib/enums/VK.cpp';
#include '../../../lib/input/ModifierKey.cpp';
#include '../../../lib/utils/vk_from_name.cpp';

/// Special "VK" for mouse wheel up.
const int VK_MouseWheelUp	= 0xff01;
/// Special "VK" for mouse wheel down.
const int VK_MouseWheelDown	= 0xff02;

class ShortcutKey
{
	
	AdvToolScript@ script;
	
	int modifiers = ModifierKey::None;
	int vk = -1;
	int priority = 0;
	
	ShortcutKey@ init(AdvToolScript@ script)
	{
		@this.script = script;
		return this;
	}
	
	private void _set(const int vk, const int modifiers, const int priority)
	{
		this.vk = vk;
		this.modifiers = modifiers;
		this.priority = priority;
	}
	
	ShortcutKey@ set(const int vk, const int modifiers=ModifierKey::None, const int priority=0)
	{
		_set(vk, modifiers, priority);
		return this;
	}
	
	ShortcutKey@ from_string(const string &in str, const int priority=0)
	{
		_set(-1, ModifierKey::None, priority);
		
		// Parse key string in the format MODIFIER+KEY
		array<string> parts = str.split('+');
		
		string key_str = string::trim(parts[int(parts.length) - 1]);
		if(key_str == '')
			return this;
		
		vk = name_to_vk(key_str);
		
		// The key itself must not be a modifier
		if(
			vk <= 0 ||
			(vk >= VK::Shift && vk <= VK::Alt) ||
			(vk >= VK::LeftShift && vk <= VK::RightMenu))
		{
			vk = -1;
			return this;
		}
		
		// Parse modifier keys
		for(int i = 0, length = int(parts.length) - 1; i < length; i++)
		{
			key_str = string::trim(parts[i]);
			if(key_str == '')
				continue;
			
			const int mod_vk = VK::from_name(key_str);
			
			switch(mod_vk)
			{
				case VK::Control: modifiers |= ModifierKey::Ctrl; break;
				case VK::Shift: modifiers |= ModifierKey::Shift; break;
				case VK::Alt: modifiers |= ModifierKey::Alt; break;
			}
		}
		
		return this;
	}
	
	ShortcutKey@ from_config(const string &in name, const string &in default_str='', const int priority=0)
	{
		return from_string(script.config.get_string(name, default_str), priority);
	}
	
	void clear()
	{
		vk = 0;
		modifiers = ModifierKey::None;
		priority = 0;
	}
	
	bool is_set() const
	{
		return vk > 0;
	}
	
	bool check_modifiers()
	{
		if(vk <= 0 || @script == null)
			return false;
		
		if(
			script.ctrl.down != ((modifiers & ModifierKey::Ctrl) != 0) ||
			script.shift.down != ((modifiers & ModifierKey::Shift) != 0) ||
			script.alt.down != ((modifiers & ModifierKey::Alt) != 0)
		)
			return false;
		
		return true;
	}
	
	bool check()
	{
		if(!check_modifiers())
			return false;
		
		if(vk == VK_MouseWheelUp)
			return script.mouse.scroll == -1;
		if(vk == VK_MouseWheelDown)
			return script.mouse.scroll == 1;
		
		return script.input.key_check_pressed_vk(vk);
	}
	
	bool down()
	{
		if(!check_modifiers())
			return false;
		
		if(vk == VK_MouseWheelUp)
			return script.mouse.scroll == -1;
		if(vk == VK_MouseWheelDown)
			return script.mouse.scroll == 1;
		
		return script.input.key_check_vk(vk);
	}
	
	bool matches(ShortcutKey@ other)
	{
		if(@other == null || other.vk <= 0 || vk <= 0)
			return false;
		
		return other.vk == vk && other.modifiers == modifiers;
	}
	
	void clear_gvb()
	{
		if(vk == VK_MouseWheelUp)
		{
			script.input.key_clear_gvb(GVB::WheelUp);
			return;
		}
		if(vk == VK_MouseWheelDown)
		{
			script.input.key_clear_gvb(GVB::WheelDown);
			return;
		}
		
		const int gvb = script.input.vk_to_gvb(vk);
		if(gvb != -1)
		{
			script.input.key_clear_gvb(gvb);
		}
	}
	
	int name_to_vk(const string &in name)
	{
		// Nicer names for the mouse button
		if(name == 'LeftClick')
			return VK::LeftButton;
		if(name == 'RightClick')
			return VK::RightButton;
		if(name == 'MiddleClick')
			return VK::MiddleButton;
		
		// Special values for mouse wheels since these don't have vk codes.
		if(name == 'WheelUp')
			return VK_MouseWheelUp;
		if(name == 'WheelDown')
			return VK_MouseWheelDown;
		
		return VK::from_name(name);
	}
	
	string vk_to_name(const int vk)
	{
		// Nicer names for the mouse button
		if(vk == VK::LeftButton)
			return 'LeftClick';
		if(vk == VK::RightButton)
			return 'RightClick';
		if(vk == VK::MiddleButton)
			return 'MiddleClick';
		
		// Special values for mouse wheels since these don't have vk codes.
		if(vk == VK_MouseWheelUp)
			return 'WheelUp';
		if(vk == VK_MouseWheelDown)
			return 'WheelDown';
		
		return VK::to_name(vk);
	}
	
	string to_string()
	{
		string str = vk_to_name(vk);
		
		if(str == '')
			return str;
		
		if((modifiers & ModifierKey::Alt) != 0)
			str = VK::to_name(VK::Alt) + '+' + str;
		if((modifiers & ModifierKey::Shift) != 0)
			str = VK::to_name(VK::Shift) + '+' + str;
		if((modifiers & ModifierKey::Ctrl) != 0)
			str = VK::to_name(VK::Control) + '+' + str;
		
		return str;
	}
	
}
