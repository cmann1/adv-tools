// TODO:
// - Erase mode. Maybe hold Ctrl when closing shape and turn outline red?
// - Disallow self-intersecting polygons.

#include "../../../../lib/embed_utils.cpp"
#include "../../../../lib/input/ModifierKey.cpp"
#include "../../../../lib/input/VK.cpp"
#include "../../../../lib/math/Vec2.cpp"
#include "../../../../lib/std.cpp"

#include "../../settings/ShortcutKey.cpp"
#include "../../ToolGroup.cpp"
#include "../shape_tool/TileWindow.cpp"
#include "../shape_tool/TileEmbeds.cpp"
#include "../Tool.cpp"

#include "PenToolMode.as"
#include "Polygon.as"

const string EMBED_spr_icon_pen_tool = SPRITES_BASE + 'icon_pen_tool.png';

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
	
	void create(ToolGroup@ group) override
	{
		set_icon(SPRITE_SET, 'icon_pen_tool', 60, 60);
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

	void build_sprites(message@ msg) override
	{
		build_sprite(msg, 'icon_pen_tool');
		build_tile_sprites(msg);
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
            Vec2 mouse(script.input.mouse_x_world(script.layer) / 48.0, script.input.mouse_y_world(script.layer) / 48.0);
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
            Vec2 mouse(script.input.mouse_x_world(script.layer) / 48.0, script.input.mouse_y_world(script.layer) / 48.0);
            mode.draw(mouse, this);
        }

        // Draw the existing partial polygon.
        for (uint i = 0; i < polygon.size(); ++i)
        {
            draw_point(polygon[i], ACTIVE_COLOUR);

            if (i > 0)
            {
                draw_line(polygon[i - 1], polygon[i], ACTIVE_COLOUR);
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

    void draw_point(const IntVec2& point, uint32 colour) const
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

    void draw_line(const IntVec2& src, const IntVec2& dst, uint32 colour) const
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
