class TileShapes
{
	private int _width;
	private int _height;

	private int _origin_x;
	private int _origin_y;

	int width { get const { return _width; } }
	int height { get const { return _height; } }

	private array<array<int>> _shapes;
	
	TileShapes(int width, int height)
	{
		_width = width;
		_height = height;
		_origin_x = width / 2;
		_origin_y = height / 2;

		clear();
	}

	private void clear()
	{
		_shapes = array<array<int>>(height, array<int>(width, -1));
	}

	void load(scene@ g, int layer, int tx, int ty)
	{
		clear();

		for(int row=0; row<_height; ++row)
		{
			for(int col=0; col<_width; ++col)
			{
				tileinfo@ tile = g.get_tile(tx + col, ty + row, layer);
				if(tile is null or not tile.solid()) continue;
				_shapes[row][col] = tile.type();
			}
		}
	}

	void place(scene@ g, int tx, int ty, int layer, bool solid, int sprite_set, int sprite_tile, int sprite_palette)
	{
		tx -= _origin_x;
		ty -= _origin_y;

		for(int row=0; row<_height; ++row)
		{
			for(int col=0; col<_width; ++col)
			{
				const int shape = _shapes[row][col];
				if(shape == -1) continue;

				g.set_tile(
					tx + col, ty + row,
					layer, solid, shape,
					sprite_set, sprite_tile, sprite_palette
				);
			}
		}
	}

	void draw(scene@ g, float x, float y, float scale_x, float scale_y, uint fill, uint outline)
	{
		const float tile_width = 48 * scale_x;
		const float tile_height = 48 * scale_y;

		x -= tile_width  * _origin_x;
		y -= tile_height * _origin_y;

		for(int row=0; row<_height; ++row)
		{
			for(int col=0; col<_width; ++col)
			{
				const int shape = _shapes[row][col];
				if(shape == -1) continue;

				draw_tile_shape(shape, g, 22, 22, x + tile_width * col, y + tile_height * row, scale_x, scale_y, fill, outline);
			}
		}
	}
}
