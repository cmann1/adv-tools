#include "../../../../lib/math/Vec2.cpp"
#include "../../../../lib/std.cpp"

#include "polygon.as"

const array<Vec2> SNAP_DIRECTIONS = {
    Vec2(1, 0),
    Vec2(2, 1),
    Vec2(1, 1),
    Vec2(1, 2),
    Vec2(0, 1),
    Vec2(-1, 2),
    Vec2(-1, 1),
    Vec2(-2, 1)
};

const Vec2@ closest_point(const array<Vec2>& target_points, const Vec2& point)
{
    float closest_distance_sqr = INFINITY;
    const Vec2@ closest = null;

    for (uint i = 0; i < target_points.size(); ++i)
    {
        float target_distance_sqr = (target_points[i] - point).sqr_magnitude();
        if (target_distance_sqr < closest_distance_sqr)
        {
            closest_distance_sqr = target_distance_sqr;
            @closest = target_points[i];
        }
    }

    return closest;
}

void draw_snap_lines(PenTool@ tool, const Vec2& point)
{
    float length = 1000 / tool.script.zoom;
    for (uint i = 0; i < SNAP_DIRECTIONS.size(); ++i)
    {
        Vec2 src = point + length * SNAP_DIRECTIONS[i];
        Vec2 dst = point - length * SNAP_DIRECTIONS[i];
        tool.draw_line(src, dst, QUIET_COLOUR);
    }
}

/// Base class for modes that the pen tool can be in.
abstract class PenToolMode
{
    protected Polygon@ polygon;

    private array<Vec2>@ target_points = null;

    PenToolMode(Polygon@ polygon)
    {
        @this.polygon = polygon;
    }

    /// Add the next point to the polygon.
    void add_point(const Vec2& mouse)
    {
        const Vec2@ next = next_point(mouse);
        if (next !is null)
        {
            polygon.insert_last(next);
        }
    }

    /// Return the point that would be added next.
    const Vec2@ next_point(const Vec2& mouse) const
    {
        return null;
    }

    /// Draw a visualisation of the next point(s) to be added.
    void draw(const Vec2& mouse, PenTool@ tool) const
    {
        // By default, draw the next point and its connection to the polygon.
        const Vec2@ next = next_point(mouse);
        if (next !is null)
        {
            tool.draw_point(next, INACTIVE_COLOUR);

            if (polygon.size() >= 1)
            {
                tool.draw_line(polygon[polygon.size() - 1], next, INACTIVE_COLOUR);
            }
        }
    }
}

/// Add a single point and then close the polygon.
class AutoCloseMode : PenToolMode
{
    AutoCloseMode(Polygon@ polygon)
    {
        super(polygon);
    }

    void add_point(const Vec2& mouse) override
    {
        const Vec2@ next = next_point(mouse);
        if (next !is null)
        {
            polygon.insert_last(next);

            // Also close the polygon after adding the next point.
            polygon.insert_last(polygon[0]);
        }
    }

    const Vec2@ next_point(const Vec2& mouse) const override
    {
        array<Vec2>@ options = calculate_options(mouse);
        return closest_point(options, mouse);
    }

    void draw(const Vec2& mouse, PenTool@ tool) const override
    {
        // Draw the angle snap guides.
        if (polygon.size() >= 1)
        {
            draw_snap_lines(tool, polygon[0]);
            draw_snap_lines(tool, polygon[polygon.size() - 1]);
        }

        // Draw the available options.
        array<Vec2>@ options = calculate_options(mouse);
        for (uint i = 0; i < options.size(); ++i)
        {
            tool.draw_point(options[i], INACTIVE_COLOUR);
        }

        // Draw the next point and its connections to the polygon.
        const Vec2@ next = next_point(mouse);
        if (next !is null)
        {
            tool.draw_point(next, INACTIVE_COLOUR);

            if (polygon.size() >= 1)
            {
                // Connecting line
                tool.draw_line(polygon[polygon.size() - 1], next, INACTIVE_COLOUR);

                // Closing line
                tool.draw_line(next, polygon[0], INACTIVE_COLOUR);
            }
        }
    }

    private array<Vec2>@ calculate_options(const Vec2& mouse) const
    {
        array<Vec2> options;

        if (polygon.size() == 0)
        {
            return options;
        }

        const Vec2@ first = polygon[0];
        const Vec2@ last = polygon[polygon.size() - 1];

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
                float t = cross_product_z(SNAP_DIRECTIONS[i], first - last) / cross_product_z(SNAP_DIRECTIONS[i], SNAP_DIRECTIONS[j]);
                Vec2 intersection = last + t * SNAP_DIRECTIONS[j];
                
                // Check that the result lies on a grid point.
                Vec2 rounded = round(intersection);
                if (approximately(intersection, rounded))
                {
                    options.insertLast(rounded);
                }
            }
        }

        return options;
    }
}

/// Add a point at a fixed angle from the previous point.
class AngleSnapMode : PenToolMode
{
    AngleSnapMode(Polygon@ polygon)
    {
        super(polygon);
    }

    const Vec2@ next_point(const Vec2& mouse) const override
    {
        array<Vec2> options;

        if (polygon.size() == 0)
        {
            return null;
        }

        const Vec2@ last = polygon[polygon.size() - 1];

        for (uint i = 0; i < SNAP_DIRECTIONS.size(); ++i)
        {
            const Vec2@ delta = SNAP_DIRECTIONS[i];
            Vec2 offset = mouse - last;
            float offset_length = dot(offset, delta) / delta.sqr_magnitude();
            float grid_length = round(offset_length);
            Vec2 target = round(last + grid_length * delta);
            options.insertLast(target);
        }

        return closest_point(options, mouse);
    }

    void draw(const Vec2& mouse, PenTool@ tool) const
    {
        // Draw the angle snap guide.
        if (polygon.size() >= 1)
        {
            draw_snap_lines(tool, polygon[polygon.size() - 1]);
        }

        PenToolMode::draw(mouse, tool);
    }
}

/// Add a point aligned to the grid.
class GridSnapMode : PenToolMode
{
    GridSnapMode(Polygon@ polygon)
    {
        super(polygon);
    }

    const Vec2@ next_point(const Vec2& mouse) const override
    {
        return round(mouse);
    }
}