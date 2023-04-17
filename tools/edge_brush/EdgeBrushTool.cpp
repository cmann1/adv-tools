#include '../../../../lib/tiles/common.cpp';
#include '../../../../lib/tiles/get_tile_edge_points.cpp';
#include '../../../../lib/tiles/get_tile_quad.cpp';
#include '../../../../lib/tiles/closest_point_on_tile.cpp';
#include '../../../../lib/tiles/EdgeFlags.cpp';
#include '../../../../lib/tiles/TileEdge.cpp';

#include 'EdgeBrushMode.cpp';
#include 'EdgeBrushRenderMode.cpp';
#include 'EdgeBrushState.cpp';
#include 'EdgeBrushToolbar.cpp';
#include 'EdgeFacing.cpp';
#include 'TileEdgeData.cpp';

const string EDGE_BRUSH_SPRITES_BASE = SPRITES_BASE + 'edge_brush/';
const string EMBED_spr_icon_edge_brush = SPRITES_BASE + 'icon_edge_brush.png';

class EdgeBrushTool : Tool
{
	
	private EdgeBrushState state = Idle;
	private Mouse@ mouse;
	private int layer;
	private float x, y;
	
	private float drag_radius_start;
	private float drag_radius_x, drag_radius_y;
	
	/// -1 = Off, 1 = On, 0 = None
	private int draw_mode = 0;
	/// In precision mode, holding shift before clicking will lock the edge mask to only the current closest edge
	/// until the mouse is released again.
	private uint locked_edge_mask;
	
	private dictionary tile_chunks;
	private int tile_cache_layer = -1;
	
	private int draw_list_index;
	private int draw_list_size = 1024;
	private array<TileEdgeData@> draw_list(draw_list_size);
	
	private TileEdgeData@ precision_edge;
	private TileEdge precision_edge_index;
	private float precision_edge_px, precision_edge_py;
	
	private EdgeBrushToolbar toolbar;
	
	// //////////////////////////////////////////////////////////
	// Settings
	// //////////////////////////////////////////////////////////
	
	// TODO: Set to Brush
	EdgeBrushMode mode = Brush;
	float brush_radius = 48;
	/// Which edges must be updated: Top, Bottom, Left, Right
	uint edge_mask = 0x1 | 0x2 | 0x4 | 0x8;
	/// Each edge has two bits/flags controlling whether an edge has collision and is rendered: collision and priority
	/// If the collision bit is on, or collision is off and priority is one, an edge is rendered.
	/// If the collision and priorty bit are both off no edge is rendered.
	bool update_collision = true;
	bool update_priority = true;
	uint8 update_custom = 0;
	/// Must internal edges (edges shared by two tiles) by updated
	EdgeFacing edge_facing = External;
	/// If true, edges shared by tiles with different sprites will be considered "external"
	bool check_internal_sprites = true;
	/// By default in edge mode the nearest edge will be updated.
	/// Setting this to true, only the edges of the tile the mouse is inside of will be checked
	bool precision_inside_tile_only = false;
	/// When turning the edge on for a tile, should the edge on adjacent tile be turned off?
	bool precision_update_neighbour = true;
	EdgeBrushRenderMode render_mode = Always;
	
	private bool is_layer_valid { get const { return layer >= 6 && layer <= 20; } }
	
	EdgeBrushTool(AdvToolScript@ script)
	{
		super(script, 'Tiles', 'Edge Brush');
		
		// Set priority = -1 so the tile tool shortcut takes priority
		init_shortcut_key(VK::B, ModifierKey::None, -1);
	}
	
	void build_sprites(message@ msg) override
	{
		build_sprite(msg, 'icon_edge_brush');
		
		toolbar.build_sprites(msg);
	}
	
	void create(ToolGroup@ group) override
	{
		set_icon(SPRITE_SET, 'icon_edge_brush');
	}
	
	void on_init() override
	{
		@mouse = @script.mouse;
	}
	
