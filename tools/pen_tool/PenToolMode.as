#include "point.as"
#include "polygon.as"

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

const Point@ closest_point(const array<Point>& target_points, const Point& point)
{
    float closest_distance_sqr = INFINITY;
    const Point@ closest = null;

    for (uint i = 0; i < target_points.size(); ++i)
    {
        float target_distance_sqr = (target_points[i] - point).length_sqr();
        if (target_distance_sqr < closest_distance_sqr)
        {
            closest_distance_sqr = target_distance_sqr;
            @closest = target_points[i];
        }
    }

    return closest;
}

void draw_snap_lines(PenTool@ tool, const Point& point)
{
    float length = 1000 / tool.script.zoom;
    for (uint i = 0; i < SNAP_DIRECTIONS.size(); ++i)
    {
        Point src = point + length * SNAP_DIRECTIONS[i];
        Point dst = point - length * SNAP_DIRECTIONS[i];
        tool.draw_line(src, dst, QUIET_COLOUR);
    }
}

/// Base class for modes that the pen tool can be in.
abstract class PenToolMode
{
    protected Polygon@ polygon;

    private array<Point>@ target_points = null;

    PenToolMode(Polygon@ polygon)
    {
        @this.polygon = polygon;
    }

    /// Add the next point to the polygon.
    void add_point(const Point& mouse)
    {
        const Point@ next = next_point(mouse);
        if (next !is null)
        {
            polygon.insert_last(next);
        }
    }

    /// Return the point that would be added next.
    const Point@ next_point(const Point& mouse) const
    {
        return null;
    }

    /// Draw a visualisation of the next point(s) to be added.
    void draw(const Point& mouse, PenTool@ tool) const
    {
        // By default, draw the next point and its connection to the polygon.
        const Point@ next = next_point(mouse);
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

    void add_point(const Point& mouse) override
    {
        const Point@ next = next_point(mouse);
        if (next !is null)
        {
            polygon.insert_last(next);

            // Also close the polygon after adding the next point.
            polygon.insert_last(polygon[0]);
        }
    }

    const Point@ next_point(const Point& mouse) const override
    {
        array<Point>@ options = calculate_options(mouse);
        return closest_point(options, mouse);
    }

    void draw(const Point& mouse, PenTool@ tool) const override
    {
        // Draw the angle snap guides.
        if (polygon.size() >= 1)
        {
            draw_snap_lines(tool, polygon[0]);
            draw_snap_lines(tool, polygon[polygon.size() - 1]);
        }

        // Draw the available options.
        array<Point>@ options = calculate_options(mouse);
        for (uint i = 0; i < options.size(); ++i)
        {
            tool.draw_point(options[i], INACTIVE_COLOUR);
        }

        // Draw the next point and its connections to the polygon.
        const Point@ next = next_point(mouse);
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

    private array<Point>@ calculate_options(const Point& mouse) const
    {
        array<Point> options;

        if (polygon.size() == 0)
        {
            return options;
        }

        const Point@ first = polygon[0];
        const Point@ last = polygon[polygon.size() - 1];

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

    const Point@ next_point(const Point& mouse) const override
    {
        array<Point> options;

        if (polygon.size() == 0)
        {
            return null;
        }

        const Point@ last = polygon[polygon.size() - 1];

        for (uint i = 0; i < SNAP_DIRECTIONS.size(); ++i)
        {
            const Point@ delta = SNAP_DIRECTIONS[i];
            Point offset = mouse - last;
            float offset_length = offset.dot(delta) / delta.length_sqr();
            float grid_length = round(offset_length);
            Point target = (last + grid_length * delta).rounded();
            options.insertLast(target);
        }

        return closest_point(options, mouse);
    }

    void draw(const Point& mouse, PenTool@ tool) const
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

    const Point@ next_point(const Point& mouse) const override
    {
        return mouse.rounded();
    }
}