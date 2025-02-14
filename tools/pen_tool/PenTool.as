// TODO:
// - Store polygon points as integers to avoid floating point error bugs.
// - Figure out how I want to draw the target points for the different modes.
// - Middle click to swap to erase mode.

#include "../../../../lib/input/ModifierKey.cpp"
#include "../../../../lib/input/VK.cpp"
#include "../../../../lib/std.cpp"

#include "../Tool.cpp"

#include "point.as"
#include "polygon.as"

const int GVB_LEFT_CLICK = 2;
const int GVB_RIGHT_CLICK = 3;
const int GVB_SPACE = 8;
const int GVB_SHIFT = 10;

const float POINT_RADIUS = 2.0;
const float LINE_WIDTH = 2.0;

const uint ACTIVE_COLOUR = 0xff44eeff;
const uint INACTIVE_COLOUR = 0xffeeeeee;
const uint QUIET_COLOUR = 0xff888888;

float screen_to_world_scale()
{
    camera@ cam = get_active_camera();
    return 1 / cam.editor_zoom();
}

const array<Point> SNAP_DIRECTIONS = {
    Point(1, 0),
    Point(2, 1),
    Point(1, 1),
    Point(1, 2),
    Point(0, 1),
    Point(-1, 2),
    Point(-1, 1),
    Point(-2, 1)
};

class PenTool : Tool
{
    Polygon polygon;

    array<Point> target_points;
    bool target_closed;

    PenTool(AdvToolScript@ script)
    {
		super(script, 'Tiles', 'Pen Tool');
		
		init_shortcut_key(VK::W, ModifierKey::Ctrl);
    }

    bool has_focus()
    {
        return (
            !script.input.key_check_gvb(GVB_SPACE)
            && !script.editor.mouse_in_gui()
        );
    }

    void calculate_targets()
    {
        target_closed = false;
        target_points.resize(0);

        Point mouse(script.input.mouse_x_world(19) / 48.0, script.input.mouse_y_world(19) / 48.0);

        // Auto-close
        if (polygon.size() >= 2 and script.input.key_check_gvb(GVB_SHIFT))
        {
            target_closed = true;

            Point@ first = polygon[0];
            Point@ last = polygon[polygon.size() - 1];

            for (uint i = 0; i < SNAP_DIRECTIONS.size(); ++i)
            {
                for (uint j = 0; j < SNAP_DIRECTIONS.size(); ++j)
                {
                    if (i == j)
                    {
                        continue;
                    }

                    // a + s * da = b + t * db
                    // t = da x (a - b) / da x db
                    float t = SNAP_DIRECTIONS[i].cross(first - last) / SNAP_DIRECTIONS[i].cross(SNAP_DIRECTIONS[j]);
                    Point intersection = last + t * SNAP_DIRECTIONS[j];
                    
                    // Check that the result lies on a grid point.
                    Point rounded = intersection.rounded();
                    if (intersection.is_close_to(rounded))
                    {
                        target_points.insertLast(rounded);
                    }
                }
            }
        }

        // Angle snap
        else if (polygon.size() >= 1)
        {
            Point@ last = polygon[polygon.size() - 1];

            for (uint i = 0; i < SNAP_DIRECTIONS.size(); ++i)
            {
                const Point@ delta = SNAP_DIRECTIONS[i];
                Point offset = mouse - last;
                float offset_length = offset.dot(delta) / delta.length_sqr();
                float grid_length = round(offset_length);
                Point target = (last + grid_length * delta).rounded();
                target_points.insertLast(target);
            }
        }

        // Grid snap
        else
        {
            target_points.insertLast(mouse.rounded());
        }
    }

    Point@ next_point()
    {
        Point mouse(script.input.mouse_x_world(19) / 48.0, script.input.mouse_y_world(19) / 48.0);

        float nearest_distance_sqr = INFINITY;
        Point@ nearest = null;

        for (uint i = 0; i < target_points.size(); ++i)
        {
            float target_distance_sqr = (target_points[i] - mouse).length_sqr();
            if (target_distance_sqr < nearest_distance_sqr)
            {
                nearest_distance_sqr = target_distance_sqr;
                @nearest = target_points[i];
            }
        }

        return nearest;
    }

    void step_impl() override
    {
        calculate_targets();

        if (!has_focus())
        {
            return;
        }

        if (script.input.key_check_pressed_gvb(GVB_LEFT_CLICK))
        {
            Point@ next = next_point();
            if (next !is null)
            {
                polygon.insert_last(next);
                if (target_closed or polygon.size() >= 3 and next == polygon[0])
                {
                    polygon.fill();
                    polygon.clear();
                }
            }
        }

        if (script.input.key_check_pressed_gvb(GVB_RIGHT_CLICK))
        {
            if (script.input.key_check_gvb(GVB_SHIFT))
            {
                polygon.clear();
            }
            else if (polygon.size() > 0)
            {
                polygon.remove_last();
            }
        }
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

    void draw_impl(const float) override
    {
        float scale = screen_to_world_scale();
        float point_radius = POINT_RADIUS * scale;
        float line_width = LINE_WIDTH * scale;

        if (script.input.key_check_gvb(GVB_SHIFT))
        {
            // Draw target points and connections
            for (uint i = 0; i < target_points.size(); ++i)
            {
                // Point
                script.g.draw_rectangle_world(
                    21, 10,
                    48 * target_points[i].x - point_radius, 48 * target_points[i].y - point_radius,
                    48 * target_points[i].x + point_radius, 48 * target_points[i].y + point_radius,
                    0, QUIET_COLOUR
                );
            }
        }

        if (has_focus())
        {
            // Draw next point and connection
            Point@ next = next_point();
            if (next !is null)
            {
                // Point
                script.g.draw_rectangle_world(
                    21, 10,
                    48 * next.x - point_radius, 48 * next.y - point_radius,
                    48 * next.x + point_radius, 48 * next.y + point_radius,
                    0, INACTIVE_COLOUR
                );

                // Connecting line
                if (polygon.size() > 0)
                {
                    Point@ last = polygon[polygon.size() - 1];
                    script.g.draw_line_world(
                        21, 10,
                        48 * last.x, 48 * last.y,
                        48 * next.x, 48 * next.y,
                        line_width, INACTIVE_COLOUR
                    );

                    // Closing line
                    if (target_closed)
                    {
                        Point@ first = polygon[0];
                        script.g.draw_line_world(
                            21, 10,
                            48 * next.x, 48 * next.y,
                            48 * first.x, 48 * first.y,
                            line_width, INACTIVE_COLOUR
                        );
                    }
                }
            }
        }

        // Draw existing points with connections
        for (uint i = 0; i < polygon.size(); ++i)
        {
            // Point
            script.g.draw_rectangle_world(
                21, 10,
                48 * polygon[i].x - point_radius, 48 * polygon[i].y - point_radius,
                48 * polygon[i].x + point_radius, 48 * polygon[i].y + point_radius,
                0, ACTIVE_COLOUR
            );

            // Connecting line
            if (i > 0)
            {
                script.g.draw_line_world(
                    21, 10,
                    48 * polygon[i - 1].x, 48 * polygon[i - 1].y,
                    48 * polygon[i].x, 48 * polygon[i].y,
                    line_width, ACTIVE_COLOUR
                );
            }
        }
    }
}