	void update_mode(const EdgeBrushMode new_mode, const bool notify=true)
	{
		if(mode == new_mode)
			return;
		
		mode = new_mode;
		locked_edge_mask = 0;
		
		string mode_name;
		switch(mode)
		{
			case Brush: mode_name = 'Brush'; break;
			case Precision: mode_name = 'Precision'; break;
			default:
				mode = Brush;
				mode_name = 'Brush';
				break;
		}
		
		toolbar.update_mode();
		
		if(notify)
		{
			script.info_overlay.show(mouse, mode_name + ' Mode', 0.5);
		}
	}
	
	void cycle_mode(const int direction=1)
	{
		update_mode(EdgeBrushMode(
			(mode + (direction >= 0 ? 1 : -1)) % (Precision + 1)
		));
	}
	
	void update_edge_mask(uint8 edge_mask)
	{
		if(this.edge_mask == edge_mask)
			return;
		
		this.edge_mask = edge_mask;
		toolbar.update_edge_mask();
		clear_tile_cache();
	}
	
	void update_edge_mask(const TileEdge edge, const bool on)
	{
		uint8 edge_mask = this.edge_mask & ~(1 << edge);
		edge_mask |= on ? (1 << edge) : 0;
		update_edge_mask(edge_mask);
	}
	
	void update_brush_radius(const float new_radius, const float overlay_time=0.1)
	{
		brush_radius = max(5.0, new_radius);
		
		toolbar.update_brush_radius();
		
		if(overlay_time > 0)
		{
			if(state == EdgeBrushState::DragRadius)
			{
				script.info_overlay.show(drag_radius_x, drag_radius_y, round(brush_radius) + '', overlay_time);
			}
			else
			{
				script.info_overlay.show(mouse, round(brush_radius) + '', overlay_time);
			}
		}
	}
	
	void update_inside_tile_only(const bool on)
	{
		precision_inside_tile_only = on;
		clear_tile_cache();
	}
	
	void do_update_collision(bool on)
	{
		if(on == update_collision)
			return;
		
		update_collision = on;
		toolbar.update_collision();
	}
	
	void do_update_priority(bool on)
	{
		if(on == update_priority)
			return;
		
		update_priority = on;
		toolbar.update_priority();
	}
	
	void update_edge_facing(const EdgeFacing new_facing, const bool notify=true)
	{
		if(edge_facing == new_facing)
			return;
		
		edge_facing = new_facing;
		
		string facing_name;
		switch(edge_facing)
		{
			case EdgeFacing::Both: facing_name = 'Both'; break;
			case EdgeFacing::Internal: facing_name = 'Internal'; break;
			default:
				edge_facing = External;
				facing_name = 'External';
				break;
		}
		
		toolbar.update_mode();
		
		if(notify)
		{
			script.info_overlay.show(mouse, 'Edge Facing: ' + facing_name, 0.5);
		}
	}
	
	void update_internal_sprites(const bool on)
	{
		check_internal_sprites = on;
		clear_tile_cache();
	}
	
	void update_update_neighbour(const bool on)
	{
		precision_update_neighbour = on;
	}
	
	void update_render_mode(const EdgeBrushRenderMode render_mode)
	{
		this.render_mode = render_mode;
	}
	
	private void clear_tile_cache()
	{
		tile_chunks.deleteAll();
		tile_cache_layer = layer;
	}
	
	/// Retrieve or create the TileData array cache for a chunk
	private array<TileEdgeData>@ get_chunk(const int chunk_x, const int chunk_y)
	{
		const string chunk_key = chunk_x + ',' + chunk_y;
		
		if(tile_chunks.exists(chunk_key))
			return cast<array<TileEdgeData>@>(tile_chunks[chunk_key]);
		
		array<TileEdgeData>@ chunk = array<TileEdgeData>(Settings::TileChunkSize * Settings::TileChunkSize);
		@tile_chunks[chunk_key] = @chunk;
		return chunk;
	}
	
