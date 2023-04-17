#include 'Handle.cpp';

class Handles
{
	
	private AdvToolScript@ script;
	
	private int handle_pool_size;
	private int handle_pool_count;
	private array<Handle@> handle_pool;
	
	private int handles_size = 32;
	private int handles_count;
	private array<Handle@> handles(handles_size);
	private Handle@ handle;
	
	private bool has_hit_handle;
	
	Line _line;
	
	bool mouse_over;
	bool mouse_over_last_handle;
	
	void init(AdvToolScript@ script)
	{
		@this.script = script;
	}
	
	private void init_handle(
		const HandleShape shape, const float x, const float y, const float size, const float rotation,
		const uint colour, const uint highlight_colour)
	{
		@handle = handle_pool_count > 0
			? handle_pool[--handle_pool_count]
			: Handle(this);
		
		if(handles_count == handles_size)
		{
			handles.resize(handles_size += 32);
		}
		
		@handles[handles_count++] = handle;
		handle.init(shape, x, y, size, rotation, colour, highlight_colour);
		mouse_over_last_handle = false;
	}
	
	private bool test_handle(const bool force_highlight)
	{
		if(script.mouse_in_scene && handle.hit_test(script, script.mouse.x, script.mouse.y))
		{
			mouse_over = true;
			mouse_over_last_handle = true;
			
			if(has_hit_handle)
			{
				handle.hit = force_highlight;
				return false;
			}
			
			has_hit_handle = true;
			return script.mouse.left_press && !script.space.down;
		}
		
		handle.hit = force_highlight;
		return false;
	}
	
	bool circle(const float x, const float y, const float size, const uint colour, const uint highlight_colour, const bool force_highlight=false)
	{
		init_handle(HandleShape::Circle, x, y, size, 0, colour, highlight_colour);
		return test_handle(force_highlight);
	}
	
	bool square(const float x, const float y, const float size, const float rotation, const uint colour, const uint highlight_colour, const bool force_highlight=false)
	{
		init_handle(HandleShape::Square, x, y, size, rotation, colour, highlight_colour);
		return test_handle(force_highlight);
	}
	
	bool line(
		const float x1, const float y1, const float x2, const float y2, const float size,
		const uint colour, const uint highlight_colour, const bool force_highlight=false)
	{
		init_handle(HandleShape::Line, x1, y1, size, 0, colour, highlight_colour);
		handle.x2 = x2;
		handle.y2 = y2;
		return test_handle(force_highlight);
	}
	
	Handle@ get_last_handle()
	{
		return handles_count > 0
			? handles[handles_count - 1]
			: null;
	}
	
	void step()
	{
		if(handle_pool_count + handles_count >= handle_pool_size)
		{
			handle_pool.resize(handle_pool_size = handle_pool_count + handles_count + 32);
		}
		
		for(int i = 0; i < handles_count; i++)
		{
			@handle_pool[handle_pool_count++] = @handles[i];
		}
		
		handles_count = 0;
		mouse_over = false;
		has_hit_handle = false;
	}
	
	void draw()
	{
		for(int i = handles_count - 1; i >= 0; i--)
		{
			handles[i].draw(script);
		}
	}
	
}
