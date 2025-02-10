// TODO:
// - Store polygon points as integers to avoid floating point error bugs.
// - Figure out how I want to draw the target points for the different modes.
// - Middle click to swap to erase mode.

const bool DEBUG = false;

const float INFINITY = fpFromIEEE(0x7f800000);
const uint UINT_MAX = -1;

const int GVB_LEFT_CLICK = 2;
const int GVB_RIGHT_CLICK = 3;
const int GVB_SPACE = 8;
const int GVB_SHIFT = 10;

const float POINT_RADIUS = 2.0;
const float LINE_WIDTH = 2.0;

const uint ACTIVE_COLOUR = 0xff44eeff;
const uint INACTIVE_COLOUR = 0xffeeeeee;
const uint QUIET_COLOUR = 0xff888888;

const uint TILE_FULL    = 0;
const uint TILE_BIG_1   = 1;
const uint TILE_SMALL_1 = 2;
const uint TILE_BIG_2   = 3;
const uint TILE_SMALL_2 = 4;
const uint TILE_BIG_3   = 5;
const uint TILE_SMALL_3 = 6;
const uint TILE_BIG_4   = 7;
const uint TILE_SMALL_4 = 8;
const uint TILE_BIG_5   = 9;
const uint TILE_SMALL_5 = 10;
const uint TILE_BIG_6   = 11;
const uint TILE_SMALL_6 = 12;
const uint TILE_BIG_7   = 13;
const uint TILE_SMALL_7 = 14;
const uint TILE_BIG_8   = 15;
const uint TILE_SMALL_8 = 16;
const uint TILE_HALF_A  = 17;
const uint TILE_HALF_B  = 18;
const uint TILE_HALF_C  = 19;
const uint TILE_HALF_D  = 20;
const uint TILE_EMPTY   = UINT_MAX;

float round(float value, float grid)
{
    return grid * round(value / grid);
}

int gcd(int a, int b)
{
    a = abs(a);
    b = abs(b);
    while (b > 0)
    {
        int r = a % b;
        a = b;
        b = r;
    }
    return a;
}

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

class EdgeData
{
    int offset_x;
    int offset_y;
    bool facing_left;
    bool second_facing_left;
    bool facing_right;
    bool second_facing_right;
    uint tile_type;
    uint second_tile_type;

    EdgeData(
        int offset_x,
        int offset_y,
        bool facing_left,
        bool second_facing_left,
        bool facing_right,
        bool second_facing_right,
        uint tile_type,
        uint second_tile_type
    ) {
        this.offset_x = offset_x;
        this.offset_y = offset_y;
        this.facing_left = facing_left;
        this.second_facing_left = second_facing_left;
        this.facing_right = facing_right;
        this.second_facing_right = second_facing_right;
        this.tile_type = tile_type;
        this.second_tile_type = second_tile_type;
    }
}

const dictionary EDGE_DATA = {
    { "1,0" , EdgeData( 0,  0, false, false, false, false, TILE_FULL   , TILE_EMPTY  )},
    { "2,1" , EdgeData( 0,  0, false, false, false,  true, TILE_BIG_1  , TILE_SMALL_1)},
    { "1,1" , EdgeData( 0,  0, false, false,  true,  true, TILE_HALF_A , TILE_EMPTY  )},
    { "1,2" , EdgeData( 0,  0, false, false,  true,  true, TILE_SMALL_8, TILE_BIG_8  )},
    { "0,1" , EdgeData(-1,  0, false, false,  true,  true, TILE_FULL   , TILE_EMPTY  )},
    {"-1,2" , EdgeData(-1,  0, false, false,  true,  true, TILE_BIG_2  , TILE_SMALL_2)},
    {"-1,1" , EdgeData(-1,  0, false, false,  true,  true, TILE_HALF_B , TILE_EMPTY  )},
    {"-2,1" , EdgeData(-1,  0, false, false,  true, false, TILE_SMALL_7, TILE_BIG_7  )},
    {"-1,0" , EdgeData(-1, -1, false, false, false, false, TILE_FULL   , TILE_EMPTY  )},
    {"-2,-1", EdgeData(-1, -1, false,  true, false, false, TILE_BIG_3  , TILE_SMALL_3)},
    {"-1,-1", EdgeData(-1, -1,  true,  true, false, false, TILE_HALF_C , TILE_EMPTY  )},
    {"-1,-2", EdgeData(-1, -1,  true,  true, false, false, TILE_SMALL_6, TILE_BIG_6  )},
    { "0,-1", EdgeData( 0, -1,  true,  true, false, false, TILE_FULL   , TILE_EMPTY  )},
    { "1,-2", EdgeData( 0, -1,  true,  true, false, false, TILE_BIG_4  , TILE_SMALL_4)},
    { "1,-1", EdgeData( 0, -1,  true,  true, false, false, TILE_HALF_D , TILE_EMPTY  )},
    { "2,-1", EdgeData( 0, -1,  true, false, false, false, TILE_SMALL_5, TILE_BIG_5  )}
};

