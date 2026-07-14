class UndoEmitterSize : callback_base
{
	
	private AdvToolScript@ script;
	emitter@ em;
	float x_previous, y_previous;
	float x_next, y_next;
	float width_previous, height_previous;
	float width_next, height_next;
	
	UndoEmitterSize(AdvToolScript@ script, emitter@ em,
		float x_previous, float y_previous,
		float width_previous, float height_previous,
		float x_next, float y_next,
		float width_next, float height_next)
	{
		@this.script = script;
		@this.em = em;
		this.x_previous = x_previous;
		this.y_previous = y_previous;
		this.x_next = x_next;
		this.y_next = y_next;
		this.width_previous = width_previous;
		this.height_previous = height_previous;
		this.width_next = width_next;
		this.height_next = height_next;
	}
	
	void undo()
	{
		em.set_xy(x_previous, y_previous);
		em.size(width_previous, height_previous);
	}
	
	void redo()
	{
		em.set_xy(x_next, y_next);
		em.size(width_next, height_next);
	}
	
}
