#include '../../misc/DataSetMode.cpp';

class EditingTextTriggerData
{
	
	entity@ trigger;
	entity@ restore_data;
	string trigger_type;
	bool is_z_trigger;
	
	varstruct@ vars;
	varvalue@ text_var;
	varvalue@ hide_var;
	varvalue@ colour_var;
	varvalue@ layer_var;
	varvalue@ sub_layer_var;
	
	private string _text;
	private uint _colour;
	private int _layer;
	private int _sub_layer;
	
	private uint _stored_colour;
	
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
		else
		{
			@colour_var = vars.get_var('colour');
			_colour = uint(colour_var.get_int32());
			
			@layer_var = vars.get_var('layer');
			_layer = uint(layer_var.get_int32());
			
			@sub_layer_var = vars.get_var('sublayer');
			_sub_layer = uint(sub_layer_var.get_int32());
		}
	}
	
	bool hidden
	{
		get { return @hide_var != null ? hide_var.get_bool() : false; }
		set { if(@hide_var != null) hide_var.set_bool(value); }
	}
	
	string text
	{
		get const { return _text; }
		set
		{
			if(_text == value)
				return;
			
			_text = value;
			text_var.set_string(value);
		}
	}
	
	uint colour
	{
		get const { return _colour; }
		set { set_colour(value, DataSetMode::Set); }
	}
	void set_colour(uint colour, const DataSetMode mode)
	{
		if(!is_z_trigger)
			return;
		
		if(mode == DataSetMode::Store)
		{
			_stored_colour = _colour;
		}
		else
		{
			_colour = mode == DataSetMode::Restore ? _stored_colour : colour;
			colour_var.set_int32(_colour);
		}
	}
	
	int layer
	{
		get const { return _layer; }
		set
		{
			if(_layer == value)
				return;
			
			_layer = value;
			layer_var.set_int32(value);
		}
	}
	
	int sub_layer
	{
		get const { return _sub_layer; }
		set
		{
			if(_sub_layer == value)
				return;
			
			_sub_layer = value;
			sub_layer_var.set_int32(value);
		}
	}
	
	
}