/** Each bit represents a point on the tile that may be covered:
 *
 *  0--1--2
 *  |     |
 *  7     3
 *  |     |
 *  6--5--4
 *
 */
const array<uint> TILE_BITMASKS = {
    0b11111111,  // FULL
    0b10011111,  // BIG_1
    0b00001111,  // SMALL_1
    0b11100111,  // BIG_2
    0b11000011,  // SMALL_2
    0b11111001,  // BIG_3
    0b11110000,  // SMALL_3
    0b01111110,  // BIG_4
    0b00111100,  // SMALL_4
    0b00111111,  // BIG_5
    0b00011110,  // SMALL_5
    0b11111100,  // BIG_6
    0b01111000,  // SMALL_6
    0b11110011,  // BIG_7
    0b11100001,  // SMALL_7
    0b11001111,  // BIG_8
    0b10000111,  // SMALL_8
    0b10001111,  // HALF_A
    0b11100011,  // HALF_B
    0b11111000,  // HALF_C
    0b00111110   // HALF_D
};

uint tile_type_from_bitmask(uint mask)
{
    // Check if the mask exactly matches any regular tile.
    for (uint type = 0; type <= 20; ++type)
    {
        if (mask == TILE_BITMASKS[type])
        {
            return type;
        }
    }
    
    // If half of the bits are set, return a full tile.
    // This accounts for special cases like two overlapping small tiles.
    uint bit_count = 0;
    while (mask > 0)
    {
        bit_count += mask & 1;
        mask >>= 1;
    }
    if (bit_count >= 4)
    {
        return TILE_FULL;
    }
    
    // Otherwise, this is an invalid/empty tile.
    return TILE_EMPTY;
}

class Point
{
    float x;
    float y;

    Point() {}

    Point(float x, float y)
    {
        this.x = x;
        this.y = y;
    }

    float dot(const Point& in other) const
    {
        return x * other.x + y * other.y;
    }

    float cross(const Point& in other) const
    {
        return x * other.y - y * other.x;
    }

    float length() const
    {
        return sqrt(x * x + y * y);
    }

    float length_sqr() const
    {
        return x * x + y * y;
    }

    Point normalised() const
    {
        float len = length();
        if (len == 0) return Point(1, 0);
        return this / len;
    }

    Point rounded(float grid = 1.0) const
    {
        return Point(
            round(x, grid),
            round(y, grid)
        );
    }

    Point lerp(const Point& in other, float f) const
    {
        return Point(
            x + f * (other.x - x),
            y + f * (other.y - y)
        );
    }

    bool is_close_to(const Point& in other) const
    {
        return closeTo(x, other.x) and closeTo(y, other.y);
    }

    bool is_collinear_with(const Point& in a, const Point& in b) const
    {
        return closeTo((y - a.y) * (b.x - x), (b.y - y) * (x - a.x));
    }

    Point opNeg() const
    {
        return Point(-x, -y);
    }

    Point opAdd(const Point& in other) const
    {
        return Point(
            x + other.x,
            y + other.y
        );
    }

    Point opSub(const Point& in other) const
    {
        return Point(
            x - other.x,
            y - other.y
        );
    }

    Point opMul(const Point& in other) const
    {
        return Point(
            x * other.x,
            y * other.y
        );
    }

    Point opMul_r(float value) const
    {
        return Point(
            x * value,
            y * value
        );
    }

    Point opMul(float value) const
    {
        return Point(
            x * value,
            y * value
        );
    }

    Point opDiv(float value) const
    {
        return Point(
            x / value,
            y / value
        );
    }

    bool opEquals(const Point& in other) const
    {
        return x == other.x and y == other.y;
    }