	private bool check_edge(const int tx, const int ty, TileEdgeData@ data, const TileEdge edge,
		const bool fast=false,
		const float cx=0, const float cy=0, const float radius=0)
	{
		if((data.valid_edges & (1 << edge)) == 0)
			return false;
		
		data.select_edge(edge);
		
		if(!fast && radius > 0)
		{
			if(!line_circle_intersect(cx, cy, brush_radius, data.ex1, data.ey1, data.ex2, data.ey2))
				return false;
		}
		
		if(edge_facing == EdgeFacing::Both)
			return true;
		
		const EdgeFacing facing = (data.edges_facing >> edge) & 0x1 == 1
			? EdgeFacing::External : EdgeFacing::Internal;
		
		return facing == edge_facing;
	}
	
	// //////////////////////////////////////////////////////////
	// Callbacks
	// //////////////////////////////////////////////////////////
	
	protected void on_select_impl()
	{
		script.hide_gui_panels(true);
		
		toolbar.show(script, this);
	}
	
	protected void on_deselect_impl()
	{
		script.hide_gui_panels(false);
		
		state = Idle;
		draw_list_index = 0;
		@precision_edge = null;
		clear_tile_cache();
		toolbar.hide();
	}
	
	protected void step_impl() override
	{
		// TODO: Add mouse/key shortcuts for toggling update_collision/priority
		
		layer = script.layer;
		if(layer == 18)
		{
			layer = 19;
		}
		
		x = mouse.x;
		y = mouse.y;
		
		if(tile_cache_layer != layer)
		{
			clear_tile_cache();
		}
		
		draw_list_index = 0;
		@precision_edge = null;
		
		switch(state)
		{
			case EdgeBrushState::Idle: state_idle(); break;
			case EdgeBrushState::Draw: state_draw(); break;
			case EdgeBrushState::DragRadius: state_drag_radius(); break;
		}
		
		if(mode == Precision && @precision_edge != null)
		{
			toolbar.show_edge_info(precision_edge);
		}
		else
		{
			script.hide_info_popup();
		}
	}
	
	protected void draw_impl(const float sub_frame) override
	{
		if((!script.mouse_in_scene && state == Idle) || !is_layer_valid)
			return;
		
		const float line_width = min(Settings::EdgeMarkerLineWidth / script.zoom, 10.0);
		
		const int start_time = Settings::EdgeBrushDebugTiming && mode != Precision
			? get_time_us() : 0;
		int stats_draw_count = 0;
		
		for(int i = 0; i < draw_list_index; i++)
		{
			TileEdgeData@ data = draw_list[i];
			
			for(TileEdge edge = TileEdge::Top; edge <= TileEdge::Right; edge++)
			{
				if(data.draw_edges & (1 << edge) == 0)
					continue;
				
				data.select_edge(edge);
				
				float x1, y1, x2, y2;
				script.transform(data.ex1, data.ey1, layer, 22, x1, y1);
				script.transform(data.ex2, data.ey2, layer, 22, x2, y2);
				
				const uint clr = data.get_colour();
				
				script.g.draw_line_world(22, 22, x1, y1, x2, y2, line_width, clr);
				
				if(Settings::EdgeBrushDebugTiming)
				{
					stats_draw_count++;
				}
			}
		}
		
		if(Settings::EdgeBrushDebugTiming && mode != Precision)
		{
			const int total_time = get_time_us() - start_time;
			int idx = 100;
			script.debug.print('DrawCount: ' + stats_draw_count, 0xff00ffff, idx++);
			script.debug.print('DrawTime: ' + (total_time / 1000) + 'ms', 0xff00ffff, idx++);
		}
		
		switch(mode)
		{
			case EdgeBrushMode::Brush: draw_brush(sub_frame); break;
			case EdgeBrushMode::Precision: draw_precision(sub_frame); break;
		}
		
		draw_cursor();
	}
	
	private void draw_cursor()
	{
		drawing::fill_circle(script.g, 22, 22,
			x, y, min(brush_radius, 3 / script.zoom), 16,
			Settings::CursorLineColour, Settings::CursorLineColour);
	}
	
