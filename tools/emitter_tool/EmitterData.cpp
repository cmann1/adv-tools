#include '../../misc/SelectableData.cpp';

class EmitterData : SelectableData
{
	
	EmitterTool@ tool;
	entity@ e;
	emitter@ em;
	bool visible;
	bool mouse_over_handle;
	
	int emitter_id;
	int layer, sub_layer;
	float x, y;
	float width, height;
	float rotation;
	int layer_position;
	
	private float world_size_x, world_size_y;
	private float rect_x1, rect_y1;
	private float rect_x2, rect_y2;
	
	float drag_start_x, drag_start_y;
	float drag_start_width, drag_start_height;
	
	void init(AdvToolScript@ script, EmitterTool@ tool, entity@ e, emitter@ em, const int scene_index)
	{
		SelectableData::init(script, em.id() + '', scene_index);
		
		@this.tool = tool;
		@this.e = e;
		@this.em = em;
		
		emitter_id = em.emitter_id();
		layer = em.layer();
		sub_layer = em.sub_layer();
		em.get_size(width, height);
		rotation = em.rotation();
		layer_position = tool.script.layer_position(layer);
		
		mouse_over_handle = false;
	}
	
	void update()
	{
		x = em.x();
		y = em.y();
		em.get_size(width, height);
		rotation = em.rotation();
		
		script.transform(x, y, layer, sub_layer, 22, 22, aabb_x, aabb_y);
		script.transform_size(width, height, layer, sub_layer, 22, 22, world_size_x, world_size_y);
		world_size_x *= 0.5;
		world_size_y *= 0.5;
		
		float rel_mouse_x, rel_mouse_y;
		rotate(
			script.mouse.x, script.mouse.y, aabb_x, aabb_y,
			-rotation * DEG2RAD, rel_mouse_x, rel_mouse_y);
		
		rect_x1 = aabb_x - world_size_x;
		rect_y1 = aabb_y - world_size_y;
		rect_x2 = aabb_x + world_size_x;
		rect_y2 = aabb_y + world_size_y;
		
		is_mouse_inside = 
			rel_mouse_x >= rect_x1 && rel_mouse_x <= rect_x2 &&
			rel_mouse_y >= rect_y1 && rel_mouse_y <= rect_y2
				? 1 : 0;
		
		aabb_from_rect(world_size_x, world_size_y, rotation * DEG2RAD);
	}
	
	bool post_step_validate()
	{
		if(em.destroyed())
			return false;
		
		return true;
	}
	
	DragHandleType do_handles(DragHandleType current_handle=DragHandleType::None)
	{
		mouse_over_handle = false;
		
		DragHandleType handle = DragHandleType::Right;
		DragHandleType dragged_handle = current_handle;
		
		const float nx = cos(rotation * DEG2RAD);
		const float ny = sin(rotation * DEG2RAD);
		
		const array<float>@ o = Settings::ScaleHandleOffsets;
		
		for(int i = 0; i < 32; i += 4)
		{
			if(script.handles.square(
				aabb_x + nx * world_size_x * o[i + 0] - ny * world_size_y * o[i + 1],
				aabb_y + ny * world_size_x * o[i + 2] + nx * world_size_y * o[i + 3],
				Settings::ScaleHandleSize, rotation,
				Settings::RotateHandleColour, Settings::RotateHandleHoveredColour,
				dragged_handle == handle) && dragged_handle == DragHandleType::None)
			{
				dragged_handle = handle;
			}
			
			if(script.handles.mouse_over_last_handle)
			{
				hovered = true;
				mouse_over_handle = true;
			}
			
			handle++;
		}
		
		if(script.handles.circle(
			aabb_x + ny * (world_size_y + Settings::RotationHandleOffset / script.zoom),
			aabb_y - nx * (world_size_y + Settings::RotationHandleOffset / script.zoom),
			Settings::RotateHandleSize,
			Settings::RotateHandleColour, Settings::RotateHandleHoveredColour, dragged_handle == DragHandleType::Rotate) && dragged_handle == DragHandleType::None)
		{
			dragged_handle = DragHandleType::Rotate;
		}
		
		if(script.handles.mouse_over_last_handle)
		{
			hovered = true;
			mouse_over_handle = true;
		}
		
		return dragged_handle;
	}
	
