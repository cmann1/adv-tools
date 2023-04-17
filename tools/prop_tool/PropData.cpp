#include '../../misc/SelectableData.cpp';
#include '../../misc/LineData.cpp';

class PropData : SelectableData
{
	
	PropTool@ tool;
	prop@ prop;
	
	const array<array<float>>@ outline;
	
	private float drag_start_x, drag_start_y;
	private float rotate_start_angle;
	private float rotate_offset_angle;
	private float drag_start_scale_x, drag_start_scale_y;
	
	private float selection_local_x1, selection_local_y1;
	private float selection_local_x2, selection_local_y2;
	
	float anchor_x, anchor_y;
	
	float local_x1, local_y1;
	float local_x2, local_y2;
	
	sprites@ spr;
	string sprite_set;
	string sprite_name;
	
	float x, y;
	private float angle;
	private float layer_scale;
	private float backdrop_scale;
	private float draw_scale;
	private float draw_scale_x, draw_scale_y;
	
	private float align_x, align_y;
	private float prop_left, prop_top;
	private float prop_width, prop_height;
	private float prop_offset_x, prop_offset_y;
	private float prop_scale_x, prop_scale_y;
	
	private int lines_size = 32;
	private int lines_count;
	private array<LineData> lines(lines_size);
	
	private bool requires_update = false;
	
	void init(AdvToolScript@ script, PropTool@ tool, prop@ prop, const array<array<float>>@ outline)
	{
		SelectableData::init(script, prop.id() + '', scene_index);
		
		@this.tool = tool;
		@this.prop = prop;
		@this.outline = outline;
		sprite_from_prop(@prop, sprite_set, sprite_name);
		
		if(@spr == null)
		{
			@spr = create_sprites();
		}
		
		spr.add_sprite_set(sprite_set);
		
		align_x = tool.origin_align_x;
		align_y = tool.origin_align_y;
		
		requires_update = true;
		update();
		init_prop();
	}
	
	void init_from_prop(
		AdvToolScript@ script, PropTool@ tool, prop@ prop, const array<array<float>>@ outline,
		const int layer=-1, const int sub_layer=-1)
	{
		@this.script = script;
		@this.tool = tool;
		@this.outline = outline;
		sprite_from_prop(@prop, sprite_set, sprite_name);
		
		if(@this.prop == null)
		{
			@this.prop = create_prop();
		}
		
		this.prop.x(prop.x());
		this.prop.y(prop.y());
		this.prop.rotation(prop.rotation());
		this.prop.scale_x(prop.scale_x());
		this.prop.scale_y(prop.scale_y());
		this.prop.prop_set(prop.prop_set());
		this.prop.prop_group(prop.prop_group());
		this.prop.prop_index(prop.prop_index());
		this.prop.palette(prop.palette());
		this.prop.layer(layer < 0 ? prop.layer() : layer);
		this.prop.sub_layer(sub_layer < 0 ? prop.sub_layer() : sub_layer);
		
		if(@spr == null)
		{
			@spr = create_sprites();
		}
		
		spr.add_sprite_set(sprite_set);
		
		align_x = 0.5;
		align_y = 0.5;
		
		requires_update = true;
		update();
		init_prop();
	}
	
	void step()
	{
		script.transform(x, y, prop.layer(), 22, aabb_x, aabb_y);
	}
	
	void draw(const PropToolHighlight highlight)
	{
		float line_width;
		uint line_colour, fill_colour;
		get_colours(line_width, line_colour, fill_colour);
		
		if(
			primary_selected || hovered || (highlight & Highlight) != 0 ||
			(highlight & Outline) != 0)
		{
			spr.draw_world(22, 22, sprite_name, 0, prop.palette(),
				aabb_x, aabb_y, prop.rotation(),
				draw_scale_x, draw_scale_y,
				fill_colour);
		}
		
		if(primary_selected || hovered || (highlight & Outline) != 0)
		{
			for(int i = lines_count - 1; i >= 0; i--)
			{
				LineData@ line = @lines[i];
				// Draw outlines on separate sublayer to sprite so that they can be batched,
				script.g.draw_line_world(22, 23,
					aabb_x + line.x1, aabb_y + line.y1,
					aabb_x + line.x2, aabb_y + line.y2,
					line_width, line_colour);
			}
		}
		
		//if(selected)
		//{
		//	outline_rect(script.g,22,22,
		//		aabb_x + aabb_x1, aabb_y + aabb_y1,
		//		aabb_x + aabb_x2, aabb_y + aabb_y2,
		//		1 / script.zoom, 0xaaff0000);
		//}
	}
	
