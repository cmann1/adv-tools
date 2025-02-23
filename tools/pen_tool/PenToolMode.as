#include "../../../../lib/math/IntVec2.cpp"
#include "../../../../lib/math/Vec2.cpp"
#include "../../../../lib/std.cpp"

#include "Polygon.as"

const array<IntVec2> SNAP_DIRECTIONS = {
    IntVec2(1, 0),
    IntVec2(2, 1),
    IntVec2(1, 1),
    IntVec2(1, 2),
    IntVec2(0, 1),
    IntVec2(-1, 2),
    IntVec2(-1, 1),
    IntVec2(-2, 1)
};

const IntVec2@ closest_point(const array<IntVec2>& target_points, const Vec2& point)
{
    float closest_distance_sqr = INFINITY;
    const IntVec2@ closest = null;

    for (uint i = 0; i < target_points.size(); ++i)
    {
        float target_distance_sqr = (Vec2(target_points[i]) - point).sqr_magnitude();
        if (target_distance_sqr < closest_distance_sqr)
        {
            closest_distance_sqr = target_distance_sqr;
            @closest = target_points[i];
        }
    }

    return closest;
}

void draw_snap_lines(PenTool@ tool, const IntVec2& point)
{
    int length = ceil(1000 / tool.script.zoom);
    for (uint i = 0; i < SNAP_DIRECTIONS.size(); ++i)
    {
        IntVec2 src = point + length * SNAP_DIRECTIONS[i];
        IntVec2 dst = point - length * SNAP_DIRECTIONS[i];
        tool.draw_line(src, dst, QUIET_COLOUR);
    }
}

/// Base class for modes that the pen tool can be in.
abstract class PenToolMode
{
    protected Polygon@ polygon;

    PenToolMode(Polygon@ polygon)
    {
        @this.polygon = polygon;
    }

    /// Add the next point to the polygon.
    void add_point(const Vec2& mouse)
    {
        const IntVec2@ next = next_point(mouse);
        if (next !is null)
        {
            polygon.insert_last(next);
        }
    }

    /// Return the point that would be added next.
    const IntVec2@ next_point(const Vec2& mouse) const
    {
        return null;
    }

    /// Draw a visualisation of the next point(s) to be added.
    void draw(const Vec2& mouse, PenTool@ tool) const
    {
        // By default, draw the next point and its connection to the polygon.
        const IntVec2@ next = next_point(mouse);
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
        const IntVec2@ next = next_point(mouse);
        if (next !is null)
        {
            polygon.insert_last(next);

            // Also close the polygon after adding the next point.
            polygon.insert_last(polygon[0]);
        }
    }

    const IntVec2@ next_point(const Vec2& mouse) const override
    {
        array<IntVec2>@ options = calculate_options(mouse);
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
        array<IntVec2>@ options = calculate_options(mouse);
        for (uint i = 0; i < options.size(); ++i)
        {
            tool.draw_point(options[i], INACTIVE_COLOUR);
        }

        // Draw the next point and its connections to the polygon.
        const IntVec2@ next = next_point(mouse);
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

    private array<IntVec2>@ calculate_options(const Vec2& mouse) const
    {
        array<IntVec2> options;

        if (polygon.size() == 0)
        {
            return options;
        }

        const IntVec2@ first = polygon[0];
        const IntVec2@ last = polygon[polygon.size() - 1];

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
                int numerator = cross_product_z(SNAP_DIRECTIONS[i], first - last);
                int denominator = cross_product_z(SNAP_DIRECTIONS[i], SNAP_DIRECTIONS[j]);

                // We are only interested in integer solutions for t.
                int t = numerator / denominator;
                if (t * denominator != numerator)
                {
                    continue;
                }

                IntVec2 intersection = last + t * SNAP_DIRECTIONS[j];
                options.insertLast(intersection);
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

    const IntVec2@ next_point(const Vec2& mouse) const override
    {
        array<IntVec2> options;

        if (polygon.size() == 0)
        {
            return null;
        }

        IntVec2 last = polygon[polygon.size() - 1];

        for (uint i = 0; i < SNAP_DIRECTIONS.size(); ++i)
        {
            IntVec2 delta = SNAP_DIRECTIONS[i];
            Vec2 offset = mouse - Vec2(last);
            float offset_length = dot(offset, Vec2(delta)) / Vec2(delta).sqr_magnitude();
            int grid_length = int(round(offset_length));
            IntVec2 target = last + grid_length * delta;
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

    const IntVec2@ next_point(const Vec2& mouse) const override
    {
        return IntVec2(round(mouse));
    }
}