class InfoOverlay
{
	
	private AdvToolScript@ script;
	
	private float x1, y1;
	private float x2, y2;
	private float display_time;
	private float max_display_time;
	
	private Container@ dummy_overlay;
	private PopupOptions@ popup;
	private IWorldBoundingBox@ target;
	private Mouse@ mouse;
	private Element@ target_element;
	
	PopupPosition position {
		set {
			create_popup();
			popup.position = value;
		}
	}
	
	void init(AdvToolScript@ script)
	{
		@this.script = script;
	}
	
	void show(IWorldBoundingBox@ target, const string &in text, const float display_time=-1)
	{
		@this.target = target;
		@target_element = null;
		
		show(text, display_time);
	}
	
	void show(Mouse@ mouse, const string &in text, const float display_time=-1)
	{
		@target_element = null;
		show(mouse.x, mouse.y, mouse.x, mouse.y, text, display_time);
		@this.mouse = mouse;
	}
	
	void show(const float x, const float y, const string &in text, const float display_time=-1)
	{
		show(x, y, x, y, text, display_time);
	}
	
	void show(
		Element@ element,
		const string text, const float display_time=-1)
	{
		@target = null;
		@mouse = null;
		@target_element = element;
		
		show(text, display_time);
	}
	
	void show(
		const float x1, const float y1, const float x2, const float y2,
		const string text, const float display_time=-1)
	{
		this.x1 = x1;
		this.y1 = y1;
		this.x2 = x2;
		this.y2 = y2;
		@target = null;
		@mouse = null;
		@target_element = null;
		
		show(text, display_time);
	}
	
	private void show(const string &in text, const float display_time=-1)
	{
		this.max_display_time = display_time;
		this.display_time = display_time;
		
		create_popup();
		update_popup_position();
		dummy_overlay.visible = true;
		
		popup.content_string = text;
		popup.position = PopupPosition::Above;
		
		script.ui.move_to_back(dummy_overlay);
		script.ui.show_tooltip(popup, dummy_overlay);
	}
	
	void refresh(const float display_time)
	{
		this.max_display_time = display_time;
		this.display_time = display_time;
	}
	
	void refresh()
	{
		this.display_time = max_display_time;
	}
	
	void set_info(const string &in text)
	{
		popup.content_string = text;
	}
	
	void update(Mouse@ mouse)
	{
		update(mouse.x, mouse.y, mouse.x, mouse.y);
		@this.mouse = mouse;
	}
	
	void update(const float x, const float y)
	{
		update(x, y, x, y);
	}
	
	void update(const float x1, const float y1, const float x2, const float y2)
	{
		this.x1 = x1;
		this.y1 = y1;
		this.x2 = x2;
		this.y2 = y2;
		@target = null;
		@mouse = null;
		@target_element = null;
		
		update_popup_position();
	}
	
	void hide()
	{
		if(@dummy_overlay == null)
			return;
		
		script.ui.hide_tooltip(popup);
	}
	
	void step()
	{
		if(@target != null || @mouse != null || @target_element != null)
		{
			update_popup_position();
		}
		
		if(display_time > 0)
		{
			display_time -= DT;
			
			if(display_time <= 0)
			{
				hide();
			}
		}
	}
	
	private void create_popup()
	{
		if(@popup != null)
			return;
		
		@dummy_overlay = Container(script.ui);
		dummy_overlay.mouse_enabled = false;
		//dummy_overlay.background_colour = 0x55ff0000;
		script.ui.add_child(dummy_overlay);
		
		@popup = PopupOptions(script.ui, '', false,
			PopupPosition::Above, PopupTriggerType::Manual, PopupHideType::Manual);
		popup.hide.on(EventCallback(on_popup_hide));
	}
	
	private void update_popup_position()
	{
		bool is_hud = false;
		
		if(@target_element != null)
		{
			dummy_overlay.x1 = target_element.x1;
			dummy_overlay.y1 = target_element.y1;
			dummy_overlay.x2 = target_element.x2;
			dummy_overlay.y2 = target_element.y2;
			dummy_overlay.visible = true;
			return;
		}
		
		int layer = 22;
		int sub_layer = 22;
		
		if(@target != null)
		{
			target.get_bounding_box_world(this.x1, this.y1, this.x2, this.y2);
			layer = 22;
			sub_layer = 22;
		}
		else if(@mouse != null)
		{
			this.x1 = mouse.x;
			this.y1 = mouse.y;
			this.x2 = mouse.x;
			this.y2 = mouse.y;
			layer = mouse.layer;
			sub_layer = mouse.sub_layer;
		}
		
		float x1, y1, x2, y2;
		script.world_to_hud(layer, sub_layer, this.x1, this.y1, x1, y1);
		script.world_to_hud(layer, sub_layer, this.x2, this.y2, x2, y2);
		
		dummy_overlay.x = x1;
		dummy_overlay.y = y1;
		dummy_overlay.width = x2 - x1;
		dummy_overlay.height = y2 - y1;
		dummy_overlay.visible = true;
		dummy_overlay.force_calculate_bounds();
	}
	
	private void on_popup_hide(EventInfo@ event)
	{
		@target = null;
		@mouse = null;
		dummy_overlay.visible = false;
	}
	
}
