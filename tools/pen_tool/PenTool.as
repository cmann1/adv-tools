// TODO:
// - Store polygon points as integers to avoid floating point error bugs.
// - Middle click to swap to erase mode.
// - Disallow self-intersecting polygons.
// - Add an icon!

#include "../../../../lib/input/ModifierKey.cpp"
#include "../../../../lib/input/VK.cpp"
#include "../../../../lib/std.cpp"

#include "../../settings/ShortcutKey.cpp"
#include "../shape_tool/TileWindow.cpp"
#include "../Tool.cpp"

#include "PenToolMode.as"
#include "point.as"
#include "polygon.as"

const float POINT_RADIUS = 2.0;
const float LINE_WIDTH = 2.0;

const uint ACTIVE_COLOUR = 0xff44eeff;
const uint INACTIVE_COLOUR = 0xffeeeeee;
const uint QUIET_COLOUR = 0x44888888;

class PenTool : Tool
{
    Polygon polygon;
    PenToolMode@ mode;
    TileWindow tile_window;

	private ShortcutKey pick_key;

    PenTool(AdvToolScript@ script)
    {
		super(script, 'Tiles', 'Pen Tool');
		
		init_shortcut_key(VK::W, ModifierKey::Ctrl);
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

    void on_select_impl() override
    {
		tile_window.show(script);

        script.hide_gui_panels(true);
    }

    void on_deselect_impl() override
    {
        polygon.clear();

		tile_window.hide();

        script.hide_gui_panels(false);
    }

    void step_impl() override
    {
        script.scroll_layer(true, false, true);

        if (polygon.size() >= 2 and script.shift.down)
        {
            @mode = AutoCloseMode(polygon);
        }
        else if (polygon.size() >= 1)
        {
            @mode = AngleSnapMode(polygon);
        }
        else
        {
            @mode = GridSnapMode(polygon);
        }

        if (!script.mouse_in_scene)
        {
            return;
        }

        if(pick_key.down())
        {
            pick_key.clear_gvb();
            pick_tile_at_mouse();
        }

        if (script.mouse.left_press)
        {
            Point mouse(script.input.mouse_x_world(script.layer) / 48.0, script.input.mouse_y_world(script.layer) / 48.0);
            mode.add_point(mouse);
            if (polygon.is_closed())
            {
                polygon.fill(script.layer, tile_window.sprite_set, tile_window.sprite_tile, tile_window.sprite_palette);
                polygon.clear();
            }
        }

        if (script.mouse.right_press)
        {
            if (script.shift.down)
            {
                polygon.clear();
            }
            else if (polygon.size() > 0)
            {
                polygon.remove_last();
            }
        }
    }

    void draw_impl(const float) override
    {
        // Draw the preview of the next point(s) to be added.
        if (mode !is null and script.mouse_in_scene)
        {
            Point mouse(script.input.mouse_x_world(script.layer) / 48.0, script.input.mouse_y_world(script.layer) / 48.0);
            mode.draw(mouse, this);
        }

        // Draw the existing partial polygon.
        for (uint i = 0; i < polygon.size(); ++i)
        {
            draw_point(polygon[i], ACTIVE_COLOUR);

            if (i > 0)
            {
                draw_line( polygon[i - 1], polygon[i], ACTIVE_COLOUR);
            }
        }
    }

    private void pick_tile_at_mouse()
    {
        int _, layer;
        tileinfo@ tile = script.pick_tile(_, _, layer);
        if (@tile != null)
        {
            script.editor.set_selected_layer(layer);
            tile_window.select_tile(tile.sprite_set(), tile.sprite_tile(), tile.sprite_palette());
        }
    }

    void draw_point(const Point& point, uint32 colour) const
    {
        float scaled_radius = POINT_RADIUS / script.zoom;
        float scaled_x, scaled_y;
        script.transform(48 * point.x, 48 * point.y, script.layer, 10, 22, 22, scaled_x, scaled_y);
        script.g.draw_rectangle_world(
            22, 22,
            scaled_x - scaled_radius, scaled_y - scaled_radius,
            scaled_x + scaled_radius, scaled_y + scaled_radius,
            0, colour
        );
    }

    void draw_line(const Point& src, const Point& dst, uint32 colour) const
    {
        float scaled_width = LINE_WIDTH / script.zoom;
        float scaled_src_x, scaled_src_y, scaled_dst_x, scaled_dst_y;
        script.transform(48 * src.x, 48 * src.y, script.layer, 10, 22, 22, scaled_src_x, scaled_src_y);
        script.transform(48 * dst.x, 48 * dst.y, script.layer, 10, 22, 22, scaled_dst_x, scaled_dst_y);
        script.g.draw_line_world(
            22, 22,
            scaled_src_x, scaled_src_y,
            scaled_dst_x, scaled_dst_y,
            scaled_width, colour
        );
    }
}
