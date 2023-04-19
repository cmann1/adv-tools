class EditingTextTriggerData
{
	
	entity@ trigger;
	entity@ restore_data;
	string trigger_type;
	bool is_z_trigger;
	varstruct@ vars;
	varvalue@ text_var;
	varvalue@ hide_var;
	
	private string _text;
	
	EditingTextTriggerData(entity@ trigger)
	{
		@this.trigger = trigger;
		trigger_type = trigger.type_name();
		
		@restore_data = create_entity(trigger_type);
		copy_vars(trigger, restore_data);
		is_z_trigger = trigger_type == TextTriggerType::ZTextProp;
		
		@vars = trigger.vars();
		@text_var = vars.get_var(is_z_trigger ? 'text' : 'text_string');
		text = text_var.get_string();
		
		if(!is_z_trigger)
		{
			@hide_var = vars.get_var('hide');
		}
	}
	
	string text
	{
		get const { return _text; }
		set
		{
			_text = value;
			text_var.set_string(value);
		}
	}
	
	bool hidden
	{
		get { return @hide_var != null ? hide_var.get_bool() : false; }
		set { if(@hide_var != null) hide_var.set_bool(value); }
	}
	
}
