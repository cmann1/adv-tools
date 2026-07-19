class UndoTile : callback_base
{
	
	private AdvToolScript@ script;
	int layer;
	array<int> tile_x;
	array<int> tile_y;
	array<tileinfo@> previous;
	array<tileinfo@> next;
	
	UndoTile(AdvToolScript@ script, int layer)
	{
		@this.script = script;
		this.layer = layer;
	}
	
	bool is_empty()
	{
		return tile_x.length == 0;
	}
	
	void add(int x, int y, tileinfo@ previous, tileinfo@ next)
	{
		tile_x.insertLast(x);
		tile_y.insertLast(y);
		this.previous.insertLast(previous);
		this.next.insertLast(next);
	}
	
	void add_or_update(int x, int y, tileinfo@ previous, tileinfo@ next)
	{
		for(uint i = 0; i < tile_x.length; i++)
		{
			if (tile_x[i] != x || tile_y[i] != y)
				continue;
			
			@this.next[i] = next;
			return;
		}
		add(x, y, previous, next);
	}
	
	void undo()
	{
		for(uint i = 0; i < tile_x.length; i++)
		{
			script.g.set_tile(tile_x[i], tile_y[i], layer, previous[i], false);
		}
	}
	
	void redo()
	{
		for(uint i = 0; i < tile_x.length; i++)
		{
			script.g.set_tile(tile_x[i], tile_y[i], layer, next[i], false);
		}
	}
	
}