	void draw()
	{
		float line_width;
		uint line_colour, fill_colour;
		get_colours(line_width, line_colour, fill_colour);
		
		if(fill_colour != 0)
		{
			script.g.draw_rectangle_world(22, 22,
				aabb_x - world_size_x, aabb_y - world_size_y,
				aabb_x + world_size_x, aabb_y + world_size_y,
				rotation, fill_colour);
		}
		
		if(line_colour != 0)
		{
			outline_rotated_rect(script.g, 22, 22,
				aabb_x, aabb_y, world_size_x, world_size_y,
				rotation, line_width,
				line_colour);
		}
		
		if(selected)
		{
			const float size = Settings::SelectDiamondSize / script.zoom;
			script.g.draw_rectangle_world(22, 22,
				aabb_x - size, aabb_y - size, aabb_x + size, aabb_y + size, 45, line_colour);
		}
		
		if(primary_selected)
		{
			const float nx =  sin(rotation * DEG2RAD);
			const float ny = -cos(rotation * DEG2RAD);
			
			script.g.draw_line_world(22, 22,
				aabb_x + nx * (world_size_y),
				aabb_y + ny * (world_size_y),
				aabb_x + nx * (world_size_y + Settings::RotationHandleOffset / script.zoom),
				aabb_y + ny * (world_size_y + Settings::RotationHandleOffset / script.zoom),
				Settings::BoundingBoxLineWidth / script.zoom, Settings::BoundingBoxColour);
		}
	}
	
	int opCmp(const EmitterData &in other)
	{
		if(selected != other.selected)
			return selected ? 1 : -1;
		
		// Emitters that the mouse is inside of take priority over ones that the mouse is close to
		
		if(is_mouse_inside != other.is_mouse_inside)
			return is_mouse_inside - other.is_mouse_inside;
		
		// Compare layers
		
		if(layer_position != other.layer_position)
			return layer_position - other.layer_position;
		
		if(sub_layer != other.sub_layer)
			return sub_layer - other.sub_layer;
		
		return scene_index - other.scene_index;
	}
	
	void get_handle_position(const DragHandleType handle, float &out x, float &out y)
	{
		float ix, iy;
		
		switch(handle)
		{
			case DragHandleType::BottomRight:
			case DragHandleType::TopRight:
			case DragHandleType::Right:
				ix = 1;
				break;
			case DragHandleType::BottomLeft:
			case DragHandleType::TopLeft:
			case DragHandleType::Left:
				ix = -1;
				break;
			default:
				ix = 0;
				break;
		}
		
		switch(handle)
		{
			case DragHandleType::Bottom:
			case DragHandleType::BottomLeft:
			case DragHandleType::BottomRight:
				iy = 1;
				break;
			case DragHandleType::Top:
			case DragHandleType::TopLeft:
			case DragHandleType::TopRight:
				iy = -1;
				break;
			default:
				iy = 0;
				break;
		}
		
		const float nx = cos(rotation * DEG2RAD);
		const float ny = sin(rotation * DEG2RAD);
		
		x = nx * width * 0.5 * ix - ny * height * 0.5 * iy;
		y = ny * width * 0.5 * ix + nx * height * 0.5 * iy;
		
		x += this.x;
		y += this.y;
	}
	
	bool intersects_aabb(float x1, float y1, float x2, float y2)
	{
		float rx, ry;
		
		// The aabb is contained within the emitter
		rotate(x1 - aabb_x, y1 - aabb_y, -rotation * DEG2RAD, rx, ry);
		if(rx <= world_size_x && rx >= -world_size_x && ry <= world_size_y && ry >= -world_size_y)
			return true;
		rotate(x2 - aabb_x, y1 - aabb_y, -rotation * DEG2RAD, rx, ry);
		if(rx <= world_size_x && rx >= -world_size_x && ry <= world_size_y && ry >= -world_size_y)
			return true;
		rotate(x2 - aabb_x, y2 - aabb_y, -rotation * DEG2RAD, rx, ry);
		if(rx <= world_size_x && rx >= -world_size_x && ry <= world_size_y && ry >= -world_size_y)
			return true;
		rotate(x1 - aabb_x, y2 - aabb_y, -rotation * DEG2RAD, rx, ry);
		if(rx <= world_size_x && rx >= -world_size_x && ry <= world_size_y && ry >= -world_size_y)
			return true;
		
		// One of the emitter "edges" intersect the aabb
		float wx1, wy1, wx2, wy2, wx3, wy3, wx4, wy4;
		calculate_rotated_rectangle(
			aabb_x, aabb_y, world_size_x, world_size_y, rotation,
			wx1, wy1, wx2, wy2, wx3, wy3, wx4, wy4);
		
		tool._line.set(wx1, wy1, wx2, wy2);
		if(tool._line.aabb_intersection(x1, y1, x2, y2, rx, ry))
			return true;
		tool._line.set(wx2, wy2, wx3, wy3);
		if(tool._line.aabb_intersection(x1, y1, x2, y2, rx, ry))
			return true;
		tool._line.set(wx3, wy3, wx4, wy4);
		if(tool._line.aabb_intersection(x1, y1, x2, y2, rx, ry))
			return true;
		tool._line.set(wx4, wy4, wx1, wy1);
		if(tool._line.aabb_intersection(x1, y1, x2, y2, rx, ry))
			return true;
		
		return false;
	}
	