	void update(const bool force_update = false)
	{
		if(!requires_update && !force_update)
			return;
		
		prop_scale_x = prop.scale_x();
		prop_scale_y = prop.scale_y();
		x = prop.x();
		y = prop.y();
		
		script.transform(x, y, prop.layer(), 22, aabb_x, aabb_y);
		
		angle = prop.rotation() * DEG2RAD * sign(prop_scale_x) * sign(prop_scale_y);
		layer_scale = prop.layer() <= 5 ? script.layer_scale(prop.layer()) : 1.0;
		backdrop_scale = prop.layer() <= 5 ? 2.0 : 1.0;
		draw_scale = script.layer_scale(prop.layer()) / script.layer_scale(22);
		
		const float cos_angle = cos(angle);
		const float sin_angle = sin(angle);
		
		draw_scale_x = prop_scale_x / layer_scale * backdrop_scale;
		draw_scale_y = prop_scale_y / layer_scale * backdrop_scale;
		
		lines_count = 0;
		
		local_x1 = local_y1 = 0;
		local_x2 = local_y2 = 0;
		
		for(int i = 0, path_count = int(outline.length()); i < path_count; i++)
		{
			const array<float>@ path = @outline[i];
			const int count = int(path.length());
			
			if(lines_count + count >= lines_size)
			{
				lines.resize(lines_size = lines_count + count + 32);
			}
			
			float p_x = path[0];
			float p_y = path[1];
			float prev_x = (cos_angle * p_x - sin_angle * p_y) * draw_scale_x;
			float prev_y = (sin_angle * p_x + cos_angle * p_y) * draw_scale_y;
			
			if(i == 0)
			{
				local_x1 = local_x2 = prev_x;
				local_y1 = local_y2 = prev_y;
			}
			
			prev_x *= draw_scale;
			prev_y *= draw_scale;
			
			for(int j = 2; j < count; j += 2)
			{
				p_x = path[j];
				p_y = path[j + 1];
				
				float x = (cos_angle * p_x - sin_angle * p_y) * draw_scale_x;
				float y = (sin_angle * p_x + cos_angle * p_y) * draw_scale_y;
				
				if(x < local_x1) local_x1 = x;
				if(x > local_x2) local_x2 = x;
				if(y < local_y1) local_y1 = y;
				if(y > local_y2) local_y2 = y;
				
				x *= draw_scale;
				y *= draw_scale;
				const float dx = x - prev_x;
				const float dy = y - prev_y;
				
				LineData@ line = @lines[lines_count++];
				line.x1 = prev_x;
				line.y1 = prev_y;
				line.x2 = x;
				line.y2 = y;
				line.mx = (prev_x + x) * 0.5;
				line.my = (prev_y + y) * 0.5;
				line.length = sqrt(dx * dx + dy * dy);
				line.angle = atan2(-dx, dy) * RAD2DEG;
				line.inv_delta_x = dx != 0 ? 1 / dx : 1;
				line.inv_delta_y = dy != 0 ? 1 / dy : 1;
				
				prev_x = x;
				prev_y = y;
			}
		}
		
		script.transform_size(local_x1, local_y1, prop.layer(), 22, aabb_x1, aabb_y1);
		script.transform_size(local_x2, local_y2, prop.layer(), 22, aabb_x2, aabb_y2);
		
		draw_scale_x *= draw_scale;
		draw_scale_y *= draw_scale;
		requires_update = false;
	}
	
	void set_position(const float x, const float y)
	{
		init_anchors();
		
		float ox, oy;
		rotate(prop_offset_x, prop_offset_y, prop.rotation() * DEG2RAD, ox, oy);
		this.x = x - ox;
		this.y = y - oy;
		prop.x(this.x);
		prop.y(this.y);
	}
	
	void set_prop_rotation(float rotation)
	{
		rotation = rotation % 360;
		if(prop.rotation() == rotation)
			return;
		
		float ox, oy;
		rotate(prop_offset_x, prop_offset_y, rotation * DEG2RAD, ox, oy);
		
		x = anchor_x - ox;
		y = anchor_y - oy;
		
		prop.rotation(rotation % 360);
		prop.x(x);
		prop.y(y);
		requires_update = true;
	}
	
