#include '../../../../lib/ui3/elements/TileShapeGraphic.cpp'

const array<int> SHAPE_MAP = {
	 8, 16,  0,  0,
	 7, 15,  0,  0,
	11,  3, 20, 17,
	12,  4, 19, 18,
	10,  9,  1,  2,
	 6,  5, 13, 14
};

class ShapeWindow
{
	
	private AdvToolScript@ script;
	
	private Window@ window;
	private Container@ shape_container;
	private Button@ custom_button;
	
	private int _tile_shape = 0;
	private int _selection_index = 2;
	
	private bool _selecting_tiles = false;
	private bool _using_custom_shape = false;
	
	private TileShapes@ _selected_shape;
	
	int tile_shape
	{
		get const { return _tile_shape; }
		set
		{
			if(value < 0 || value > 20)
				return;
			int index = SHAPE_MAP.find(value);
			cast<Button>(shape_container.get_child(index)).selected = true;
		}
	}
	
	bool selecting_tiles { get const { return _selecting_tiles; } }
	bool using_custom_shape { get const { return _using_custom_shape; } }
	
	void create_ui()
	{
		UI@ ui = script.ui;
		Style@ style = ui.style;
		
		@window = Window(ui, 'Shape', false, true);
		window.x = 430;
		window.y = 70;
		@window.layout = FlowLayout(ui, FlowDirection::Row);
		window.layout.set_padding(0);
		
		@shape_container = Container(ui);
		
		GridLayout@ grid = GridLayout(ui, 4);
		grid.row_spacing = 0;
		grid.column_spacing = 0;
		grid.set_padding(0);
		@shape_container.layout = grid;
		
		ButtonGroup@ button_group = ButtonGroup(ui, false);
		button_group.select.on(EventCallback(on_button_select));
		
		for(int i = 0; i < 24; i++)
		{
			int tile_shape = SHAPE_MAP[i];
			TileShapeGraphic@ t = TileShapeGraphic(ui, tile_shape);
			
			Button@ b = Button(ui, t);
			b.name = '' + i;
			b.width = 48;
			b.height = 48;
			b.selectable = true;
			
			if(i == 2) 
				b.selected = true;
			
			button_group.add(b);
			shape_container.add_child(b);
		}
		
		shape_container.fit_to_contents();
		window.add_child(shape_container);
		
		Button@ select_button = Button(ui, 'Copy');
		select_button.name = 'select';
		select_button.selectable = true;
		@select_button.tooltip = PopupOptions(ui, 'Select a region', false, PopupPosition::Below);
		select_button.fit_to_contents();
		
		@custom_button = Button(ui, 'Paste');
		custom_button.name = 'custom';
		custom_button.selectable = true;
		custom_button.disabled = true;
		@custom_button.tooltip = PopupOptions(ui, 'Paste tiles', false, PopupPosition::Below);
		custom_button.fit_to_contents();
		
		button_group.add(select_button);
		button_group.add(custom_button);
		
		window.add_button_left(select_button);
		window.add_button_right(custom_button);
		
		window.fit_to_contents(true);
		script.window_manager.register_element(window);
		ui.add_child(window);
	}
	
	void show(AdvToolScript@ script)
	{
		if(@this.script == null)
		{
			@this.script = script;
			
			create_ui();
		}
		
		window.show();
	}
	
	void hide()
	{
		window.hide();
	}
	
	void move_selection(int dx, int dy)
	{
		if(_selecting_tiles || _using_custom_shape)
			return;
		
		const int x = (_selection_index % 4) + dx;
		const int y = (_selection_index / 4) + dy;
		if(x < 0 || x > 3 || y < 0 || y > 5)
			return;
		
		const int index = 4 * y + x;
		cast<Button>(shape_container.get_child(index)).selected = true;
	}
	
	void finished_selection()
	{
		custom_button.disabled = false;
		custom_button.selected = true;
	}
	
	// ///////////////////////////////////////////
	// Events
	// ///////////////////////////////////////////
	
	private void on_button_select(EventInfo@ event)
	{
		if(@event.target == null)
			return;
		
		const string name = event.target.name;
		if(name == 'select')
		{
			_selecting_tiles = true;
			_using_custom_shape = false;
		}
		else if(name == 'custom')
		{
			_selecting_tiles = false;
			_using_custom_shape = true;
		}
		else
		{
			_selecting_tiles = false;
			_using_custom_shape = false;
			_selection_index = parseInt(event.target.name);
			_tile_shape = SHAPE_MAP[_selection_index];
		}
	}
	
}
