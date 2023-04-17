class TileEdgeData
{
	
	tileinfo@ tile;
	int type;
	bool solid;
	uint8 edge_top;
	uint8 edge_bottom;
	uint8 edge_left;
	uint8 edge_right;
	
	/// Bit flags marking edges that are valid and can be updated
	uint valid_edges;
	/// Bit flags marking edges that must be drawn
	uint draw_edges;
	/// Bit flags marking which edges have been updated this pass
	uint updated_edges;
	
	bool has_reset;
	
	int selected_edge;
	/// The currently selected edge points
	float ex1, ey1, ex2, ey2;
	/// The currently selected edge bits
	uint edge;
	
	float x1, y1;
	float x2, y2;
	float x3, y3;
	float x4, y4;
	
	uint8 edges_facing;
	
	void init(scene@ g, const int tx, const int ty, const int layer,
		const uint edge_mask, const bool check_internal_sprites,
		const bool update_vertices=true)
	{
		@tile = g.get_tile(tx, ty, layer);
		solid = tile.solid();

		if(!solid)
			return;

		type = tile.type();
		edge_top = tile.edge_top();
		edge_bottom = tile.edge_bottom();
		edge_left = tile.edge_left();
		edge_right = tile.edge_right();
		
		valid_edges = edge_mask & (
			(is_valid_edge(type, TileEdge::Top) ? 0x1 : 0) |
			(is_valid_edge(type, TileEdge::Bottom) ? 0x2 : 0) |
			(is_valid_edge(type, TileEdge::Left) ? 0x4 : 0) |
			(is_valid_edge(type, TileEdge::Right) ? 0x8 : 0)
		);
		edges_facing =
			(is_external_edge(g, layer, tx, ty, tile, type, TileEdge::Top, check_internal_sprites)    ? 0x1 : 0) |
			(is_external_edge(g, layer, tx, ty, tile, type, TileEdge::Bottom, check_internal_sprites) ? 0x2 : 0) |
			(is_external_edge(g, layer, tx, ty, tile, type, TileEdge::Left, check_internal_sprites)   ? 0x4 : 0) |
			(is_external_edge(g, layer, tx, ty, tile, type, TileEdge::Right, check_internal_sprites)  ? 0x8 : 0); 
		
		if(update_vertices)
		{
			const float x = tx * 48;
			const float y = ty * 48;
			get_tile_quad(type,
				x1, y1, x2, y2,
				x3, y3, x4, y4);
			x1 += x; y1 += y;
			x2 += x; y2 += y;
			x3 += x; y3 += y;
			x4 += x; y4 += y;
		}
	}
	
	void select_edge(const int edge)
	{
		selected_edge = edge;
		
		switch(edge)
		{
			case TileEdge::Top:
				ex1 = x1; ey1 = y1;
				ex2 = x2; ey2 = y2;
				this.edge = edge_top;
				break;
			case TileEdge::Bottom:
				ex1 = x3; ey1 = y3;
				ex2 = x4; ey2 = y4;
				this.edge = edge_bottom;
				break;
			case TileEdge::Left:
				ex1 = x4; ey1 = y4;
				ex2 = x1; ey2 = y1;
				this.edge = edge_left;
				break;
			case TileEdge::Right:
				ex1 = x2; ey1 = y2;
				ex2 = x3; ey2 = y3;
				this.edge = edge_right;
				break;
		}
	}
	
	bool update_edge(const int edge, const int update_edges, const bool update_collision, const bool update_priority, const uint8 update_custom)
	{
		updated_edges |= (1 << edge);
		
		const uint8 old_edge_bits = this.edge;
		
		if(update_collision)
		{
			if(update_edges == -1)
			{
				this.edge &= ~EdgeFlags::Collision;
			}
			else if(update_edges == 1)
			{
				this.edge |= EdgeFlags::Collision;
			}
		}
		
		if(update_priority)
		{
			if(update_edges == -1)
			{
				this.edge &= ~EdgeFlags::Priority;
			}
			else if(update_edges == 1)
			{
				this.edge |= EdgeFlags::Priority;
			}
		}
		
		if(update_custom != 0)
		{
			if(update_edges == -1)
			{
				this.edge &= ~update_custom;
			}
			else if(update_edges == 1)
			{
				this.edge |= update_custom;
			}
		}
		
		if(this.edge != old_edge_bits)
		{
			switch(edge)
			{
				case TileEdge::Top:
					tile.edge_top(edge_top = this.edge);
					break;
				case TileEdge::Bottom:
					tile.edge_bottom(edge_bottom = this.edge);
					break;
				case TileEdge::Left:
					tile.edge_left(edge_left = this.edge);
					break;
				case TileEdge::Right:
					tile.edge_right(edge_right = this.edge);
					break;
			}
			
			return true;
		}
		
		return false;
	}
	
	uint get_colour()
	{
		const bool collision_on = edge & Collision != 0;
		const bool priority_on = edge & Priority != 0;
		
		if(collision_on && priority_on)
			return Settings::EdgeOnColour;
		if(collision_on && !priority_on)
			return Settings::EdgeInvisibleColour;
		
		return priority_on ? Settings::EdgeVisibleColour : Settings::EdgeOffColour;
	}
	
}
