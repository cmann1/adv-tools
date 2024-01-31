const string EMBED_spr_icon_depth_tool = SPRITES_BASE + 'icon_depth_tool.png';

bool same_tile_type(tileinfo@ a, tileinfo@ b)
{
	return (
		a.solid() == b.solid() &&
		a.sprite_set() == b.sprite_set() &&
		a.sprite_tile() == b.sprite_tile() &&
		a.sprite_palette() == b.sprite_palette()
	);
}

class Tile
{
	
	int x;
	int y;
	tileinfo@ info;
	
	Tile() {}
	
	Tile(int x, int y, tileinfo@ info)
	{
		this.x = x;
		this.y = y;
		@this.info = info;
	}
	
}

class DepthTool : Tool
{
	
	DepthTool(AdvToolScript@ script)
	{
		super(script, 'Tiles', 'Depth Tool');
		
		init_shortcut_key(VK::D, ModifierKey::None, -1);
	}
	
	void build_sprites(message@ msg) override
	{
		build_sprite(msg, 'icon_depth_tool');
	}
	
	void create(ToolGroup@ group) override
	{
		set_icon(SPRITE_SET, 'icon_depth_tool', 33, 33);
	}
	
	void step_impl() override
	{
		if(script.mouse.left_press && script.mouse_in_scene && !script.space.down)
		{
			int tile_x, tile_y, layer;
			if(@script.pick_tile(tile_x, tile_y, layer) != null)
			{
				move_to_front(tile_x, tile_y, layer);
			}
		}
	}
	
	void on_select_impl() override
	{
		script.hide_gui_panels(true);
	}
	
	void on_deselect_impl() override
	{
		script.hide_gui_panels(false);
	}
	
	private void move_to_front(int tile_x, int tile_y, int layer)
	{
		dictionary seen;
		array<Tile> todo;
		
		seen[tile_x + ',' + tile_y] = true;
		tileinfo@ info = script.g.get_tile(tile_x, tile_y, layer);
		todo.insertLast(Tile(tile_x, tile_y, info));
		
		while(todo.length() > 0)
		{
			Tile@ tile = todo[todo.length() - 1];
			todo.removeLast();
			
			// Move the tile to the front
			script.g.set_tile(tile.x, tile.y, layer, tile.info, true);
			
			tileinfo@ top = script.g.get_tile(tile.x, tile.y - 1, layer);
			tileinfo@ bottom = script.g.get_tile(tile.x, tile.y + 1, layer);
			tileinfo@ left = script.g.get_tile(tile.x - 1, tile.y, layer);
			tileinfo@ right = script.g.get_tile(tile.x + 1, tile.y, layer);
			
			bool same_top = same_tile_type(tile.info, top);
			bool same_bottom = same_tile_type(tile.info, bottom);
			bool same_left = same_tile_type(tile.info, left);
			bool same_right = same_tile_type(tile.info, right);
			
			if(same_top)
			{
				string key = tile.x + ',' + (tile.y - 1);
				if(!seen.exists(key))
				{
					seen[key] = true;
					todo.insertLast(Tile(tile.x, tile.y - 1, top));
				}
			}
			
			if(same_bottom)
			{
				string key = tile.x + ',' + (tile.y + 1);
				if(!seen.exists(key))
				{
					seen[key] = true;
					todo.insertLast(Tile(tile.x, tile.y + 1, bottom));
				}
			}
			
			if(same_left)
			{
				string key = (tile.x - 1) + ',' + tile.y;
				if(!seen.exists(key))
				{
					seen[key] = true;
					todo.insertLast(Tile(tile.x - 1, tile.y, left));
				}
			}
			
			if(same_right)
			{
				string key = (tile.x + 1) + ',' + tile.y;
				if(!seen.exists(key))
				{
					seen[key] = true;
					todo.insertLast(Tile(tile.x + 1, tile.y, right));
				}
			}
		}
	}
	
}
