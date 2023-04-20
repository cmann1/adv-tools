#include '../../misc/DataSetMode.cpp';

#include 'TriggerHandlerData.cpp';

class TextTriggerHandlerData : TriggerHandlerData
{
	
	bool is_z_trigger;
	
	float base_scale;
	float base_rotation;
	
	varvalue@ text_var;
	varvalue@ hide_var;
	varvalue@ colour_var;
	varvalue@ layer_var;
	varvalue@ sub_layer_var;
	varvalue@ rotation_var;
	varvalue@ scale_var;
	varvalue@ font_var;
	varvalue@ font_size_var;
	
	private bool _hidden;
	private string _text;
	private uint _colour;
	private int _layer;
	private int _sub_layer;
	private int _rotation;
	private float _scale;
	private string _font;
	private int _font_size;
	
	private uint _stored_colour;
	private int _stored_layer;
	private int _stored_sub_layer;
	
	TextTriggerHandlerData(entity@ trigger)
	{
		super(trigger);
		
		is_z_trigger = trigger_type == TextTriggerType::ZTextProp;
		
		@text_var = vars.get_var(is_z_trigger ? 'text' : 'text_string');
		
		if(!is_z_trigger)
		{
			@hide_var = vars.get_var('hide');
		}
		else
		{
			@colour_var = vars.get_var('colour');
			@layer_var = vars.get_var('layer');
			@sub_layer_var = vars.get_var('sublayer');
			@rotation_var = vars.get_var('text_rotation');
			@scale_var = vars.get_var('text_scale');
			@font_var = vars.get_var('font');
			@font_size_var = vars.get_var('font_size');
		}
		
		read_vars();
	}
	
	protected void read_vars() override
	{
		text = text_var.get_string();
		
		if(!is_z_trigger)
		{
			_hidden = hide_var.get_bool();
		}
		else
		{
			_colour = uint(colour_var.get_int32());
			_layer = uint(layer_var.get_int32());
			_sub_layer = uint(sub_layer_var.get_int32());
			_rotation = rotation_var.get_int32();
			_scale = scale_var.get_float();
			_font = font_var.get_string();
			_font_size = font_size_var.get_int32();
		}
	}
	
	bool hidden
	{
		get { return !is_z_trigger ? _hidden : false; }
		set
		{
			if(is_z_trigger)
				return;
			if(_hidden == value)
				return;
			
			_hidden = value;
			hide_var.set_bool(value);
		}
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
		set { set_layer(value, DataSetMode::Set); }
	}
	void set_layer(int layer, const DataSetMode mode)
	{
		if(!is_z_trigger)
			return;
		
		if(mode == DataSetMode::Store)
		{
			_stored_layer = _layer;
		}
		else
		{
			_layer = mode == DataSetMode::Restore ? _stored_layer : layer;
			layer_var.set_int32(_layer);
		}
	}
	
	int sub_layer
	{
		get const { return _sub_layer; }
		set { set_sub_layer(value, DataSetMode::Set); }
	}
	void set_sub_layer(int sub_layer, const DataSetMode mode)
	{
		if(!is_z_trigger)
			return;
		
		if(mode == DataSetMode::Store)
		{
			_stored_sub_layer = _sub_layer;
		}
		else
		{
			_sub_layer = mode == DataSetMode::Restore ? _stored_sub_layer : sub_layer;
			sub_layer_var.set_int32(_sub_layer);
		}
	}
	
	int rotation
	{
		get const { return _rotation; }
		set
		{
			if(!is_z_trigger)
				return;
			if(_rotation == value)
				return;
			
			_rotation = value;
			rotation_var.set_int32(value);
		}
	}
	
	float scale
	{
		get const { return _scale; }
		set
		{
			if(!is_z_trigger)
				return;
			if(_scale == value)
				return;
			
			_scale = value;
			scale_var.set_float(value);
		}
	}
	
	string font
	{
		get const { return _font; }
		set
		{
			if(!is_z_trigger)
				return;
			if(_font == value)
				return;
			
			_font = value;
			font_var.set_string(value);
		}
	}
	
	int font_size
	{
		get const { return _font_size; }
		set
		{
			if(!is_z_trigger)
				return;
			if(_font_size == value)
				return;
			
			_font_size = value;
			font_size_var.set_int32(value);
		}
	}
	
}