	// Moving
	
	void start_drag()
	{
		drag_start_x = x;
		drag_start_y = y;
	}
	
	void do_drag(const float drag_delta_x, const float drag_delta_y)
	{
		x = drag_start_x + drag_delta_x;
		y = drag_start_y + drag_delta_y;
		
		em.set_xy(x, y);
	}
	
	void stop_drag(const bool accept)
	{
		if(!accept)
		{
			x = drag_start_x;
			y = drag_start_y;
			
			em.set_xy(x, y);
		}
	}
	
	void move(const float dx, const float dy)
	{
		x += dx;
		y += dy;
		
		em.set_xy(x, y);
	}
	
	// Layer/Sublayer
	
	void shift_layer(const int dir, const bool sub_layer=false)
	{
		if(sub_layer)
		{
			this.sub_layer = clamp(this.sub_layer + dir, 0, 24);
			em.sub_layer(this.sub_layer);
		}
		else
		{
			layer = clamp(layer + dir, 0, 20);
			em.layer(layer);
		}
		
		update();
	}
	
	// Scale
	
	void start_scale()
	{
		drag_start_width  = width;
		drag_start_height = height;
		
		start_drag();
	}
	
	void do_scale(const float w, const float h, float anchor_x, float anchor_y)
	{
		width  = abs(w);
		height = abs(h);
		
		em.size(ceil_int(width), ceil_int(height));
		
		x = anchor_x;
		y = anchor_y;
		
		em.set_xy(x, y);
		
		update();
	}
	
	void stop_scale(const bool cancel)
	{
		if(cancel)
		{
			width  = drag_start_width;
			height = drag_start_height;
			em.size(round_int(width), round_int(height));
			stop_drag(false);
		}
		
		update();
	}
	
	// Rotate
	
	void start_rotate()
	{
		drag_start_x = x;
		drag_start_y = y;
		drag_start_width = rotation;
	}
	
	void do_rotate(const float rotation)
	{
		this.rotation = rotation;
		em.rotation(round_int(this.rotation));
		
		update();
	}
	
	void stop_rotate(const bool cancel)
	{
		if(cancel)
		{
			x = drag_start_x;
			y = drag_start_y;
			rotation = drag_start_width;
			em.rotation(round_int(rotation));
			em.set_xy(x, y);
		}
		
		update();
	}
	
	// Property setters
	
	void update_emitter_id(const int id)
	{
		emitter_id = id;
		em.emitter_id(emitter_id);
		
		update();
	}
	
	void update_rotation(const float rotation)
	{
		this.rotation = rotation;
		em.rotation(round_int(rotation));
		
		update();
	}
	
	void update_layer(const int layer, const int sub_layer)
	{
		this.layer = clamp(layer, 0, 20);
		em.layer(this.layer);
		this.sub_layer = clamp(sub_layer, 0, 24);
		em.sub_layer(this.sub_layer);
		
		update();
	}
	
	void update_layer(const int layer)
	{
		this.layer = clamp(layer, 0, 20);
		em.layer(this.layer);
		
		update();
	}
	
	void update_sub_layer(const int sub_layer)
	{
		this.sub_layer = clamp(sub_layer, 0, 24);
		em.sub_layer(this.sub_layer);
		
		update();
	}
	
	// IWorldBoundingBox
	
	void get_bounding_box_world(float &out x1, float &out y1, float &out x2, float &out y2) override
	{
		script.transform(aabb_x + aabb_x1, aabb_y + aabb_y1, layer, sub_layer, 22, 22, x1, y1);
		script.transform(aabb_x + aabb_x2, aabb_y + aabb_y2, layer, sub_layer, 22, 22, x2, y2);
	}
}
