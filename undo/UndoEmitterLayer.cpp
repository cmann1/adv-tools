class UndoEmitterLayer : callback_base
{
	
	array<emitter@> entities;
	array<UndoEmitterLayerData> previous;
	array<UndoEmitterLayerData> next;
	
	UndoEmitterLayer() { }
	
	void add(emitter@ em, int layer, int sub_layer)
	{
		entities.insertLast(em);
		previous.insertLast(UndoEmitterLayerData(layer, sub_layer));
		next.insertLast(UndoEmitterLayerData(layer, sub_layer));
	}
	
	void update(int i, int layer, int sub_layer)
	{
		UndoEmitterLayerData@ data = @next[i];
		data.layer = layer;
		data.sub_layer = sub_layer;
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

class UndoEmitterLayerData
{
	int layer, sub_layer;
	
	UndoEmitterLayerData() {}
	
	UndoEmitterLayerData(int layer, int sub_layer)
	{
		this.layer = layer;
		this.sub_layer = sub_layer;
	}
	
	void set(emitter@ em)
	{
		em.layer(layer, sub_layer);
	}
}