    string opConv() const
    {
        return "Point(" + formatFloat(x) + "," + formatFloat(y) + ")";
    }
}

class Tile
{
    uint mask = 0b11111111;

    bool facing_left = false;
    bool facing_right = false;
}

class Polygon
{
    array<Point> points;

    Point& opIndex(int i)
    {
        return points[i];
    }

    uint size() const
    {
        return points.size();
    }

    void insert_last(Point point)
    {
        // If the new one is collinear with the last two, move the last point.
        if (points.size() >= 2 and point.is_collinear_with(points[points.size() - 1], points[points.size() - 2]))
        {
            points[points.size() - 1] = point;
        }

        // Don't add the point if it is identical to the first point.
        else if (points.size() == 0 or point != points[0])
        {
            points.insertLast(point);
        }
    }

    void remove_last()
    {
        points.removeLast();
    }

    void clear()
    {
        points.resize(0);
    }

    float signed_area() const
    {
        // Shoelace formula.
        float twice_area = 0;
        for (uint i = 0; i < points.size(); ++i)
        {
            twice_area += points[i].cross(points[(i + 1) % points.size()]);
        }
        return twice_area / 2.0;
    }

    bool is_clockwise() const
    {
        return signed_area() > 0.0;
    }

    void fill()
    {
        scene@ g = get_scene();

        // If the last point is collinear with the first two, remove the first point.
        // This prevents the polygon from doubling back on itself.
        if (points.size() >= 3 and points[points.size() - 1].is_collinear_with(points[0], points[1]))
        {
            points.removeAt(0);
        }

        // Ensure that our polygon winds clockwise.
        if (!is_clockwise())
        {
            points.reverse();
        }

        // Keep track of the furthest right point, to use as a failsafe when filling tiles.
        int max_x = -INFINITY;

        // Follow the polygon to calculate the tiles along the border.
        dictionary border;  // "x,y" -> Tile
        for (uint i = 0; i < points.size(); ++i)
        {
            Point@ a = points[i];
            Point@ b = points[(i + 1) % points.size()];
            
            if (a.x > max_x)
            {
                max_x = a.x;
            }
            
            int dx = b.x - a.x;
            int dy = b.y - a.y;
            int steps = gcd(dx, dy);
            if (steps == 0)
                continue;
            int step_dx = dx / steps;
            int step_dy = dy / steps;

            const EdgeData@ edge_data = cast<EdgeData>(EDGE_DATA[step_dx + "," + step_dy]);
            bool is_slant = edge_data.second_tile_type != TILE_EMPTY;

            for (uint step = 0; step < steps; ++step)
            {
                int x = a.x + step * step_dx + edge_data.offset_x;
                int y = a.y + step * step_dy + edge_data.offset_y;
                string key = x + "," + y;
                Tile tile;
                border.get(key, tile);
                tile.mask &= TILE_BITMASKS[edge_data.tile_type];
                tile.facing_left  = tile.facing_left  or edge_data.facing_left;
                tile.facing_right = tile.facing_right or edge_data.facing_right;
                border.set(key, tile);

                if (is_slant)
                {
                    int second_x = x + step_dx / 2;
                    int second_y = y + step_dy / 2;
                    string second_key = second_x + "," + second_y;
                    Tile second_tile;
                    border.get(second_key, second_tile);
                    second_tile.mask &= TILE_BITMASKS[edge_data.second_tile_type];
                    second_tile.facing_left  = second_tile.facing_left  or edge_data.second_facing_left;
                    second_tile.facing_right = second_tile.facing_right or edge_data.second_facing_right;
                    border.set(second_key, second_tile);
                }
            }
        }

        if (DEBUG)
        {
            array<string> border_keys = border.getKeys();
            border_keys.sortAsc();
            for (uint i = 0; i < border_keys.size(); ++i)
            {
                string key = border_keys[i];
                Tile@ tile = cast<Tile>(border[key]);
                puts("" + key + " -> " + tile.mask + " " + tile.facing_left + " " + tile.facing_right);
            }
        }

        // For every left-facing tile, scan and fill until we hit a right-facing tile.
        array<string>@ border_coordinates = border.getKeys();
        for (uint i = 0; i < border_coordinates.size(); ++i)
        {
            string border_coordinate = border_coordinates[i];
            Tile@ border_tile = cast<Tile>(border[border_coordinate]);
            if (!border_tile.facing_left)
            {
                continue;
            }

            array<string> split_coordinate = border_coordinate.split(",");
            float x = parseFloat(split_coordinate[0]);
            float y = parseFloat(split_coordinate[1]);

            Tile@ tile;
            do
            {
                border.get(x + "," + y, @tile);

                // If this tile is on the border, use the tile shape from the mask.
                uint type;
                if (tile !is null)
                {
                    uint mask = tile.mask;

                    // Merge the new tile shape with the existing tile shape.
                    tileinfo@ existing_tile = g.get_tile(x, y);
                    if (existing_tile.solid())
                    {
                        uint existing_mask = TILE_BITMASKS[existing_tile.type()];
                        mask |= existing_mask;
                    }

                    type = tile_type_from_bitmask(mask);
                }

                // Otherwise, just fill the whole tile.
                else
                {
                    type = TILE_FULL;
                }

                if (DEBUG)
                {
                    puts(x + "," + y + " => " + type);
                }

                // Do nothing if the tile has been eroded away completely.
                if (type != TILE_EMPTY)
                {
                    g.set_tile(x, y, 19, true, type, 1, 1, 1);
                }

                x += 1;
            }
            while ((tile is null or !tile.facing_right) and x < max_x);
        }
    }
}

