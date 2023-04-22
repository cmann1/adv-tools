#include 'TriggerHandlerData.cpp';

class FogTriggerHandlerData : TriggerHandlerData
{
	
	fog_setting@ fog = create_fog_setting(false);
	fog_setting@ edit_fog;
	
	bool has_sub_layers;
	
	FogTriggerHandlerData(entity@ trigger)
	{
		super(trigger);
		
		read_vars();
	}
	
	/**
	 * @brief Read from the fog trigger when starting editing instead of when being selected since
	 *        the user can change the fog settings while the trigger is selected.
	 */
	protected void read_vars() override
	{
		has_sub_layers = vars.get_var('has_sub_layers').get_bool();
	}
	
	void store_all() override
	{
		if(@edit_fog == null)
		{
			@edit_fog = create_fog_setting(fog.has_sub_layers());
		}
		
		fog.copy_from(trigger);
		has_sub_layers = vars.get_var('has_sub_layers').get_bool();
		edit_fog.copy_from(fog);
		
		TriggerHandlerData::store_all();
	}
	
}
