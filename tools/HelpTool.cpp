class HelpTool : Tool
{
	
	HelpTool(AdvToolScript@ script, const string name)
	{
		super(script, name);
		selectable = false;
	}
	
	// //////////////////////////////////////////////////////////
	// Events
	// //////////////////////////////////////////////////////////
	
	bool on_before_select() override
	{
		if(script.shift.down)
		{
			puts('> Reloading AdvTools config');
			script.reload_config();
			return false;
		}
		
		script.editor.help_screen_vis(!script.editor.help_screen_vis());
		return false;
	}
	
}
