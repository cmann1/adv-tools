#include 'HandleShape.cpp';

class Handle
{
	
	Handles@ handles;
	HandleShape shape;
	float x, y;
	float x2, y2;
	float size;
	float rotation;
	uint colour;
	uint highlight_colour;
	
	bool hit;
	
	Handle(Handles@ handles)
	{
		@this.handles = handles;
	}
	
	void init(const HandleShape shape, const float x, const float y, const float size, const float rotation, const uint colour, const uint highlight_colour)
	{
		this.shape = shape;
		this.x = x;
		this.y = y;
		this.size = size;
		this.rotation = rotation;
		this.colour = colour;
		this.highlight_colour = highlight_colour;
		
		hit = false;
	}
	
	bool hit_test(AdvToolScript@ script, float px, float py)
	{
		const float size = this.size / script.zoom;
		
		switch(shape)
		{
			case HandleShape::Square:
			{
				px -= x;
				py -= y;
				
				if(rotation != 0)
				{
					rotate(px, py, -rotation * DEG2RAD, px, py);
				}
				
				hit = px >= -size && px <= size && py >= -size && py <= size;
			}
				break;
			case HandleShape::Circle:
			{
				const float dx = px - x;
				const float dy = py - y;
				hit = (dx * dx + dy * dy) <= (size * size);
			}
				break;
			case HandleShape::Line:
			{
				handles._line.x1 = x;
				handles._line.y1 = y;
				handles._line.x2 = x2;
				handles._line.y2 = y2;
				const float dist = handles._line.distance_squared(px, py);
				hit = dist <= size * size;
			}
				break;
		}
		
		return hit;
	}
	
	void draw(AdvToolScript@ script)
	{
		const float size = this.size / script.zoom;
		const float shadow_outset = 2 / script.zoom;
		
		switch(shape)
		{
			case HandleShape::Square:
			{
				script.g.draw_rectangle_world(22, 22, x - size - shadow_outset, y - size - shadow_outset, x + size + shadow_outset, y + size + shadow_outset, rotation, 0x44000000);
				script.g.draw_rectangle_world(22, 22, x - size, y - size, x + size, y + size, rotation, hit ? highlight_colour : colour);
			}
				break;
			case HandleShape::Circle:
			{
				drawing::fill_circle(script.g, 22, 22, x, y, size + shadow_outset, 12, 0x44000000, 0x44000000);
				drawing::fill_circle(script.g, 22, 22, x, y, size, 12, hit ? highlight_colour : colour, hit ? highlight_colour : colour);
			}
				break;
			case HandleShape::Line:
			{
				script.g.draw_line_world(22, 22, x, y, x2, y2, size * 0.5 + shadow_outset, 0x44000000);
				script.g.draw_line_world(22, 22, x, y, x2, y2, size * 0.5, hit ? highlight_colour : colour);
			}
				break;
		}
	}
	
}
