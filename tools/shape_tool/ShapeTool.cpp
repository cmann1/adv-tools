#include '../../../../lib/drawing/tile.cpp'
#include '../../../../lib/tiles/common.cpp'

#include 'TileEmbeds.cpp'
#include 'TileWindow.cpp'
#include 'ShapeWindow.cpp'
#include 'TileShapes.cpp'

const string EMBED_spr_icon_shape_tool = SPRITES_BASE + 'icon_shape_tool.png';

class ShapeTool : Tool
{
	
	private Mouse@ mouse;
	private TileWindow@ tile_window;
	private ShapeWindow@ shape_window;
	
	private bool dragging;
	private float drag_start_x, drag_start_y;
	private TileShapes@ custom_shape;
	
	ShapeTool(AdvToolScript@ script)
	{
		super(script, 'Tiles', 'Shape Tool');
		
		init_shortcut_key(VK::W, ModifierKey::Alt, -1);
	}
	
	void create(ToolGroup@ group) override
	{
		set_icon(SPRITE_SET, 'icon_shape_tool', 40, 40);
	}
	
	void on_init() override
	{
		@mouse = script.mouse;
		
		UI@ ui = script.ui;
		
		@tile_window = TileWindow(ui);
		tile_window.x = 90;
		tile_window.y = 70;
		ui.add_child(tile_window);
		script.window_manager.register_element(tile_window);
		
		@shape_window = ShapeWindow(ui);
		shape_window.x = 460;
		shape_window.y = 70;
		ui.add_child(shape_window);
		script.window_manager.register_element(shape_window);
	}

	void build_sprites(message@ msg) override
	{
		build_sprite(msg, 'icon_shape_tool');
		build_tile_sprites(msg);
	}

	private void set_tile_at_mouse(bool solid)
	{
		const int layer = script.editor.get_selected_layer();
		if(layer <= 5 || layer > 20)
			return;
		
		float mx, my;
		script.transform(mouse.x, mouse.y, 19, 10, layer, 10, mx, my);
		const int tile_x = int(floor(mx / 48));
		const int tile_y = int(floor(my / 48));
		
		if(shape_window.using_custom_shape)
		{
			if(@custom_shape != null)
			{
				custom_shape.place(
					script.g,
					tile_x,
					tile_y,
					layer,
					solid,
					tile_window.sprite_set,
					tile_window.sprite_tile,
					tile_window.sprite_palette
				);
			}
		}
		else
		{
			script.g.set_tile(
				tile_x,
				tile_y,
				layer,
				solid,
				shape_window.tile_shape,
				tile_window.sprite_set,
				tile_window.sprite_tile,
				tile_window.sprite_palette
			);
		}
	}

	private void pick_tile_at_mouse()
	{
		for(int layer = 20; layer >= 6; layer--)
		{
			if(!script.editor.get_layer_visible(layer))
				continue;
			
			const float mx = script.g.mouse_x_world(0, layer);
			const float my = script.g.mouse_y_world(0, layer);
			
			const int tile_x = int(floor(mx / 48));
			const int tile_y = int(floor(my / 48));
			
			tileinfo@ tile = script.g.get_tile(tile_x, tile_y, layer);
			
			float _;
			if(tile.solid() && point_in_tile(mx, my, tile_x, tile_y, tile.type(), _, _))
			{
				script.editor.set_selected_layer(layer);
				tile_window.select_tile(tile.sprite_set(), tile.sprite_tile());
				tile_window.sprite_palette = tile.sprite_palette();
				shape_window.tile_shape = tile.type();
				break;
			}
		}
	}
	
	private void change_layer(int delta)
	{
		int layer = max(6, min(20, script.editor.get_selected_layer() + delta));
		if(layer == 18)
			layer += sign(delta);
		
		script.editor.set_selected_layer(layer);
	}
	
	private void draw_selection()
	{
		const int layer = script.editor.get_selected_layer();
		int tx1, ty1, tx2, ty2;
		fix_tile_selection(
			drag_start_x,
			drag_start_y,
			script.g.mouse_x_world(0, layer),
			script.g.mouse_y_world(0, layer),
			tx1, ty1, tx2, ty2
		);
		
		float x1, y1, x2, y2;
		script.transform(48 * tx1, 48 * ty1, layer, 10, 19, 10, x1, y1);
		script.transform(48 * tx2, 48 * ty2, layer, 10, 19, 10, x2, y2);
		
		script.g.draw_rectangle_world(22, 22, x1, y1, x2, y2, 0, Settings::HoveredFillColour);
		outline_rect(script.g, 22, 22, x1, y1, x2, y2, 1, Settings::HoveredLineColour);
	}
	