	void set_scale(float scale_x, float scale_y)
	{
		prop_scale_x = scale_x;
		prop_scale_y = scale_y;
		
		if(prop_scale_x == 0)
			prop_scale_x = 0.001;
		if(prop_scale_y == 0)
			prop_scale_y = 0.001;
		
		if(prop.scale_x() == prop_scale_x && prop.scale_y() == prop_scale_y)
			return;
		
		float ox, oy;
		rotate(prop_offset_x, prop_offset_y, prop.rotation() * DEG2RAD, ox, oy);
		
		x = anchor_x - ox;
		y = anchor_y - oy;
		
		prop.scale_x(prop_scale_x);
		prop.scale_y(prop_scale_y);
		requires_update = true;
	}
	
	void anchor_world(float world_x, float world_y)
	{
		const float scale_x = prop_scale_x / layer_scale * backdrop_scale;
		const float scale_y = prop_scale_y / layer_scale * backdrop_scale;
		
		rotate(world_x - x, world_y - y, -prop.rotation() * DEG2RAD, world_x, world_y);
		align_x = (world_x - prop_left * scale_x) / (prop_width * scale_x);
		align_y = (world_y - prop_top * scale_y)  / (prop_height * scale_y);
		
		init_anchors();
	}
	
	void shift_layer(const int dir, const bool sublayer=false)
	{
		if(sublayer)
		{
			prop.sub_layer(clamp(prop.sub_layer() + dir, 0, 24));
		}
		else
		{
			const uint layer = prop.layer();
			const uint new_layer = clamp(layer + dir, 0, 20);
			
			if(new_layer <= 5 && layer > 5 || new_layer > 5 && layer <= 5)
			{
				script.g.remove_prop(prop);
				prop.layer(new_layer);
				script.g.add_prop(prop);
			}
			else
			{
				prop.layer(new_layer);
			}
			
			if(new_layer != layer)
			{
				requires_update = true;
			}
		}
		
		update();
	}
	
	bool intersects_aabb(float x1, float y1, float x2, float y2)
	{
		x1 -= aabb_x;
		y1 -= aabb_y;
		x2 -= aabb_x;
		y2 -= aabb_y;
		
		for(int i = lines_count - 1; i >= 0; i--)
		{
			float t_min, t_max;
			if(lines[i].aabb_intersection(x1, y1, x2, y2, t_min, t_max))
				return true;
		}
		
		if(intersects(x1 + aabb_x, y1 + aabb_y))
			return true;
		if(intersects(x2 + aabb_x, y2 + aabb_y))
			return true;
		if(intersects(x2 + aabb_x, y1 + aabb_y))
			return true;
		if(intersects(x1 + aabb_x, y2 + aabb_y))
			return true;
		
		return false;
	}
	
	bool intersects(float px, float py)
	{
		if(
			px < aabb_x + aabb_x1 || px > aabb_x + aabb_x2 ||
			py < aabb_y + aabb_y1 || py > aabb_y + aabb_y2)
			return false;
		
		const float scale_x = prop_scale_x / layer_scale * backdrop_scale;
		const float scale_y = prop_scale_y / layer_scale * backdrop_scale;
		
		// Calculate mouse "local" position relative to prop rotation and scale
		
		tool.script.transform(px, py, 22, prop.layer(), px, py);
		
		rotate(
			(px - x) / scale_x,
			(py - y) / scale_y,
			-angle, px, py);
		
		for(uint i = 0; i < outline.length(); i++)
		{
			if(point_in_polygon(px, py, @outline[i]))
				return true;
		}
		
		return false;
	}
	
	private void init_prop()
	{
		rectangle@ r = spr.get_sprite_rect(sprite_name, 0);
		prop_left = r.left();
		prop_top = r.top();
		prop_width = r.get_width();
		prop_height = r.get_height();
		
		init_anchors();
	}
	
	void init_anchors()
	{
		prop_offset_x = (prop_left + prop_width * align_x) * prop_scale_x  / layer_scale * backdrop_scale;
		prop_offset_y = (prop_top  + prop_height * align_y) * prop_scale_y / layer_scale * backdrop_scale;
		
		rotate(prop_offset_x, prop_offset_y, prop.rotation() * DEG2RAD, anchor_x, anchor_y);
		
		anchor_x += x;
		anchor_y += y;
	}
	
