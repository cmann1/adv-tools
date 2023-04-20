class TriggerHandlerData
{
	
	entity@ trigger;
	entity@ restore_data;
	string trigger_type;
	
	varstruct@ vars;
	
	TriggerHandlerData(entity@ trigger)
	{
		@this.trigger = trigger;
		trigger_type = trigger.type_name();
		
		@vars = trigger.vars();
	}
	
	protected void read_vars()
	{
		
	}
	
	void store_all()
	{
		if(@trigger == null)
			return;
		
		if(@restore_data == null)
		{
			@restore_data = create_entity(trigger_type);
		}
		
		copy_vars(trigger, restore_data);
	}
	
	void restore_all()
	{
		if(@restore_data == null)
			return;
		
		copy_vars(restore_data, trigger);
		read_vars();
	}
	
}
