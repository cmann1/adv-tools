class UndoEntityAdd : callback_base
{
	
	private AdvToolScript@ script;
	array<entity@> entities;
	bool add;
	
	UndoEntityAdd(AdvToolScript@ script, bool add = true, entity@ entity = null)
	{
		@this.script = script;
		this.add = add;
		
		if (@entity != null)
		{
			entities.insertLast(entity);
		}
	}
	
	void add_entity(entity@ entity)
	{
		if (@entity != null)
		{
			entities.insertLast(entity);
		}
	}
	
	void undo()
	{
		for(uint i = 0; i < entities.length; i++)
		{
			if(add)
				script.g.remove_entity(entities[i]);
			else
				script.g.add_entity(entities[i]);
		}
	}
	
	void redo()
	{
		for(uint i = 0; i < entities.length; i++)
		{
			if(add)
				script.g.add_entity(entities[i]);
			else
				script.g.remove_entity(entities[i]);
		}
	}
	
}
