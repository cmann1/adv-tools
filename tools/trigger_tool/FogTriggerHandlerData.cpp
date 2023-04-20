//#include '../../misc/DataSetMode.cpp';

#include 'TriggerHandlerData.cpp';

class FogTriggerHandlerData : TriggerHandlerData
{
	
	fog_setting@ fog = create_fog_setting(false);
	
	FogTriggerHandlerData(entity@ trigger)
	{
		super(trigger);
		
		read_vars();
	}
	
	protected void read_vars() override
	{
		fog.copy_from(trigger);
	}
	
}
