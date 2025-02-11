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

    Point rounded() const
    {
        return Point(
            round(x),
            round(y)
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