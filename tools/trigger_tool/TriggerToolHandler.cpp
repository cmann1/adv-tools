class TriggerToolHandler
{
	
	AdvToolScript@ script;
	ExtendedTriggerTool@ tool;
	
	TriggerToolHandler(AdvToolScript@ script, ExtendedTriggerTool@ tool)
	{
		@this.script = script;
		@this.tool = tool;
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
	
	// //////////////////////////////////////////////////////////
	// Methods
	// //////////////////////////////////////////////////////////
	
	protected void draw_line_to_ui(const float x, const float y, Element@ element)
	{
		if(@element == null || !element.visible)
			return;
		
		float x1, y1, x2, y2;
		script.world_to_hud(x, y, x1, y1, false);
		
		if(x1 < element.x1 || x1 > element.x2 || y1 < element.y1 || y1 > element.y2)
		{
			const float line_width = 2;
			const uint colour = multiply_alpha(script.ui.style.normal_bg_clr, 0.5);
			
			closest_point_to_rect(x1, y1,
				element.x1 + line_width, element.y1 + line_width,
				element.x2 - line_width, element.y2 - line_width,
				x2, y2);
			
			script.ui.style.draw_line(x1, y1, x2, y2, line_width, colour);
		}
	}
	
}