class script
{
    scene@ g;
    input_api@ input;
    editor_api@ editor;

    Polygon polygon;

    array<Point> target_points;
    bool target_closed;

    script()
    {
        @g = get_scene();
        @input = get_input_api();
        @editor = get_editor_api();
    }

    bool has_focus()
    {
        return (
            !input.key_check_gvb(GVB_SPACE)
            && !editor.mouse_in_gui()
        );
    }

    void calculate_targets()
    {
        target_closed = false;
        target_points.resize(0);

        Point mouse(input.mouse_x_world(19) / 48.0, input.mouse_y_world(19) / 48.0);

        // Auto-close
        if (polygon.size() >= 2 and input.key_check_gvb(GVB_SHIFT))
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
        Point mouse(input.mouse_x_world(19) / 48.0, input.mouse_y_world(19) / 48.0);

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

    void editor_step()
    {
        calculate_targets();

        if (!has_focus())
        {
            return;
        }

        if (input.key_check_pressed_gvb(GVB_LEFT_CLICK))
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

        if (input.key_check_pressed_gvb(GVB_RIGHT_CLICK))
        {
            if (input.key_check_gvb(GVB_SHIFT))
            {
                polygon.clear();
            }
            else if (polygon.size() > 0)
            {
                polygon.remove_last();
            }
        }
    }

    void editor_draw(float)
    {
        float scale = screen_to_world_scale();
        float point_radius = POINT_RADIUS * scale;
        float line_width = LINE_WIDTH * scale;

        if (input.key_check_gvb(GVB_SHIFT))
        {
            // Draw target points and connections
            for (uint i = 0; i < target_points.size(); ++i)
            {
                // Point
                g.draw_rectangle_world(
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
                g.draw_rectangle_world(
                    21, 10,
                    48 * next.x - point_radius, 48 * next.y - point_radius,
                    48 * next.x + point_radius, 48 * next.y + point_radius,
                    0, INACTIVE_COLOUR
                );

                // Connecting line
                if (polygon.size() > 0)
                {
                    Point@ last = polygon[polygon.size() - 1];
                    g.draw_line_world(
                        21, 10,
                        48 * last.x, 48 * last.y,
                        48 * next.x, 48 * next.y,
                        line_width, INACTIVE_COLOUR
                    );

                    // Closing line
                    if (target_closed)
                    {
                        Point@ first = polygon[0];
                        g.draw_line_world(
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
            g.draw_rectangle_world(
                21, 10,
                48 * polygon[i].x - point_radius, 48 * polygon[i].y - point_radius,
                48 * polygon[i].x + point_radius, 48 * polygon[i].y + point_radius,
                0, ACTIVE_COLOUR
            );

            // Connecting line
            if (i > 0)
            {
                g.draw_line_world(
                    21, 10,
                    48 * polygon[i - 1].x, 48 * polygon[i - 1].y,
                    48 * polygon[i].x, 48 * polygon[i].y,
                    line_width, ACTIVE_COLOUR
                );
            }
        }
    }
}