	private void draw_brush(const float sub_frame)
	{
		if(!script.shift.down)
		{
			script.circle(22, 22, x, y, brush_radius, 64,
				Settings::CursorLineWidth, Settings::CursorLineColour);
		}
		else
		{
			outline_rect(script.g, 22, 22,
				x - brush_radius, y - brush_radius,
				x + brush_radius, y + brush_radius,
				Settings::CursorLineWidth / script.zoom, Settings::CursorLineColour);
		}
	}
	
	private void draw_precision(const float sub_frame)
	{
		if(@precision_edge == null)
			return;
		
		precision_edge.select_edge(precision_edge_index);
		
		float ex1, ey1, ex2, ey2, px, py;
		script.transform(precision_edge.ex1, precision_edge.ey1, layer, 22, ex1, ey1);
		script.transform(precision_edge.ex2, precision_edge.ey2, layer, 22, ex2, ey2);
		script.transform(precision_edge_px, precision_edge_py, layer, 22, px, py);
		
		const uint clr = precision_edge.get_colour() | 0xff000000;
		
		const float line_width = Settings::EdgeMarkerLineWidth / script.zoom;
		
		script.g.draw_line_world(22, 21,
			mouse.x, mouse.y, px, py,
			line_width, Settings::EdgeArrowMarkerColour);
		
		const float radius = min(Settings::EdgeMarkerRadius / script.zoom, 24.0);
		
		const float mx = (ex1 + ex2) * 0.5;
		const float my = (ey1 + ey2) * 0.5;
		float dx = ex2 - ex1;
		float dy = ey2 - ey1;
		const float length = sqrt(dx * dx + dy * dy);
		dx /= length;
		dy /= length;
		script.g.draw_line_world(22, 21,
			mx, my,
			mx + dy * radius, my - dx * radius,
			min(line_width * 1.5, 8.0),
			clr);
	}
	
	// //////////////////////////////////////////////////////////
	// States
	// //////////////////////////////////////////////////////////
	
	private void state_idle()
	{
		if(script.scroll_layer(true, false, true))
		{
			layer = script.layer;
			clear_tile_cache();
		}
		
		// Cycle mode
		if(mouse.middle_press)
		{
			cycle_mode();
		}
		
		check_edge_mask_keys();
		
		switch(mode)
		{
			case Brush: brush_state_idle(); break;
			case Precision: precision_state_idle(); break;
		}
	}
	
	private void check_edge_mask_keys()
	{
		uint8 edge_mask = this.edge_mask;
		
		if(script.input.key_check_pressed_vk(VK::Up))
			edge_mask ^= TopBit;
		if(script.input.key_check_pressed_vk(VK::Down))
			edge_mask ^= BottomBit;
		if(script.input.key_check_pressed_vk(VK::Left))
			edge_mask ^= LeftBit;
		if(script.input.key_check_pressed_vk(VK::Right))
			edge_mask ^= RightBit;
		
		if(edge_mask != this.edge_mask)
		{
			update_edge_mask(edge_mask);
		}
	}
	
	private void brush_state_idle()
	{
		// Change brush size mouse wheel
		if(!script.ctrl.down && mouse.scroll != 0)
		{
			update_brush_radius(brush_radius - mouse.scroll * 10, 0.5);
		}
		// Change brush size by dragging
		if(!script.ctrl.down && !script.shift.down && script.alt.down && mouse.left_press)
		{
			drag_radius_start = brush_radius;
			drag_radius_x = mouse.x;
			drag_radius_y = mouse.y;
			state = DragRadius;
			return;
		}
		
		if(render_mode == Always && script.mouse_in_scene)
		{
			do_brush_mode(0);
		}
		
		if(mouse.left_press || mouse.right_press)
		{
			draw_mode = mouse.left_press ? 1 : -1;
			clear_tile_cache();
			state = Draw;
			return;
		}
	}
	
	private void precision_state_idle()
	{
		if(!script.shift.down && locked_edge_mask != 0)
		{
			locked_edge_mask = 0;
		}
		
		if((render_mode == Always || script.shift.down) && script.mouse_in_scene)
		{
			do_precision_mode(0);
		}
		
		if(mouse.left_press || mouse.right_press)
		{
			draw_mode = mouse.left_press ? 1 : -1;
			clear_tile_cache();
			state = Draw;
			return;
		}
	}
	
