class EditorKey
{
	
	int gvb;
	
	bool down;
	bool press;
	bool release;
	bool double_press;
	int double_count;
	
	private input_api@ input;
	private float press_time;
	
	EditorKey() { }
	
	EditorKey(input_api@ input, const int gvb)
	{
		@this.input = input;
		this.gvb = gvb;
	}
	
	void update(const float frame)
	{
		down    = input.key_check_gvb(gvb);
		press   = input.key_check_pressed_gvb(gvb);
		release = input.key_check_released_gvb(gvb);
		
		if(press)
		{
			double_press = press_time != -1 && frame - press_time <= 20;
			double_count = double_press ? double_count + 1 : 0;
			press_time = frame;
		}
		else
		{
			double_press = false;
		}
	}
	
	const bool opImplConv() const { return down; }
	const bool opConv() const { return down; }
	
}
