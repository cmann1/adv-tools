#include '../../../../lib/math/IntVec2.cpp'
#include '../../../../lib/std.cpp'

namespace Polygon
{
	
	const bool DEBUG = false;
	
	const uint TILE_EMPTY = MAX_UINT;
	
	class EdgeData
	{
		
		int offset_x;
		int offset_y;
		bool facing_left;
		bool second_facing_left;
		bool facing_right;
		bool second_facing_right;
		uint tile_type;
		uint second_tile_type;
		
		EdgeData(
			int offset_x,
			int offset_y,
			bool facing_left,
			bool second_facing_left,
			bool facing_right,
			bool second_facing_right,
			uint tile_type,
			uint second_tile_type
		) {
			this.offset_x = offset_x;
			this.offset_y = offset_y;
			this.facing_left = facing_left;
			this.second_facing_left = second_facing_left;
			this.facing_right = facing_right;
			this.second_facing_right = second_facing_right;
			this.tile_type = tile_type;
			this.second_tile_type = second_tile_type;
		}
	}
	
	const dictionary EDGE_DATA = {
		{ '1,0' , EdgeData( 0,  0, false, false, false, false, TileShape::Full  , TILE_EMPTY       )},
		{ '2,1' , EdgeData( 0,  0, false, false, false,  true, TileShape::Big1  , TileShape::Small1)},
		{ '1,1' , EdgeData( 0,  0, false, false,  true,  true, TileShape::HalfA , TILE_EMPTY       )},
		{ '1,2' , EdgeData( 0,  0, false, false,  true,  true, TileShape::Small8, TileShape::Big8  )},
		{ '0,1' , EdgeData(-1,  0, false, false,  true,  true, TileShape::Full  , TILE_EMPTY       )},
		{'-1,2' , EdgeData(-1,  0, false, false,  true,  true, TileShape::Big2  , TileShape::Small2)},
		{'-1,1' , EdgeData(-1,  0, false, false,  true,  true, TileShape::HalfB , TILE_EMPTY       )},
		{'-2,1' , EdgeData(-1,  0, false, false,  true, false, TileShape::Small7, TileShape::Big7  )},
		{'-1,0' , EdgeData(-1, -1, false, false, false, false, TileShape::Full  , TILE_EMPTY       )},
		{'-2,-1', EdgeData(-1, -1, false,  true, false, false, TileShape::Big3  , TileShape::Small3)},
		{'-1,-1', EdgeData(-1, -1,  true,  true, false, false, TileShape::HalfC , TILE_EMPTY       )},
		{'-1,-2', EdgeData(-1, -1,  true,  true, false, false, TileShape::Small6, TileShape::Big6  )},
		{ '0,-1', EdgeData( 0, -1,  true,  true, false, false, TileShape::Full  , TILE_EMPTY       )},
		{ '1,-2', EdgeData( 0, -1,  true,  true, false, false, TileShape::Big4  , TileShape::Small4)},
		{ '1,-1', EdgeData( 0, -1,  true,  true, false, false, TileShape::HalfD , TILE_EMPTY       )},
		{ '2,-1', EdgeData( 0, -1,  true, false, false, false, TileShape::Small5, TileShape::Big5  )}
	};
	
	/** Each bit represents a point on the tile that may be covered:
	 *
	 *  0--1--2
	 *  |     |
	 *  7     3
	 *  |     |
	 *  6--5--4
	 *
	 */
	const array<uint> TILE_BITMASKS = {
		0b11111111,  // FULL
		0b10011111,  // BIG_1
		0b00001111,  // SMALL_1
		0b11100111,  // BIG_2
		0b11000011,  // SMALL_2
		0b11111001,  // BIG_3
		0b11110000,  // SMALL_3
		0b01111110,  // BIG_4
		0b00111100,  // SMALL_4
		0b00111111,  // BIG_5
		0b00011110,  // SMALL_5
		0b11111100,  // BIG_6
		0b01111000,  // SMALL_6
		0b11110011,  // BIG_7
		0b11100001,  // SMALL_7
		0b11001111,  // BIG_8
		0b10000111,  // SMALL_8
		0b10001111,  // HALF_A
		0b11100011,  // HALF_B
		0b11111000,  // HALF_C
		0b00111110   // HALF_D
	};
	
	uint tile_type_from_bitmask(uint mask)
	{
		// Check if the mask exactly matches any regular tile.
		for(uint type = 0; type <= 20; ++type)
			if(mask == TILE_BITMASKS[type])
				return type;
		
		// If half of the bits are set, return a full tile.
		// This accounts for special cases like two overlapping small tiles.
		uint bit_count = 0;
		while(mask > 0)
		{
			bit_count += mask & 1;
			mask >>= 1;
		}
		if(bit_count >= 4)
			return TileShape::Full;
		
		// Otherwise, this is an invalid/empty tile.
		return TILE_EMPTY;
	}
	
	class BorderTile
	{
		
		uint mask = 0b11111111;
		
		bool facing_left = false;
		bool facing_right = false;
		
	}
	
}


class Polygon
{
	
	private array<IntVec2> points;
	private bool closed;
	
	uint length { get { return points.length; } }
	
	const IntVec2& opIndex(int i) const
	{
		return points[i];
	}
	
	bool is_closed() const
	{
		return closed;
	}
	