	private void state_draw()
	{
		switch(mode)
		{
			case Brush: brush_state_draw(); break;
			case Precision: precision_state_draw(); break;
		}
	}
	
	private void brush_state_draw()
	{
		do_brush_mode(draw_mode);
		
		if(draw_mode == 1 && !mouse.left_down || draw_mode == -1 && !mouse.right_down)
		{
			state = Idle;
			return;
		}
	}
	
	private void precision_state_draw()
	{
		do_precision_mode(draw_mode);
		
		if(draw_mode == 1 && !mouse.left_down || draw_mode == -1 && !mouse.right_down)
		{
			state = Idle;
			return;
		}
	}
	
	private void state_drag_radius()
	{
		if(!mouse.left_down)
		{
			state = Idle;
			return;
		}
		
		x = drag_radius_x;
		y = drag_radius_y;
		
		update_brush_radius(
			(mouse.x - drag_radius_x) +
			drag_radius_start);
	}
	
	private void do_brush_mode(const int update_edges)
	{
		if(!is_layer_valid)
			return;
		
		const bool render_edges = render_mode == Always || render_mode == DrawOnly && update_edges != 0;
		const bool reset_edges = update_edges == 1 && script.ctrl.down;
		
		float mx, my;
		script.mouse_layer(layer, mx, my);
		
		const float layer_radius = script.transform_size(brush_radius, mouse.layer, layer);
		const float radius_sqr = layer_radius * layer_radius;
		const int tx1 = floor_int((mx - layer_radius) * PIXEL2TILE);
		const int ty1 = floor_int((my - layer_radius) * PIXEL2TILE);
		const int tx2 = floor_int((mx + layer_radius) * PIXEL2TILE);
		const int ty2 = floor_int((my + layer_radius) * PIXEL2TILE);
		
		const int start_time = Settings::EdgeBrushDebugTiming ? get_time_us() : 0;
		int stats_tile_update_count = 0;
		int stats_tile_skip_count = 0;
		
		array<TileEdgeData>@ chunk;
		// Find all chunks the brush is touching
		const int chunks_x1 = floor_int(float(tx1) / Settings::TileChunkSize);
		const int chunks_y1 = floor_int(float(ty1) / Settings::TileChunkSize);
		const int chunks_x2 = floor_int(float(tx2) / Settings::TileChunkSize);
		const int chunks_y2 = floor_int(float(ty2) / Settings::TileChunkSize);
		
		for(int chunk_y = chunks_y1; chunk_y <= chunks_y2; chunk_y++)
		{
			for(int chunk_x = chunks_x1; chunk_x <= chunks_x2; chunk_x++)
			{
				@chunk = get_chunk(chunk_x, chunk_y);
				
				const int chunk_tx = chunk_x * Settings::TileChunkSize;
				const int chunk_ty = chunk_y * Settings::TileChunkSize;
				const int c_tx1 = tx1 > chunk_tx ? tx1 : chunk_tx;
				const int c_ty1 = ty1 > chunk_ty ? ty1 : chunk_ty;
				const int c_tx2 = tx2 < chunk_tx + Settings::TileChunkSize - 1
					? tx2 : chunk_tx + Settings::TileChunkSize - 1;
				const int c_ty2 = ty2 < chunk_ty + Settings::TileChunkSize - 1
					? ty2 : chunk_ty + Settings::TileChunkSize - 1;
				
				// Iterate all tiles within this chunk that are also inside the brush bounding box
				for(int tx = c_tx1; tx <= c_tx2; tx++)
				{
					for(int ty = c_ty1; ty <= c_ty2; ty++)
					{
						const float x = tx * 48;
						const float y = ty * 48;
						
						// Get and cache the tile data
						TileEdgeData@ data = @chunk[(ty - chunk_ty) * Settings::TileChunkSize + (tx - chunk_tx)];
						if(@data.tile == null)
						{
							data.init(script.g, tx, ty, layer, edge_mask, check_internal_sprites);
						}
						
						if(!data.solid)
						{
							if(Settings::EdgeBrushDebugTiming)
							{
								stats_tile_skip_count++;
							}
							continue;
						}
						
						data.draw_edges = 0;
						bool tile_requires_update = false;
						
						for(TileEdge edge = TileEdge::Top; edge <= TileEdge::Right; edge++)
						{
							if(!check_edge(tx, ty, data, edge, script.shift.down, mx, my, brush_radius))
								continue;
							
							if(
								update_edges != 0 && (data.updated_edges & (1 << edge)) == 0 &&
								data.update_edge(edge, update_edges, update_collision, update_priority, update_custom))
							{
								tile_requires_update = true;
							}
							else if(reset_edges)
							{
								tile_requires_update = true;
							}
							
							// Mark edge for rendering
							if(render_edges)
							{
								if(data.draw_edges == 0)
								{
									while(draw_list_index + 1 >= draw_list_size)
									{
										draw_list.resize(draw_list_size *= 2);
									}
									
									@draw_list[draw_list_index++] = data;
								}
								
								data.draw_edges |= (1 << edge);
							}
						} // edge
						
						if(tile_requires_update)
						{
							script.g.set_tile(tx, ty, layer, data.tile, reset_edges);
							
							// Query the tile properties again since the properties might have changed
							// after being updated
							if(reset_edges && !data.has_reset)
							{
								data.init(script.g, tx, ty, layer, edge_mask, check_internal_sprites);
								data.has_reset = true;
							}
							
							if(Settings::EdgeBrushDebugTiming)
							{
								stats_tile_update_count++;
							}
						}
					} // tile y
				} // tile x
			} // chunk x
		} // chunk y
		
		if(Settings::EdgeBrushDebugTiming)
		{
			const int total_time = get_time_us() - start_time;
			int idx = 0;
			const int tw = tx2 - tx1 + 1;
			const int th = ty2 - ty1 + 1;
			if(stats_tile_update_count > 0)
				script.debug.print('Updated: ' + stats_tile_update_count, 0xffffffff, idx++, 120);
			else
				idx++;
			script.debug.print('Tiles: ' + tw + ',' + th + ' ' + (tw * th), idx++);
			script.debug.print('NotSolid: ' + stats_tile_skip_count, idx++);
			script.debug.print('UpdateTime: ' + (total_time / 1000) + 'ms', idx++);
		}
	}
	