	void store_selection_bounds()
	{
		selection_local_x1 = local_x1 / prop_scale_x;
		selection_local_y1 = local_y1 / prop_scale_y;
		selection_local_x2 = local_x2 / prop_scale_x;
		selection_local_y2 = local_y2 / prop_scale_y;
	}
	
	void get_selection_bounds(float &out x1, float &out y1, float &out x2, float &out y2)
	{
		x1 = selection_local_x1 * prop_scale_x;
		y1 = selection_local_y1 * prop_scale_y;
		x2 = selection_local_x2 * prop_scale_x;
		y2 = selection_local_y2 * prop_scale_y;
		
		if(x2 < x1)
		{
			const float t = x1;
			x1 = x2;
			x2 = t;
		}
		
		if(y2 < y1)
		{
			const float t = y1;
			y1 = y2;
			y2 = t;
		}
	}
	
	//
	
	void start_drag()
	{
		drag_start_x = x;
		drag_start_y = y;
	}
	
	void do_drag(const float drag_delta_x, const float drag_delta_y)
	{
		x = drag_start_x + drag_delta_x;
		y = drag_start_y + drag_delta_y;
		
		init_anchors();
		
		prop.x(x);
		prop.y(y);
	}
	
	void cancel_drag()
	{
		x = drag_start_x;
		y = drag_start_y;
		
		prop.x(x);
		prop.y(y);
	}
	
	void move(const float dx, const float dy)
	{
		x += dx;
		y += dy;
		
		init_anchors();
		
		prop.x(x);
		prop.y(y);
	}
	
	//
	
	void start_rotate(const float anchor_x, const float anchor_y, const float base_rotation)
	{
		rotate_start_angle = prop.rotation();
		rotate_offset_angle = prop.rotation() - base_rotation;
		anchor_world(anchor_x, anchor_y);
		
		start_drag();
	}
	
	void do_rotation(const float angle)
	{
		set_prop_rotation(rotate_offset_angle + angle);
	}
	
	void stop_rotate(const bool cancel)
	{
		if(cancel)
		{
			prop.rotation(rotate_start_angle);
			cancel_drag();
		}
		
		align_x = tool.origin_align_x;
		align_y = tool.origin_align_y;
		init_anchors();
		
		update();
	}
	
	//
	
	void start_scale(const float anchor_x, const float anchor_y)
	{
		drag_start_scale_x = prop_scale_x;
		drag_start_scale_y = prop_scale_y;
		anchor_world(anchor_x, anchor_y);
		
		start_drag();
	}
	
	void do_scale(float scale_x, float scale_y, const bool auto_correct=false)
	{
		prop_scale_x = drag_start_scale_x * scale_x;
		prop_scale_y = drag_start_scale_y * scale_y;
		
		if(auto_correct)
		{
			prop_scale_x = get_valid_prop_scale(abs(prop_scale_x)) * sign(prop_scale_x);
			prop_scale_y = get_valid_prop_scale(abs(prop_scale_y)) * sign(prop_scale_y);
			scale_x = prop_scale_x / drag_start_scale_x;
			scale_y = prop_scale_y / drag_start_scale_y;
		}
		
		if(prop_scale_x == 0)
			prop_scale_x = 0.001;
		if(prop_scale_y == 0)
			prop_scale_y = 0.001;
		
		if(prop.scale_x() == prop_scale_x && prop.scale_y() == prop_scale_y)
			return;
		
		prop.scale_x(prop_scale_x);
		prop.scale_y(prop_scale_y);
		
		float ox, oy;
		rotate(prop_offset_x * scale_x, prop_offset_y * scale_y, prop.rotation() * DEG2RAD, ox, oy);
		
		x = anchor_x - ox;
		y = anchor_y - oy;
		
		prop.x(x);
		prop.y(y);
		requires_update = true;
	}
	
	void stop_scale(const bool cancel)
	{
		if(cancel)
		{
			prop.scale_x(drag_start_scale_x);
			prop.scale_y(drag_start_scale_y);
			cancel_drag();
		}
		
		align_x = tool.origin_align_x;
		align_y = tool.origin_align_y;
		
		init_anchors();
		update();
	}
	
}
