class UndoEntityRotation : callback_base
{
	
	private AdvToolScript@ script;
	array<entity@> entities;
	array<UndoEntityRotationData> previous;
	array<UndoEntityRotationData> next;
	
	UndoEntityRotation(AdvToolScript@ script)
	{
		@this.script = script;
	}
	
	void add(entity@ e, float rotation_previous, float rotation_next,
		float x_previous = NAN, float y_previous = NAN,
		float x_next = NAN, float y_next = NAN)
	{
		entities.insertLast(e);
		previous.insertLast(UndoEntityRotationData(rotation_previous, x_previous, y_previous));
		next.insertLast(UndoEntityRotationData(rotation_next, x_next, y_next));
	}
	
	void undo()
	{
		for(uint i = 0; i < entities.length; i++)
		{
			previous[i].set(entities[i]);
		}
	}
	
	void redo()
	{
		for(uint i = 0; i < entities.length; i++)
		{
			next[i].set(entities[i]);
		}
	}
	
}

class UndoEntityRotationData
{
	float rotation;
	float x, y;
	
	UndoEntityRotationData() {}
	
	UndoEntityRotationData(float rotation, float x, float y)
	{
		this.rotation = rotation;
		this.x = x;
		this.y = y;
	}
	
	void set(entity@ e)
	{
		e.rotation(rotation);
		if(!is_nan(x))
		{
			e.set_xy(x, y);
		}
	}
}
