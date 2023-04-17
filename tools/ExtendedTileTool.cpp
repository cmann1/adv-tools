class ExtendedTileTool : Tool
{
	
	private ShortcutKey pick_key;
	
	ExtendedTileTool(AdvToolScript@ script)
	{
		super(script, 'Tiles');
		
		init_shortcut_key(VK::W);
	}
	
	void create(ToolGroup@ group) override
	{
		set_icon('editor', 'tilesicon');
	}
	
	void on_init() override
	{
		pick_key.init(script);
		reload_shortcut_key();
	}
	
	bool reload_shortcut_key() override
	{
		pick_key.from_config('KeyPickTile', 'MiddleClick');
		
		return Tool::reload_shortcut_key();
	}
	
	// //////////////////////////////////////////////////////////
	// Tool Callbacks
	// //////////////////////////////////////////////////////////
	
	protected void step_impl() override
	{
		if(script.mouse_in_scene && !script.space.down && !script.handles.mouse_over && pick_key.down())
		{
			pick_key.clear_gvb();
			
			for(int layer = 20; layer >= 6; layer--)
			{
				if(!script.editor.get_layer_visible(layer))
					continue;
				
				float layer_x, layer_y;
				script.mouse_layer(layer, layer_x, layer_y);
				const int tile_x = floor_int(layer_x / 48);
				const int tile_y = floor_int(layer_y / 48);
				tileinfo@ tile = script.g.get_tile(tile_x, tile_y, layer);
				
				if(tile.solid())
				{
					script.editor.set_tile_sprite(
						tile.sprite_set(), tile.sprite_tile(), tile.sprite_palette());
					script.show_info_popup(
						'Tile: ' + tile.sprite_set() + '.' +tile.sprite_tile() + '.' +tile.sprite_palette(),
						null, PopupPosition::Below, 2);
					break;
				}
			}
		}
	}
	
	// //////////////////////////////////////////////////////////
	// Methods
	// //////////////////////////////////////////////////////////
	
}
