const array<array<int>> TILE_COUNTS = {
	{3, 3, 3, 3, 3, 3, 3, 1, 1, 1, 1, 1, 1, 3, 3, 3, 1, 1, 1, 1, 1},
	{3, 3, 3, 2, 2, 2, 2, 2, 3, 1, 3, 3, 1},
	{4, 2, 2, 1, 1, 1},
	{4, 2, 1, 4, 4, 4, 4, 4, 1},
	{6, 1}
};

int get_tile_index(int sprite_set, int sprite_tile)
{
	switch (sprite_set)
	{
		case 1:
			return sprite_tile - 1;
		case 2:
			return 21 + sprite_tile - 1;
		case 3:
			return 21 + 13 + sprite_tile - 1;
		case 4:
			return 21 + 13 + 6 + sprite_tile - 1;
		case 5:
			return 21 + 13 + 6 + 9 + sprite_tile - 1;
	}
	return -1;
}

class PaletteMenu : Container
{
	
	private int _sprite_set = 1;
	private int _sprite_tile = 1;
	private int _sprite_palette = 0;
	
	int sprite_palette
	{
		get const { return _sprite_palette; }
		set
		{
			if(value == _sprite_palette)
				return;
			if(value < 0 || value >= TILE_COUNTS[_sprite_set-1][_sprite_tile-1])
				return;
			cast<Button>(get_child(value)).selected = true;
		}
	}
	
	private ButtonGroup@ button_group;
	
	PaletteMenu(UI@ ui)
	{
		super(ui);
		
		@layout = FlowLayout(ui, FlowDirection::Row);
		width = 72 + 2 * ui.style.spacing;
		
		@button_group = ButtonGroup(ui, false);
		button_group.select.on(EventCallback(on_palette_select));
		
		create_ui();
	}
	
	void select_tile(int sprite_set, int sprite_tile)
	{
		if(sprite_set == _sprite_set && sprite_tile == _sprite_tile)
			return;
		
		_sprite_set = sprite_set;
		_sprite_tile = sprite_tile;
		_sprite_palette = int(min(_sprite_palette, TILE_COUNTS[sprite_set-1][sprite_tile-1] - 1));
		
		create_ui();
	}
	
	private void create_ui()
	{
		clear();
		
		for(int sprite_palette = 0; sprite_palette < TILE_COUNTS[_sprite_set-1][_sprite_tile-1]; sprite_palette++)
		{
			Image@ t = Image(ui, SPRITE_SET, 'tile_' + _sprite_set + '_' + _sprite_tile + '_' + (sprite_palette + 1), 96, 96);
			t.width = 72;
			t.height = 72;
			t.sizing = ImageSize::None;
			
			Button@ b = Button(ui, t);
			b.name = '' + sprite_palette;
			b.width = 72;
			b.height = 72;
			b.selectable = true;
			
			if(sprite_palette == _sprite_palette)
				b.selected = true;
			
			button_group.add(b);
			add_child(b);
		}
	}
	
	private void on_palette_select(EventInfo@ event)
	{
		if(@event.target == null)
			return;
		
		int sprite_palette = parseInt(event.target.name);
		_sprite_palette = sprite_palette;
	}
	
}


class TileWindow : Window
{
	
	private int _sprite_set = 1;
	private int _sprite_tile = 1;
	
	int sprite_set { get const { return _sprite_set; } }
	int sprite_tile { get const { return _sprite_tile; } }
	int sprite_palette
	{
		get const { return palette_menu.sprite_palette; }
		set { palette_menu.sprite_palette = value; }
	}
	
	private Container@ tile_container;
	private PaletteMenu@ palette_menu;
	
	TileWindow(UI@ ui)
	{
		super(ui, 'Tile', false);
		
		@layout = FlowLayout(ui, FlowDirection::Column);
		
		ScrollView@ tile_view = ScrollView(ui);
		tile_view.x = 0;
		tile_view.y = 0;
		tile_view.width = 3 * 72 + 5 * ui.style.spacing + ui.style.default_scrollbar_size;
		tile_view.height = 6 * 72 + 7 * ui.style.spacing + 12 * EPSILON; // was having trouble with the bottom row
		tile_view.scroll_amount = 72 + ui.style.spacing - EPSILON;
		
		@tile_container = tile_view.content;
		
		GridLayout@ grid = GridLayout(ui, 3);
		@tile_container.layout = grid;
		
		ButtonGroup@ button_group = ButtonGroup(ui, false);
		button_group.select.on(EventCallback(on_tile_select));
		
		for(uint sprite_set = 1; sprite_set <= TILE_COUNTS.length(); sprite_set++)
		{
			for(uint sprite_tile = 1; sprite_tile <= TILE_COUNTS[sprite_set-1].length(); sprite_tile++)
			{
				Image@ t = Image(ui, SPRITE_SET, 'tile_' + sprite_set + '_' + sprite_tile + '_1', 96, 96);
				t.width = 72;
				t.height = 72;
				t.sizing = ImageSize::None;
				
				Button@ b = Button(ui, t);
				b.name = sprite_set + '_' + sprite_tile;
				b.width = 72;
				b.height = 72;
				b.selectable = true;
				
				button_group.add(b);
				tile_container.add_child(b);
				
				if(sprite_set == 1 && sprite_tile == 1)
					b.selected = true;
			}
		}
		
		@palette_menu = PaletteMenu(ui);
		
		add_child(tile_view);
		add_child(palette_menu);
		
		fit_to_contents(true);
	}
	
	void select_tile(int sprite_set, int sprite_tile)
	{
		if(sprite_set == _sprite_set && sprite_tile == _sprite_tile)
			return;
		if(sprite_set < 1 || sprite_set > 5)
			return;
		if(sprite_tile < 1 || sprite_tile > int(TILE_COUNTS[sprite_set-1].length()))
			return;
		
		int index = get_tile_index(sprite_set, sprite_tile);
		cast<Button>(tile_container.get_child(index)).selected = true;
	}
	
	// ///////////////////////////////////////////
	// Events
	// ///////////////////////////////////////////
	
	private void on_tile_select(EventInfo@ event)
	{
		if(@event.target == null)
			return;
		
		array<string> sprite_info = event.target.name.split('_');
		
		int sprite_set = parseInt(sprite_info[0]);
		int sprite_tile = parseInt(sprite_info[1]);
		
		if(sprite_set == _sprite_set && sprite_tile == _sprite_tile)
			return;
		
		_sprite_set = sprite_set;
		_sprite_tile = sprite_tile;
		
		palette_menu.select_tile(sprite_set, sprite_tile);
	}
	
}
