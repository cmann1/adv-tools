#include '../../misc/SelectableData.cpp';

class EmitterData : SelectableData
{
	
	EmitterTool@ tool;
	entity@ emitter;
	bool visible;
	bool modified;
	bool mouse_over_handle;
	
	private varstruct@ vars;
	private varvalue@ emitter_id_var;
	private varvalue@ width_var;
	private varvalue@ height_var;
	private varvalue@ rotation_var;
	private varvalue@ sublayer_var;
	
	int emitter_id;
	int layer, sublayer;
	float x, y;
	float width, height;
	float rotation;
	int layer_position;
	
	private float world_size_x, world_size_y;
	private float rect_x1, rect_y1;
	private float rect_x2, rect_y2;
	
	private float drag_start_x, drag_start_y;
	private float drag_start_width, drag_start_height;
	
	void init(AdvToolScript@ script, EmitterTool@ tool, entity@ emitter, const int scene_index)
	{
		SelectableData::init(script, emitter.id() + '', scene_index);
		
		@this.tool = tool;
		@this.emitter = emitter;
		
		@vars = emitter.vars();
		@emitter_id_var = vars.get_var('emitter_id');
		@width_var = vars.get_var('width');
		@height_var = vars.get_var('height');
		@rotation_var = vars.get_var('e_rotation');
		@sublayer_var = vars.get_var('draw_depth_sub');
		
		emitter_id = emitter_id_var.get_int32();
		layer = emitter.layer();
		sublayer = sublayer_var.get_int32();
		width = width_var.get_int32();
		height = height_var.get_int32();
		rotation = rotation_var.get_int32();
		layer_position = tool.script.layer_position(layer);
		
		modified = false;
		mouse_over_handle = false;
	}
	
	void update()
	{
		x = emitter.x();
		y = emitter.y();
		
		script.transform(x, y, layer, 22, aabb_x, aabb_y);
		script.transform_size(width, height, layer, 22, world_size_x, world_size_y);
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
	
	/// Changing an emitter's properties does not reflect in the editor for some reason.
	/// And simply adding and removing does not seem to work either. At the end of the frame remove the emitter.
	/// Then at the start of the next, remove it again, and then add it.
	void pre_step_validate()
	{
		if(!modified)
			return;
		
		script.g.remove_entity(emitter);
		script.g.add_entity(emitter);
		modified = false;
	}
	
	void post_step_validate()
	{
		if(!modified)
			return;
		
		script.g.remove_entity(emitter);
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
		
		if(sublayer != other.sublayer)
			return sublayer - other.sublayer;
		
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
		
		emitter.set_xy(x, y);
	}
	
	void stop_drag(const bool accept)
	{
		if(!accept)
		{
			x = drag_start_x;
			y = drag_start_y;
			
			emitter.set_xy(x, y);
		}
	}
	
	void move(const float dx, const float dy)
	{
		x += dx;
		y += dy;
		
		emitter.set_xy(x, y);
	}
	
	// Layer/Sublayer
	
	void shift_layer(const int dir, const bool sublayer=false)
	{
		if(sublayer)
		{
			this.sublayer = clamp(this.sublayer + dir, 0, 24);
			sublayer_var.set_int32(this.sublayer);
		}
		else
		{
			layer = clamp(layer + dir, 0, 20);
			emitter.layer(layer);
		}
		
		modified = true;
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
		
		width_var.set_int32(ceil_int(width));
		height_var.set_int32(ceil_int(height));
		
		x = anchor_x;
		y = anchor_y;
		
		emitter.set_xy(x, y);
		
		modified = true;
		update();
	}
	
	void stop_scale(const bool cancel)
	{
		if(cancel)
		{
			width  = drag_start_width;
			height = drag_start_height;
			width_var.set_int32(round_int(width));
			height_var.set_int32(round_int(height));
			stop_drag(false);
		}
		
		modified = true;
		update();
	}
	
	// Rotate
	
	void start_rotate()
	{
		drag_start_width = rotation;
	}
	
	void do_rotate(const float rotation)
	{
		this.rotation = rotation;
		rotation_var.set_int32(round_int(this.rotation));
		
		modified = true;
		update();
	}
	
	void stop_rotate(const bool cancel)
	{
		if(cancel)
		{
			rotation = drag_start_width;
			rotation_var.set_int32(round_int(rotation));
		}
		
		modified = true;
		update();
	}
	
	// Property setters
	
	void update_emitter_id(const int id)
	{
		emitter_id = id;
		emitter_id_var.set_int32(emitter_id);
		
		modified = true;
		update();
	}
	
	void update_rotation(const float rotation)
	{
		this.rotation = rotation;
		rotation_var.set_int32(round_int(rotation));
		
		modified = true;
		update();
	}
	
	void update_layer(const int layer, const int sublayer)
	{
		this.layer = clamp(layer, 0, 20);
		emitter.layer(this.layer);
		this.sublayer = clamp(sublayer, 0, 24);
		sublayer_var.set_int32(this.sublayer);
		
		modified = true;
		update();
	}
	
	void update_layer(const int layer)
	{
		this.layer = clamp(layer, 0, 20);
		emitter.layer(this.layer);
		
		modified = true;
		update();
	}
	
	void update_sublayer(const int sublayer)
	{
		this.sublayer = clamp(sublayer, 0, 24);
		sublayer_var.set_int32(this.sublayer);
		
		modified = true;
		update();
	}
	
}
