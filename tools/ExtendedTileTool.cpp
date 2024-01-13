#include '../../../lib/tiles/get_tile_quad.cpp';

class ExtendedTileTool : Tool
{
	
	private ShortcutKey pick_key;
	private bool highlight_picked_tile = false;
	
	private bool has_picked_tile;
	private uint picked_tile_shape;
	private float picked_tile_x, picked_tile_y;
	private int picked_tile_layer, picked_tile_sub_layer;
	
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
		highlight_picked_tile = script.config.get_bool('HighlightPickedTile');
		
		return Tool::reload_shortcut_key();
	}
	
	// //////////////////////////////////////////////////////////
	// Tool Callbacks
	// //////////////////////////////////////////////////////////
	
	protected void step_impl() override
	{
		has_picked_tile = false;
		
		if(script.mouse_in_scene && !script.space.down && !script.handles.mouse_over && pick_key.down())
		{
			pick_key.clear_gvb();
			
			for(int layer = 20; layer >= 6; layer--)
			{
				if(!script.editor.get_layer_visible(layer))
					continue;
				
				float layer_x, layer_y;
				script.mouse_layer(layer, 10, layer_x, layer_y);
				const int tile_x = floor_int(layer_x / 48);
				const int tile_y = floor_int(layer_y / 48);
				tileinfo@ tile = script.g.get_tile(tile_x, tile_y, layer);
				
				if(!tile.solid())
					continue;
				
				float _;
				if(!point_in_tile(layer_x, layer_y, tile_x, tile_y, tile.type(), _, _))
					continue;
				
				has_picked_tile = true;
				picked_tile_shape = tile.type();
				picked_tile_x = tile_x * 48;
				picked_tile_y = tile_y * 48;
				picked_tile_layer = layer;
				
				script.editor.set_tile_sprite(
					tile.sprite_set(), tile.sprite_tile(), tile.sprite_palette());
				script.editor.set_selected_layer(layer);
				script.show_info_popup(
					'Tile: ' + tile.sprite_set() + '.' +tile.sprite_tile() + '.' +tile.sprite_palette() + '\n' +
					'Layer: ' + layer,
					null, PopupPosition::Below, 2);
				break;
			}
		}
	}
	
	protected void draw_impl(const float sub_frame) override
	{
		if(has_picked_tile)
		{
			const float line_width = min(Settings::TilePickerLineWidth / script.zoom, 10.0);
			
			float x1, y1, x2, y2, x3, y3, x4, y4;
			get_tile_quad(picked_tile_shape, x1, y1, x2, y2, x3, y3, x4, y4);
			x1 += picked_tile_x;
			y1 += picked_tile_y;
			x2 += picked_tile_x;
			y2 += picked_tile_y;
			x3 += picked_tile_x;
			y3 += picked_tile_y;
			x4 += picked_tile_x;
			y4 += picked_tile_y;
			script.transform(x1, y1, picked_tile_layer, 10, 22, 22, x1, y1);
			script.transform(x2, y2, picked_tile_layer, 10, 22, 22, x2, y2);
			script.transform(x3, y3, picked_tile_layer, 10, 22, 22, x3, y3);
			script.transform(x4, y4, picked_tile_layer, 10, 22, 22, x4, y4);
			
			if(x1 != x2 || y1 != y2)
				script.g.draw_line_world(22, 22, x1, y1, x2, y2, line_width, Settings::TilePickerLineColour);
			if(x2 != x3 || y2 != y3)
				script.g.draw_line_world(22, 22, x2, y2, x3, y3, line_width, Settings::TilePickerLineColour);
			if(x3 != x4 || y3 != y4)
				script.g.draw_line_world(22, 22, x3, y3, x4, y4, line_width, Settings::TilePickerLineColour);
			if(x4 != x1 || y4 != y1)
				script.g.draw_line_world(22, 22, x4, y4, x1, y1, line_width, Settings::TilePickerLineColour);
			
			script.g.draw_quad_world(
				22, 22, false,
				x1, y1, x2, y2, x3, y3, x4, y4,
				Settings::TilePickerFillColour, Settings::TilePickerFillColour, Settings::TilePickerFillColour, Settings::TilePickerFillColour);
		}
	}
	
	// //////////////////////////////////////////////////////////
	// Methods
	// //////////////////////////////////////////////////////////
	
}