	void insert_last(const IntVec2& point)
	{
		if(closed)
			return;
		
		// If we have a polygon and the new point is identical to the first point, the polygon is now closed.
		if(points.length >= 3 && point == points[0])
		{
			// Don't add the duplicate point.
			closed = true;
		}
		
		// If the new point is identical to the second-last point, remove the last point.
		else if(points.length >= 2 && point == points[points.length - 2])
		{
			points.removeLast();
		}
		
		// If the new point is collinear with the last two, move the last point.
		else if(points.length >= 2 && orientation(point, points[points.length - 1], points[points.length - 2]) == 0)
		{
			points[points.length - 1] = point;
		}
		
		// If the new point is different to the last point, add the new point.
		else if(points.length == 0 || point != points[0])
		{
			points.insertLast(point);
		}
	}
	
	void remove_last()
	{
		points.removeLast();
		closed = false;
	}
	
	void clear()
	{
		points.resize(0);
		closed = false;
	}
	
	int twice_signed_area() const
	{
		// Shoelace formula.
		int twice_area = 0;
		for(uint i = 0; i < points.length; ++i)
		{
			twice_area += cross_product_z(points[i], points[(i + 1) % points.length]);
		}
		return twice_area;
	}
	
	bool is_clockwise() const
	{
		return twice_signed_area() > 0;
	}
	
	void fill(int layer, int sprite_set, int sprite_tile, int sprite_palette)
	{
		scene@ g = get_scene();
		
		// If the last point is collinear with the first two, remove the first point.
		// This prevents the polygon from doubling back on itself.
		if(points.length >= 3 && orientation(points[points.length - 1], points[0], points[1]) == 0)
		{
			points.removeAt(0);
		}
		
		// Ensure that our polygon winds clockwise.
		if(!is_clockwise())
		{
			points.reverse();
		}
		
		// Keep track of the furthest right point, to use as a failsafe when filling tiles.
		int max_x = MIN_INT;
		
		// Follow the polygon to calculate the tiles along the border.
		dictionary border;  // 'x,y' -> Tile
		for(uint i = 0; i < points.length; ++i)
		{
			IntVec2@ a = points[i];
			IntVec2@ b = points[(i + 1) % points.length];
			
			if(a.x > max_x)
			{
				max_x = a.x;
			}
			
			int dx = b.x - a.x;
			int dy = b.y - a.y;
			uint steps = gcd(dx, dy);
			if(steps == 0)
				continue;
			int step_dx = dx / steps;
			int step_dy = dy / steps;
			
			const Polygon::EdgeData@ edge_data = cast<Polygon::EdgeData>(Polygon::EDGE_DATA[step_dx + ',' + step_dy]);
			bool is_slant = edge_data.second_tile_type != Polygon::TILE_EMPTY;
			
			for(uint step = 0; step < steps; ++step)
			{
				int x = a.x + step * step_dx + edge_data.offset_x;
				int y = a.y + step * step_dy + edge_data.offset_y;
				string key = x + ',' + y;
				Polygon::BorderTile tile;
				border.get(key, tile);
				tile.mask &= Polygon::TILE_BITMASKS[edge_data.tile_type];
				tile.facing_left  = tile.facing_left  || edge_data.facing_left;
				tile.facing_right = tile.facing_right || edge_data.facing_right;
				border.set(key, tile);
				
				if(is_slant)
				{
					int second_x = x + step_dx / 2;
					int second_y = y + step_dy / 2;
					string second_key = second_x + ',' + second_y;
					Polygon::BorderTile second_tile;
					border.get(second_key, second_tile);
					second_tile.mask &= Polygon::TILE_BITMASKS[edge_data.second_tile_type];
					second_tile.facing_left  = second_tile.facing_left  || edge_data.second_facing_left;
					second_tile.facing_right = second_tile.facing_right || edge_data.second_facing_right;
					border.set(second_key, second_tile);
				}
			}
		}
		
		if(Polygon::DEBUG)
		{
			array<string> border_keys = border.getKeys();
			border_keys.sortAsc();
			for(uint i = 0; i < border_keys.length; ++i)
			{
				string key = border_keys[i];
				Polygon::BorderTile@ tile = cast<Polygon::BorderTile>(border[key]);
				puts(key + ' -> ' + tile.mask + ' ' + tile.facing_left + ' ' + tile.facing_right);
			}
		}
		
		// For every left-facing tile, scan and fill until we hit a right-facing tile.
		array<string>@ border_coordinates = border.getKeys();
		for(uint i = 0; i < border_coordinates.length; ++i)
		{
			string border_coordinate = border_coordinates[i];
			Polygon::BorderTile@ border_tile = cast<Polygon::BorderTile>(border[border_coordinate]);
			if(!border_tile.facing_left)
				continue;
			
			array<string> split_coordinate = border_coordinate.split(',');
			int x = parseInt(split_coordinate[0]);
			int y = parseInt(split_coordinate[1]);
			
			Polygon::BorderTile@ tile;
			do
			{
				border.get(x + ',' + y, @tile);
				
				// If this tile is on the border, use the tile shape from the mask.
				uint type;
				if(@tile != null)
				{
					uint mask = tile.mask;
					
					// Merge the new tile shape with the existing tile shape.
					tileinfo@ existing_tile = g.get_tile(x, y, layer);
					if(existing_tile.solid())
					{
						uint existing_mask = Polygon::TILE_BITMASKS[existing_tile.type()];
						mask |= existing_mask;
					}
					
					type = Polygon::tile_type_from_bitmask(mask);
				}
				
				// Otherwise, just fill the whole tile.
				else
				{
					type = TileShape::Full;
				}
				
				if(Polygon::DEBUG)
				{
					puts(x + ',' + y + ' => ' + type);
				}
				
				// Do nothing if the tile has been eroded away completely.
				if(type != Polygon::TILE_EMPTY)
				{
					g.set_tile(x, y, layer, true, type, sprite_set, sprite_tile, sprite_palette);
				}
				
				x += 1;
			}
			while((@tile == null || !tile.facing_right) && x < max_x);
		}
	}
	
}
