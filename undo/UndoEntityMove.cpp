class UndoEntityMove : callback_base
{
	
	private AdvToolScript@ script;
	array<entity@> entities;
	array<UndoEntityMoveData> previous;
	array<UndoEntityMoveData> next;
	
	UndoEntityMove(AdvToolScript@ script)
	{
		@this.script = script;
	}
	
	void add(entity@ e, float x_previous, float y_previous, float x, float y)
	{
		entities.insertLast(e);
		previous.insertLast(UndoEntityMoveData(x_previous, y_previous));
		next.insertLast(UndoEntityMoveData(x, y));
	}
	
	void update(int i, float x, float y)
	{
		UndoEntityMoveData@ data = @next[i];
		data.x = x;
		data.y = y;
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

class UndoEntityMoveData
{
	float x, y;
	
	UndoEntityMoveData() {}
	
	UndoEntityMoveData(float x, float y)
	{
		this.x = x;
		this.y = y;
	}
	
	void set(entity@ e)
	{
		e.set_xy(x, y);
	}
}
