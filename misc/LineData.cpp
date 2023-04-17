class LineData
{
	
	float x1, y1;
	float x2, y2;
	float mx, my;
	float length;
	float angle;
	float inv_delta_x, inv_delta_y;
	
	void set(const float x1, const float y1, const float x2, const float y2)
	{
		this.x1 = x1;
		this.y1 = y1;
		this.x2 = x2;
		this.y2 = y2;
		const float dx = x2 - x1;
		const float dy = y2 - y1;
		inv_delta_x = dx != 0 ? 1 / dx : 1;
		inv_delta_y = dy != 0 ? 1 / dy : 1;
	}
	
	bool aabb_intersection(
		const float x1, const float y1, const float x2, const float y2,
		float &out t_min, float &out t_max)
	{
		float t1 = (x1 - this.x1) * inv_delta_x;
		float t2 = (x2 - this.x1) * inv_delta_x;

		t_min = min(t1, t2);
		t_max = max(t1, t2);

		t1 = (y1 - this.y1) * inv_delta_y;
		t2 = (y2 - this.y1) * inv_delta_y;

		t_min = max(t_min, min(t1, t2));
		t_max = min(t_max, max(t1, t2));
		
		return t_min <= 1 && t_max >= 0 && t_max >= t_min;
	}
	
}
