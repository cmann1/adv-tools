class UndoEmitterId : callback_base
{
	
	array<emitter@> entities;
	array<uint> ids_previous;
	array<uint> ids_next;
	
	UndoEmitterId() {}
	
	void add(emitter@ em, uint id_previous, uint id_next)
	{
		entities.insertLast(em);
		ids_previous.insertLast(id_previous);
		ids_next.insertLast(id_next);
	}
	
	void undo()
	{
		for(uint i = 0; i < entities.length; i++)
		{
			entities[i].emitter_id(ids_previous[i]);
		}
	}
	
	void redo()
	{
		for(uint i = 0; i < entities.length; i++)
		{
			entities[i].emitter_id(ids_next[i]);
		}
	}
	
}
