// TODO:
// - Store polygon points as integers to avoid floating point error bugs.
// - Middle click to swap to erase mode.
// - Integrate with the Tile Window from the Shape Tool.
// - Use the currently selected layer.
// - Disallow self-intersecting polygons.

#include "../../../../lib/input/ModifierKey.cpp"
#include "../../../../lib/input/VK.cpp"
#include "../../../../lib/std.cpp"

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

    PenTool(AdvToolScript@ script)
    {
		super(script, 'Tiles', 'Pen Tool');
		
		init_shortcut_key(VK::W, ModifierKey::Ctrl);
    }

    void on_select_impl() override
    {
        script.hide_gui_panels(true);
    }

    void on_deselect_impl() override
    {
        polygon.clear();
        script.hide_gui_panels(false);
    }

    void step_impl() override
    {
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

        if (script.mouse.left_press)
        {
            Point mouse(script.input.mouse_x_world(19) / 48.0, script.input.mouse_y_world(19) / 48.0);
            mode.add_point(mouse);
            if (polygon.is_closed())
            {
                polygon.fill();
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
            Point mouse(script.input.mouse_x_world(19) / 48.0, script.input.mouse_y_world(19) / 48.0);
            mode.draw(mouse, this);
        }

        // Draw the existing partial polygon.
        for (uint i = 0; i < polygon.size(); ++i)
        {
            draw_point(21, 10, polygon[i], ACTIVE_COLOUR);

            if (i > 0)
            {
                draw_line(21, 10, polygon[i - 1], polygon[i], ACTIVE_COLOUR);
            }
        }
    }

    void draw_point(int layer, int sub_layer, const Point& point, uint32 colour) const
    {
        float scaled_radius = POINT_RADIUS / script.zoom;
        script.g.draw_rectangle_world(
            layer, sub_layer,
            48 * point.x - scaled_radius, 48 * point.y - scaled_radius,
            48 * point.x + scaled_radius, 48 * point.y + scaled_radius,
            0, colour
        );
    }

    void draw_line(int layer, int sub_layer, const Point& src, const Point& dst, uint32 colour) const
    {
        float scaled_width = LINE_WIDTH / script.zoom;
        script.g.draw_line_world(
            layer, sub_layer,
            48 * src.x, 48 * src.y,
            48 * dst.x, 48 * dst.y,
            scaled_width, colour
        );
    }
}