	// //////////////////////////////////////////////////////////
	// Callbacks
	// //////////////////////////////////////////////////////////
	
	protected void on_select_impl() override
	{
		script.editor.hide_panels_gui(true);
		
		tile_window.show();
		shape_window.show();
	}
	
	protected void on_deselect_impl() override
	{
		script.editor.hide_panels_gui(false);
		
		tile_window.hide();
		shape_window.hide();
	}
	
	protected void step_impl() override
	{
		if(script.mouse_in_scene && !script.space.down)
		{
			if(!shape_window.selecting_tiles)
			{
				if(mouse.left_down) set_tile_at_mouse(true);
				if(mouse.right_down) set_tile_at_mouse(false);
			}
			
			if(mouse.middle_press) pick_tile_at_mouse();
			
			if(script.ctrl.down && mouse.scroll != 0) change_layer(mouse.scroll);
			
			// Drag start
			if(mouse.left_press)
			{
				dragging = true;
				const int layer = script.editor.get_selected_layer();
				drag_start_x = script.g.mouse_x_world(0, layer);
				drag_start_y = script.g.mouse_y_world(0, layer);
			}
			
			// Drag end
			if(dragging && mouse.left_release)
			{
				dragging = false;
				if(shape_window.selecting_tiles)
				{
					const int layer = script.editor.get_selected_layer();
					
					int tx1, ty1, tx2, ty2;
					fix_tile_selection(
						drag_start_x,
						drag_start_y,
						script.g.mouse_x_world(0, layer),
						script.g.mouse_y_world(0, layer),
						tx1, ty1, tx2, ty2
					);
					
					@custom_shape = TileShapes(tx2 - tx1, ty2 - ty1);
					custom_shape.load(script.g, layer, tx1, ty1);
					
					shape_window.finished_selection();
				}
			}
		}
		
		if(mouse.left_release) dragging = false;
		
		if(script.scene_focus)
		{
			if(script.key_repeat_gvb(GVB::LeftArrow )) shape_window.move_selection(-1,  0);
			if(script.key_repeat_gvb(GVB::RightArrow)) shape_window.move_selection( 1,  0);
			if(script.key_repeat_gvb(GVB::UpArrow   )) shape_window.move_selection( 0, -1);
			if(script.key_repeat_gvb(GVB::DownArrow )) shape_window.move_selection( 0,  1);
		}
	}
	
	protected void draw_impl(const float sub_frame) override
	{
		if(script.space.down || !script.mouse_in_scene)
			return;
		
		const int layer = script.editor.get_selected_layer();
		if(layer <= 5 || layer > 20)
			return;
		
		float mx, my;
		script.transform(mouse.x, mouse.y, 19, 10, layer, 10, mx, my);
		mx = 48 * floor(mx / 48);
		my = 48 * floor(my / 48);
		script.transform(mx, my, layer, 10, 19, 10, mx, my);
		
		float sx, sy;
		script.transform_size(1, 1, layer, 10, 19, 10, sx, sy);
		
		if(shape_window.selecting_tiles)
		{
			if(dragging)
			{
				draw_selection();
			}
		}
		else if(shape_window.using_custom_shape && @custom_shape != null)
		{
			custom_shape.draw(
				script.g,
				mx, my,
				sx, sy,
				Settings::HoveredFillColour, Settings::HoveredLineColour
			);
		}
		else
		{
			draw_tile_shape(
				shape_window.tile_shape,
				script.g,
				22, 22,
				mx, my,
				sx, sy,
				Settings::HoveredFillColour, Settings::HoveredLineColour
			);
		}
	}
	
}

void fix_tile_selection(float x1, float y1, float x2, float y2, int &out tx1, int &out ty1, int &out tx2, int &out ty2)
{
	if(x1 <= x2)
	{
		tx1 = int(floor(x1 / 48));
		tx2 = int( ceil(x2 / 48));
	}
	else
	{
		tx1 = int(floor(x2 / 48));
		tx2 = int( ceil(x1 / 48));
	}

	if(y1 <= y2)
	{
		ty1 = int(floor(y1 / 48));
		ty2 = int( ceil(y2 / 48));
	}
	else
	{
		ty1 = int(floor(y2 / 48));
		ty2 = int( ceil(y1 / 48));
	}
}
