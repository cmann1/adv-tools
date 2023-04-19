class TriggerToolHandler
{
	
	AdvToolScript@ script;
	
	TriggerToolHandler(AdvToolScript@ script)
	{
		@this.script = script;
	}
	
	void build_sprites(message@ msg) { }
	
	bool should_handle(entity@ trigger, const string &in type)
	{
		return false;
	}
	
	void editor_loaded() {}
	
	void editor_unloaded() {}
	
	void select(entity@ trigger, const string &in type) {}
	
	void deselect() {}
	
	void step() {}
	
	void draw(const float sub_frame) { }
	
}