	private void do_precision_mode(const int update_edges)
	{
		if(!is_layer_valid)
			return;
		
		const bool render_edges = (render_mode == Always || script.shift.down) || render_mode == DrawOnly && update_edges != 0;
		
		float mx, my;
		script.mouse_layer(layer, mx, my);
		const int mtx = floor_int(mx * PIXEL2TILE);
		const int mty = floor_int(my * PIXEL2TILE);
		
		const float radius_sqr = 48 * 48;
		const float layer_radius = precision_inside_tile_only ? 0 : 48;
		const int tx1 = floor_int((mx - layer_radius) * PIXEL2TILE);
		const int ty1 = floor_int((my - layer_radius) * PIXEL2TILE);
		const int tx2 = floor_int((mx + layer_radius) * PIXEL2TILE);
		const int ty2 = floor_int((my + layer_radius) * PIXEL2TILE);
		
		TileEdgeData@ closest_data = null;
		int closest_tx, closest_ty;
		TileEdge closest_edge;
		float closest_distance = 999999;
		float closest_px, closest_py;
		
		TileEdgeData data = TileEdgeData();
		Line edge_line;
		
		for(int ty = ty1; ty <= ty2; ty++)
		{
			for(int tx = tx1; tx <= tx2; tx++)
			{
				data.init(script.g, tx, ty, layer, locked_edge_mask != 0 ? locked_edge_mask : edge_mask, check_internal_sprites);
				data.draw_edges = 0;
				
				if(!data.solid)
					continue;
				
				bool is_closest_tile = false;
				
				for(TileEdge edge = TileEdge::Top; edge <= TileEdge::Right; edge++)
				{
					if(!check_edge(tx, ty, data, edge))
						continue;
					
					data.draw_edges |= (1 << edge);
					
					// Shrink the edge very slightly so that when two edges share an end point,
					// the edge closer to the mouse will take priority
					edge_line.x1 = data.ex1;
					edge_line.y1 = data.ey1;
					edge_line.x2 = data.ex2;
					edge_line.y2 = data.ey2;
					const float shrink_factor = 0.002;
					const float dx = (edge_line.x2 - edge_line.x1) * shrink_factor;
					const float dy = (edge_line.y2 - edge_line.y1) * shrink_factor;
					edge_line.x1 += dx;
					edge_line.y1 += dy;
					edge_line.x2 -= dx;
					edge_line.y2 -= dy;
					
					float px, py;
					edge_line.closest_point(mx, my, px, py);
					float dist = dist_sqr(mx, my, px, py);
					
					// Prioritise the tile the mouse is inside of
					if(tx == mtx && ty == mty)
					{
						dist = max(dist - 0.01, 0.0);
					}
					
					if(dist <= radius_sqr && dist < closest_distance)
					{
						closest_distance = dist;
						closest_edge = edge;
						closest_px = px;
						closest_py = py;
						is_closest_tile = true;
					}
				} // edge
				
				if(is_closest_tile)
				{
					closest_tx = tx;
					closest_ty = ty;
					
					TileEdgeData new_data = data;
					@closest_data = new_data;
				}
				
			} // tile x
		} // tile y
		
		if(@closest_data == null)
			return;
		
		closest_data.select_edge(closest_edge);
		
		if(script.shift.down && locked_edge_mask == 0)
		{
			switch(closest_edge)
			{
				case TileEdge::Top: locked_edge_mask = TopBit; break;
				case TileEdge::Bottom: locked_edge_mask = BottomBit; break;
				case TileEdge::Left: locked_edge_mask = LeftBit; break;
				case TileEdge::Right: locked_edge_mask = RightBit; break;
			}
		}
		
		if(
			update_edges != 0 &&
			closest_data.update_edge(closest_edge, update_edges, update_collision, update_priority, update_custom))
		{
			script.g.set_tile(closest_tx, closest_ty, layer, closest_data.tile, false);
			
			// Turn off the neighbouring edge
			if(
				precision_update_neighbour &&
				closest_data.edge & (Priority | Collision) != 0)
			{
				int neighbor_tx, neighbor_ty;
				const int neighbor_edge = opposite_tile_edge(closest_edge);
				edge_adjacent_tile(closest_edge, closest_tx, closest_ty, neighbor_tx, neighbor_ty);
				tileinfo@ neightbour_tile = script.g.get_tile(neighbor_tx, neighbor_ty, layer);
				
				if(is_full_edge(neightbour_tile.type(), neighbor_edge) && neightbour_tile.solid())
				{
					uint8 neightbour_edge_bits = get_tile_edge(neightbour_tile, neighbor_edge);
					neightbour_edge_bits &= ~(Priority | Collision);
					set_tile_edge(neightbour_tile, neighbor_edge, neightbour_edge_bits);
					script.g.set_tile(neighbor_tx, neighbor_ty, layer, neightbour_tile, false);
				}
			}
		}
		
		if(render_edges && closest_data.draw_edges != 0)
		{
			while(draw_list_index + 1 >= draw_list_size)
			{
				draw_list.resize(draw_list_size *= 2);
			}
			
			@precision_edge = @closest_data;
			precision_edge_index = closest_edge;
			precision_edge_px = closest_px;
			precision_edge_py = closest_py;
			//puts(closest_edge+' '+str(precision_edge.ex1, precision_edge.ey1)+' '+str(precision_edge.ex2,precision_edge.ey2));
			@draw_list[draw_list_index++] = precision_edge;
		}
	}
	
}
