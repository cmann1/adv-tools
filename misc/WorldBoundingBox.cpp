#include 'IWorldBoundingBox.cpp';

class WorldBoundingBox : IWorldBoundingBox
{
	
	AdvToolScript@ script;
	
	float x1, y1;
	float x2, y2;
	int layer = 19;
	
	private int count;
	
	void get_bounding_box_world(float &out x1, float &out y1, float &out x2, float &out y2) override
	{
		if(layer != -1 && layer < 12)
		{
			script.transform(this.x1, this.y1, layer, 19, x1, y1);
			script.transform(this.x2, this.y2, layer, 19, x2, y2);
			return;
		}
		
		x1 = this.x1;
		y1 = this.y1;
		x2 = this.x2;
		y2 = this.y2;
	}
	
	void reset()
	{
		layer = -1;
		count = 0;
	}
	
	void add(const float x1, const float y1, const float x2, const float y2, const int layer=19)
	{
		if(layer > this.layer)
		{
			this.layer = layer;
		}
		
		if(count++ == 0)
		{
			this.x1 = x1;
			this.y1 = y1;
			this.x2 = x2;
			this.y2 = y2;
		}
		else
		{
			if(x1 < this.x1) this.x1 = x1;
			if(y1 < this.y1) this.y1 = y1;
			if(x2 > this.x2) this.x2 = x2;
			if(y2 > this.y2) this.y2 = y2;
		}
	}
	
}