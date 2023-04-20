#include '../../../../lib/ui3/elements/Container.cpp';
#include '../../../../lib/ui3/elements/Toolbar.cpp';
#include '../../../../lib/ui3/elements/Button.cpp';
#include '../../../../lib/ui3/elements/ColourSwatch.cpp';
#include '../../../../lib/ui3/elements/Label.cpp';
#include '../../../../lib/ui3/elements/LayerButton.cpp';
#include '../../../../lib/ui3/elements/NumberSlider.cpp';
#include '../../../../lib/ui3/elements/RotationWheel.cpp';
#include '../../../../lib/ui3/elements/Select.cpp';
#include '../../../../lib/ui3/elements/TextBox.cpp';
#include '../../../../lib/ui3/elements/Window.cpp';
#include '../../../../lib/ui3/layouts/AnchorLayout.cpp';

#include 'TriggerToolHandler.cpp';
#include 'TextTriggerHandlerData.cpp';

const string EMBED_spr_icon_text = SPRITES_BASE + 'icon_text.png';

class TextTriggerHandler : TriggerToolHandler
{
	
	/** The firsts selected z trigger if there is one. */
	private TextTriggerHandlerData@ selected_z_trigger;
	/** The firsts selected text trigger if there is one. */
	private TextTriggerHandlerData@ selected_normal_trigger;
	
	private Container@ dummy_overlay;
	private PopupOptions@ popup;
	private Toolbar@ toolbar;
	private Button@ edit_button;
	
	private Window@ window;
	private Checkbox@ visible_checkbox;
	private TextBox@ text_box;
	private Container@ z_properties_container;
	
	private ColourSwatch@ colour_swatch;
	private LayerButton@ layer_button;
	private LayerSelector@ layer_select;
	private RotationWheel@ rotation_wheel;
	private NumberSlider@ scale_slider;
	private Select@ font_select;
	private Select@ font_size_select;
	
	private float colour_swatch_open_alpha;
	
	private bool is_editing;
	private const array<int>@ font_sizes;
	private int selected_font_size;
	
	private bool ignore_events;
	private bool ignore_next_font_size_update;
	/// Lock the text box input for one frame after pressing Enter to start editing to
	/// prevent the Enter/text event being processed by the TextBox and overwrites the selected text.
	private bool lock_input;
	
	private float base_drag_value;
	private float base_drag_dir_x, base_drag_dir_y;
	
	TextTriggerHandler(AdvToolScript@ script, ExtendedTriggerTool@ tool)
	{
		super(script, tool);
	}
	
	void build_sprites(message@ msg) override
	{
		build_sprite(msg, 'icon_text');
	}
	
	bool should_handle(entity@ trigger, const string &in type) override
	{
		return type == TextTriggerType::Normal || type == TextTriggerType::ZTextProp;
	}
	
	protected void select_impl(entity@ trigger, const string &in type) override
	{
		select_trigger(trigger, true);
	}
	
	protected void deselect_impl() override
	{
		if(lock_input)
		{
			unlock_input();
		}
	}
	
	void step() override
	{
		if(lock_input)
		{
			unlock_input();
		}
		
		if(!script.ui.visible)
		{
			stop_editing(true);
		}
		
		if(@popup != null && !popup.popup_visible && state == TriggerHandlerState::Idle)
		{
			script.ui.show_tooltip(popup, dummy_overlay);
		}
		else if(@popup != null && popup.popup_visible && state != TriggerHandlerState::Idle)
		{
			script.ui.hide_tooltip(popup);
		}
		
		check_selected_triggers();
		check_keys();
		check_mouse();
		
		for(uint i = 0; i < select_list.length; i++)
		{
			TextTriggerHandlerData@ data = selected(i);
			
			if(!data.is_z_trigger && data.hidden)
			{
				tool.add_hidden_trigger(data.trigger);
			}
		}
		
		if(@popup != null && popup.popup_visible && select_list.length > 0)
		{
			update_toolbar_position();
		}
	}
	
	void draw(const float sub_frame) override
	{
		if(state == TriggerHandlerState::Idle && select_list.length > 0)
		{
			for(uint i = 0; i < select_list.length; i++)
			{
				TextTriggerHandlerData@ data = selected(i);
				draw_line_to_ui(data.trigger.x(), data.trigger.y(), popup.popup);
			}
		}
		
		if(state == TriggerHandlerState::Rotating || state == TriggerHandlerState::Scaling)
		{
			float x1, y1, x2, y2;
			script.world_to_hud(selected_z_trigger.trigger.x(), selected_z_trigger.trigger.y(), x1, y1, false);
			script.world_to_hud(script.mouse.x, script.mouse.y, x2, y2, false);
			script.ui.style.draw_line(x1, y1, x2, y2, 2, (state == TriggerHandlerState::Rotating ? 0x9c3edf : 0xd85d45) | 0xbb000000);
		}
	}
	
	// //////////////////////////////////////////////////////////
	// Methods
	// //////////////////////////////////////////////////////////
	
	private bool sub_ui_active()
	{
		if(@colour_swatch == null)
			return false;
		
		return colour_swatch.open || layer_button.open;
	}
	
	/// Handle shortcut keys like Escape and Enter.
	private void check_keys()
	{
		if(state == TriggerHandlerState::Idle)
		{
			// Start editing with Enter.
			if(@selected_trigger != null && script.scene_focus && script.consume_pressed_gvb(GVB::Return))
			{
				lock_input = true;
				start_editing();
			}
			
			// Accept/Cancel with Enter/Escape when the textbox isn't focused.
			if(script.scene_focus && !sub_ui_active())
			{
				if(script.escape_press)
				{
					stop_editing(false);
				}
				else if(script.ctrl.down && script.return_press)
				{
					stop_editing(true);
				}
			}
		}
		
		// Consume Enter to prevent the default editing.
		if(@selected_trigger != null && !script.ui_focus && !sub_ui_active() && script.return_press)
		{
			if(!script.ctrl.down && script.return_press)
			{
				script.input.key_clear_gvb(GVB::Return);
			}
		}
	}
	
	private void check_mouse()
	{
		bool changed = false;
		
		if(state == TriggerHandlerState::Idle)
		{
			// Multi select
			if(select_list.length > 0 && !script.alt.down && script.shift.down && script.mouse.left_press)
			{
				script.input.key_clear_gvb(GVB::LeftClick);
				
				entity@ new_trigger = pick_trigger();
				if(@new_trigger != null)
				{
					select_trigger(new_trigger);
				}
			}
			
			if(@selected_z_trigger != null)
			{
				if(script.alt.down && script.mouse.left_press)
				{
					rotate_start();
				}
				else if(script.alt.down && script.mouse.right_press)
				{
					scale_start();
				}
			}
		}
		else if(state == TriggerHandlerState::Rotating)
		{
			rotate_step();
			script.input.key_clear_gvb(GVB::LeftClick);
		}
		else if(state == TriggerHandlerState::Scaling)
		{
			scale_step();
			script.input.key_clear_gvb(GVB::RightClick);
		}
	}
	
	//
	
	private void rotate_start()
	{
		base_drag_value = atan2(
			script.mouse.y - selected_z_trigger.trigger.y(),
			script.mouse.x - selected_z_trigger.trigger.x());
		
		for(uint i = 0; i < select_list.length; i++)
		{
			TextTriggerHandlerData@ data = selected(i);
			data.base_rotation = data.rotation;
		}
		
		script.input.key_clear_gvb(GVB::LeftClick);
		state = TriggerHandlerState::Rotating;
	}
	
	private void rotate_step()
	{
		float offset = shortest_angle(
			base_drag_value,
			atan2(
				script.mouse.y - selected_z_trigger.trigger.y(),
				script.mouse.x - selected_z_trigger.trigger.x()));
		script.snap(offset, offset, false);
		
		bool changed = false;
		
		for(uint i = 0; i < select_list.length; i++)
		{
			TextTriggerHandlerData@ data = selected(i);
			const int base_val = data.rotation;
			data.rotation = int(normalize_degress(data.base_rotation + offset * RAD2DEG));
			
			if(!changed && base_val != data.rotation)
			{
				changed = true;
			}
		}
		
		if(changed && is_editing)
		{
			ignore_events = true;
			rotation_wheel.degrees = selected_z_trigger.rotation;
			ignore_events = false;
		}
		
		if(!script.mouse.left_down || script.escape_press)
		{
			rotate_end();
		}
	}
	
	private void rotate_end()
	{
		if(script.escape_press)
		{
			for(uint i = 0; i < select_list.length; i++)
			{
				TextTriggerHandlerData@ data = selected(i);
				data.rotation = int(data.base_rotation);
			}
		}
		
		state = TriggerHandlerState::Idle;
	}
	
	private void scale_start()
	{
		base_drag_value = distance(
			script.mouse.x, script.mouse.y,
			selected_z_trigger.trigger.x(), selected_z_trigger.trigger.y());
		base_drag_dir_x = script.mouse.x - selected_z_trigger.trigger.x();
		base_drag_dir_y = script.mouse.y - selected_z_trigger.trigger.y();
		
		for(uint i = 0; i < select_list.length; i++)
		{
			TextTriggerHandlerData@ data = selected(i);
			data.base_scale = data.scale;
		}
		
		script.input.key_clear_gvb(GVB::RightClick);
		state = TriggerHandlerState::Scaling;
	}
	
	private void scale_step()
	{
		const float dir = sign(dot(
			base_drag_dir_x, base_drag_dir_y,
			script.mouse.x - selected_z_trigger.trigger.x(), script.mouse.y - selected_z_trigger.trigger.y()));
		const float scale =
			distance(
				script.mouse.x, script.mouse.y,
				selected_z_trigger.trigger.x(), selected_z_trigger.trigger.y()) /
			base_drag_value * dir;
		bool changed = false;
		
		for(uint i = 0; i < select_list.length; i++)
		{
			TextTriggerHandlerData@ data = selected(i);
			const float base_val = data.scale;
			data.scale = data.base_scale * scale;
			
			if(!changed && base_val != data.rotation)
			{
				changed = true;
			}
		}
		
		if(changed && is_editing)
		{
			ignore_events = true;
			scale_slider.value = selected_z_trigger.scale;
			ignore_events = false;
		}
		
		if(!script.mouse.right_down || script.escape_press)
		{
			scale_end();
		}
	}
	
	private void scale_end()
	{
		if(script.escape_press)
		{
			for(uint i = 0; i < select_list.length; i++)
			{
				TextTriggerHandlerData@ data = selected(i);
				data.scale = data.base_scale;
			}
		}
		
		state = TriggerHandlerState::Idle;
	}
	
	//
	
	private TextTriggerHandlerData@ selected(const int index)
	{
		return cast<TextTriggerHandlerData@>(select_list[index >= 0 ? index : select_list.length - 1]);
	}
	
	protected TriggerHandlerData@ create_handler_data(entity@ trigger) override
	{
		return TextTriggerHandlerData(trigger);
	}
	
	protected void on_selection_changed(const bool primary, const bool added, const bool removed) override
	{
		if(is_editing && select_list.length == 0)
		{
			stop_editing(true);
		}
		
		if(primary && !added && is_editing)
		{
			update_properties();
			return;
		}
		
		if(!primary && select_list.length != 0)
		{
			if(is_editing)
			{
				update_properties();
			}
			else
			{
				update_z_trigger(removed);
			}
			return;
		}
		
		if(@selected_trigger != null)
		{
			if(@popup == null)
			{
				create_toolbar();
			}
			
			script.ui.add_child(dummy_overlay);
			script.ui.move_to_back(dummy_overlay);
			update_toolbar_position();
			script.ui.show_tooltip(popup, dummy_overlay);
			
			if(is_editing)
			{
				stop_editing(true, false);
				start_editing();
			}
		}
		else
		{
			script.ui.hide_tooltip(popup, script.in_editor);
			script.ui.remove_child(dummy_overlay);
			
			stop_editing(true);
		}
		
		update_z_trigger(true);
	}
	
	private void update_z_trigger(const bool full=false)
	{
		const bool had_z_trigger = @selected_z_trigger != null;
		const bool had_normal_trigger = @selected_normal_trigger != null;
		
		if(full)
		{
			@selected_z_trigger = null;
			@selected_normal_trigger = null;
			
			for(uint i = 0; i < select_list.length; i++)
			{
				TextTriggerHandlerData@ data = selected(i);
				
				if(@selected_z_trigger == null && data.is_z_trigger)
				{
					@selected_z_trigger = data;
				}
				if(@selected_normal_trigger == null && !data.is_z_trigger)
				{
					@selected_normal_trigger = data;
				}
				
				if(@selected_z_trigger != null && @selected_normal_trigger !=  null)
					break;
			}
		}
		else if(@selected_trigger == null)
		{
			@selected_z_trigger = null;
			@selected_normal_trigger = null;
		}
		else
		{
			TextTriggerHandlerData@ data = selected(-1);
			if(@selected_z_trigger == null && data.trigger_type == TextTriggerType::ZTextProp)
			{
				@selected_z_trigger = data;
			}
			else if(@selected_normal_trigger == null && data.trigger_type == TextTriggerType::Normal)
			{
				@selected_normal_trigger = data;
			}
		}
		
		if(had_z_trigger != (@selected_z_trigger != null) || had_normal_trigger != (@selected_normal_trigger != null))
		{
			update_properties();
		}
	}
	
	private void start_editing(entity@ trigger=null)
	{
		const bool is_window_created = create_window();
		
		if(is_editing)
		{
			selected(select_list.length - 1).store_all();
		}
		else
		{
			is_editing = true;
			
			for(uint i = 0; i < select_list.length; i++)
			{
				selected(i).store_all();
			}
		}
		
		update_properties();
		
		if(is_window_created)
		{
			window.fit_to_contents(true);
			window.centre();
			script.window_manager.force_immediate_reposition(window);
		}
		
		edit_button.selected = true;
		window.show();
		window.parent.move_to_front(window);
		@script.ui.focus = text_box;
		text_box.select_all();
	}
	
	private void stop_editing(const bool accept, const bool update_ui=true)
	{
		if(!is_editing)
			return;
		
		const string text = accept && @text_box != null ? text_box.text : '';
		
		for(uint i = 0; i < select_list.length; i++)
		{
			TextTriggerHandlerData@ data = selected(i);
			
			if(!accept)
			{
				data.restor_all();
			}
		}
		
		is_editing = false;
		
		if(update_ui)
		{
			edit_button.selected = false;
			if(@window != null)
			{
				window.hide('user', script.in_editor);
			}
		}
	}
	
	private void unlock_input()
	{
		if(@text_box != null)
		{
			text_box.lock_input = false;
		}
		
		lock_input = false;
	}
	
	// //////////////////////////////////////////////////////////
	// UI
	// //////////////////////////////////////////////////////////
	
	private void create_toolbar()
	{
		UI@ ui = script.ui;
		Style@ style = ui.style;
		
		@toolbar = Toolbar(ui, false, true);
		toolbar.name = 'TriggerToolTextToolbar';
		toolbar.x = script.main_toolbar.x;
		toolbar.y = script.main_toolbar.y + script.main_toolbar.height;
		toolbar.is_snap_target = false;
		
		EventCallback@ button_click = EventCallback(on_toolbar_button_click);
		
		@edit_button = toolbar.create_button(SPRITE_SET, 'icon_edit', Settings::IconSize, Settings::IconSize);
		edit_button.name = 'edit';
		edit_button.selectable = true;
		edit_button.fit_to_contents(true);
		@edit_button.tooltip = PopupOptions(ui, 'Edit');
		script.init_icon(edit_button);
		edit_button.mouse_click.on(button_click);
		
		toolbar.fit_to_contents(true);
		
		//
		
		@dummy_overlay = Container(script.ui);
		//dummy_overlay.background_colour = 0x55ff0000;
		dummy_overlay.mouse_self = false;
		dummy_overlay.is_snap_target = false;
		
		@popup = PopupOptions(script.ui, toolbar, true, PopupPosition::Above, PopupTriggerType::Manual, PopupHideType::Manual);
		popup.as_overlay = false;
		popup.spacing = 0;
		popup.padding = 0;
		popup.background_colour = 0;
		popup.border_colour = 0;
		popup.shadow_colour = 0;
		popup.background_blur = false;
	}
	
	private bool create_window()
	{
		if(@window != null)
			return false;
		
		UI@ ui = script.ui;
		Style@ style = ui.style;
		
		EventCallback@ cancel_click = EventCallback(on_cancel_click);
		
		@window = Window(ui, 'Edit Text');
		window.resizable = true;
		window.min_width = 450;
		window.min_height = 350;
		window.name = 'TextToolTextProperties';
		window.set_icon(SPRITE_SET, 'icon_text', 25, 25);
		ui.add_child(window);
		window.x = 200;
		window.y = 20;
		window.width  = 200;
		window.height = 200;
		window.contents.autoscroll_on_focus = false;
		@window.layout = AnchorLayout(ui).set_padding(0);
		window.close.on(cancel_click);
		
		@text_box = TextBox(ui);
		text_box.multi_line = true;
		text_box.select_all_on_focus = false;
		text_box.accept_on_blur = false;
		text_box.width  = 500;
		text_box.height = 300;
		text_box.anchor_left.pixel(0);
		text_box.anchor_right.pixel(0);
		text_box.anchor_top.pixel(0);
		text_box.anchor_bottom.pixel(0);
		text_box.change.on(EventCallback(on_text_change));
		text_box.accept.on(EventCallback(on_text_accept));
		window.add_child(text_box);
		
		Button@ btn = Button(ui, 'Accept');
		btn.fit_to_contents();
		btn.mouse_click.on(EventCallback(on_accept_click));
		window.add_button_left(btn);
		
		@btn = Button(ui, 'Cancel');
		btn.fit_to_contents();
		btn.mouse_click.on(EventCallback(on_cancel_click));
		window.add_button_right(btn);
		
		script.window_manager.register_element(window);
		
		return true;
	}
	
	private void create_z_properties_container()
	{
		if(@z_properties_container != null)
			return;
		
		UI@ ui = script.ui;
		Style@ style = ui.style;
		
		@z_properties_container = Container(ui);
		z_properties_container.y = text_box.y + text_box.height + ui.style.spacing;
		z_properties_container.width = text_box.width;
		z_properties_container.anchor_left.pixel(0);
		z_properties_container.anchor_right.pixel(0);
		z_properties_container.anchor_bottom.pixel(0);
		
		text_box.anchor_bottom.sibling(z_properties_container).padding(style.spacing);
		
		@rotation_wheel = RotationWheel(ui);
		z_properties_container.add_child(rotation_wheel);
		
		// Colour swatch
		
		@colour_swatch = ColourSwatch(ui);
		colour_swatch.width  = rotation_wheel.height;
		colour_swatch.height = rotation_wheel.height;
		@colour_swatch.tooltip = PopupOptions(ui, 'Colour');
		colour_swatch.change.on(EventCallback(on_colour_change));
		colour_swatch.activate.on(EventCallback(on_colour_change));
		z_properties_container.add_child(colour_swatch);
		
		// Layer button
		
		@layer_button = LayerButton(ui);
		layer_button.height = rotation_wheel.height;
		layer_button.auto_close = false;
		layer_button.x = colour_swatch.x + colour_swatch.width + style.spacing;
		@layer_button.tooltip = PopupOptions(ui, 'Layer');
		layer_button.change.on(EventCallback(on_layer_select));
		layer_button.select.on(EventCallback(on_layer_select));
		z_properties_container.add_child(layer_button);
		
		@layer_select = layer_button.layer_select;
		layer_select.multi_select = false;
		layer_select.min_select = 1;
		layer_select.min_select_layers = 1;
		
		// Rotation wheel
		
		rotation_wheel.x = layer_button.x + layer_button.width + style.spacing;
		rotation_wheel.start_angle = 0;
		rotation_wheel.allow_range = false;
		rotation_wheel.auto_tooltip = true;
		rotation_wheel.tooltip_precision = 0;
		rotation_wheel.tooltip_prefix = 'Rotation: ';
		rotation_wheel.change.on(EventCallback(on_rotation_change));
		
		// Scale slider
		
		@scale_slider = NumberSlider(ui, 0, NAN, NAN, 0.01);
		scale_slider.x = colour_swatch.x;
		scale_slider.y = colour_swatch.y + colour_swatch.height + style.spacing;
		scale_slider.width = (rotation_wheel.x + rotation_wheel.width) - scale_slider.x;
		@scale_slider.tooltip = PopupOptions(ui, 'Scale');
		scale_slider.change.on(EventCallback(on_scale_change));
		z_properties_container.add_child(scale_slider);
		
		// Font select
		
		@font_select = Select(ui);
		font_select.anchor_right.pixel(0);
		z_properties_container.add_child(font_select);
		
		font_select.add_value(font::ENVY_BOLD, 'Envy Bold');
		font_select.add_value(font::SANS_BOLD, 'Sans Bold');
		font_select.add_value(font::CARACTERES, 'Caracteres');
		font_select.add_value(font::PROXIMANOVA_REG, 'ProximaNovaReg');
		font_select.add_value(font::PROXIMANOVA_THIN, 'ProximaNovaThin');
		
		font_select.width = 200;
		font_select.x = z_properties_container.width - font_select.width;
		font_select.y = colour_swatch.y;
		font_select.change.on(EventCallback(on_font_change));
		
		Label@ font_label = create_label('Font');
		font_label.anchor_right.sibling(font_select).padding(style.spacing);
		font_label.y = font_select.y;
		font_label.height = font_select.height;
		
		// Font size select
		
		@font_size_select = Select(ui);
		font_size_select.anchor_right.pixel(0);
		z_properties_container.add_child(font_size_select);
		
		font_size_select.width = 85;
		font_size_select.x = z_properties_container.width - font_size_select.width;
		font_size_select.y = font_select.y + font_select.height + style.spacing;
		font_size_select.change.on(EventCallback(on_font_size_change));
		
		Label@ font_size_label = create_label('Size');
		font_size_label.anchor_right.sibling(font_size_select).padding(style.spacing);
		font_size_label.x = font_size_select.x - font_size_label.width - style.spacing;
		font_size_label.y = font_size_select.y;
		font_size_label.height = font_size_select.height;
		
		z_properties_container.fit_to_contents(true);
		@z_properties_container.layout = AnchorLayout(script.ui).set_padding(0);
		
		window.add_child(z_properties_container);
	}
	
	private Label@ create_label(const string text)
	{
		Label@ label = Label(script.ui, text);
		label.set_padding(script.ui.style.spacing, script.ui.style.spacing, 0, 0);
		label.align_v = GraphicAlign::Middle;
		label.fit_to_contents();
		z_properties_container.add_child(label);
		
		return label;
	}
	
	private void update_toolbar_position()
	{
		float x = 0, y = 0;
		
		for(uint i = 0; i < select_list.length; i++)
		{
			TextTriggerHandlerData@ data = selected(i);
			x += data.trigger.x();
			y += data.trigger.y();
		}
		
		x /= select_list.length;
		y /= select_list.length;
		
		float x1, y1, x2, y2;
		const float size = select_list.length == 1 ? 10 : 0;
		script.world_to_hud(x - size, y - size, x1, y1);
		script.world_to_hud(x + size, y + size, x2, y2);
		
		if(select_list.length == 1)
		{
			y1 -= script.ui.style.spacing;
		}
		else
		{
			y1 += toolbar._height * 0.5;
		}
		
		dummy_overlay.x = x1;
		dummy_overlay.y = y1;
		dummy_overlay.width = x2 - x1;
		dummy_overlay.height = y2 - y1;
		dummy_overlay.visible = true;
		dummy_overlay.force_calculate_bounds();
		
		popup.interactable = !script.space.down;
	}
	
	private void update_properties()
	{
		if(!is_editing)
			return;
		
		// 
		// Check properties
		// 
		
		TextTriggerHandlerData@ data = selected(0);
		
		@selected_z_trigger = data.trigger_type == TextTriggerType::ZTextProp ? data : null;
		@selected_normal_trigger = data.trigger_type == TextTriggerType::Normal ? data : null;
		
		bool same_text = true;
		bool same_hidden = true;
		bool same_colour = true;
		bool same_layer = true;
		bool same_sub_layer = true;
		bool same_rotation = true;
		bool same_scale = true;
		bool same_font = true;
		bool same_font_size = true;
		
		for(uint i = 1; i < select_list.length; i++)
		{
			TextTriggerHandlerData@ data0 = selected(i - 1);
			TextTriggerHandlerData@ data1 = selected(i);
			const bool is_0_normal = data0.trigger_type == TextTriggerType::Normal;
			const bool is_1_normal = data1.trigger_type == TextTriggerType::Normal;
			const bool is_0_z_text = !is_0_normal;
			const bool is_1_z_text = !is_1_normal;
			
			if(is_1_z_text)
			{
				if(@selected_z_trigger == null)
				{
					@selected_z_trigger = data1;
				}
				
				if(is_0_z_text)
				{
					if(same_colour && data0.colour != data1.colour)
					{
						same_colour = false;
					}
					if(same_layer && data0.layer != data1.layer)
					{
						same_layer = false;
					}
					if(same_sub_layer && data0.sub_layer != data1.sub_layer)
					{
						same_sub_layer = false;
					}
					if(same_rotation && data0.rotation != data1.rotation)
					{
						same_rotation = false;
					}
					if(same_scale && data0.scale != data1.scale)
					{
						same_scale = false;
					}
					if(same_font && data0.font != data1.font)
					{
						same_font = false;
					}
					if(same_font_size && data0.font_size != data1.font_size)
					{
						same_font_size = false;
					}
				}
			}
			
			if(is_1_normal)
			{
				if(@selected_normal_trigger == null)
				{
					@selected_normal_trigger = data1;
				}
				
				if(same_hidden && is_0_normal && data0.hidden != data1.hidden)
				{
					same_hidden = false;
				}
			}
			
			if(same_text && data0.text != data1.text)
			{
				same_text = false;
			}
		}
		
		// 
		// Update UI
		// 
		
		ignore_events = true;
		
		const string types = @selected_z_trigger != null && @selected_normal_trigger == null ? 'Z Text' : 'Text';
		window.title = 'Edit ' + types + ' Trigger' + (select_list.length > 1 ? 's' : '') +
			(select_list.length == 1 ? ' [' + data.trigger.id() + ']' : '');
		
		create_visible_checkbox(@selected_normal_trigger != null);
		if(@selected_normal_trigger != null)
		{
			visible_checkbox.state = same_hidden
				? (!selected_normal_trigger.hidden ? CheckboxState::On : CheckboxState::Off)
				: CheckboxState::Indeterminate;
		}
		
		text_box.text = same_text ? data.text : '[Multiple values]';
		text_box.colour = same_text ? script.ui.style.text_clr : multiply_alpha(script.ui.style.text_clr, 0.5);
		text_box.has_colour = !same_text;
		text_box.lock_input = lock_input;
		
		if(@selected_z_trigger != null)
		{
			create_z_properties_container();
			
			const float multi_alpha = 0.5;
			
			colour_swatch.colour = selected_z_trigger.colour;
			colour_swatch.alpha = same_colour ? 1.0 : multi_alpha;
			
			layer_select.set_selected_layer(same_layer ? selected_z_trigger.layer : -1, false, true);
			layer_select.set_selected_sub_layer(same_sub_layer ? selected_z_trigger.sub_layer : -1, false, true);
			
			rotation_wheel.degrees = selected_z_trigger.rotation;
			rotation_wheel.alpha = same_rotation ? 1.0 : multi_alpha;
			
			scale_slider.value = selected_z_trigger.scale;
			scale_slider.alpha = same_scale ? 1.0 : multi_alpha;
			
			selected_font_size = selected_z_trigger.font_size;
			font_select.selected_value = selected_z_trigger.font;
			font_select.alpha = same_font ? 1.0 : multi_alpha;
			font_size_select.alpha = same_font_size ? 1.0 : multi_alpha;
			font_select.allow_reselect = !same_font;
			font_size_select.allow_reselect = !same_font_size;
			
			update_font_sizes();
			
			z_properties_container.visible = true;
		}
		else if(@z_properties_container != null)
		{
			z_properties_container.visible = false;
		}
		
		ignore_events = false;
	}
	
	private void update_font_sizes()
	{
		font_size_select.clear();
		@font_sizes = font::get_valid_sizes(font_select.selected_value);
		
		if(@font_sizes == null)
			return;
		
		int selected_index = -1;
		float closest_dist = 99999;
		
		for(uint i = 0; i < font_sizes.length(); i++)
		{
			const int size = font_sizes[i];
			font_size_select.add_value(size + '', size + '');
			
			const float dist = abs(selected_font_size - size);
			if(dist < closest_dist)
			{
				closest_dist = dist;
				selected_index = i;
			}
		}
		
		font_size_select.selected_index = max(0, selected_index);
	}
	
	private void create_visible_checkbox(const bool visible)
	{
		if(visible)
		{
			if(@visible_checkbox == null)
			{
				@visible_checkbox = Checkbox(script.ui);
				@visible_checkbox.tooltip = PopupOptions(script.ui, 'Visible');
				visible_checkbox.change.on(EventCallback(on_hidden_change));
			}
			
			window.add_title_before(visible_checkbox);
		}
		else
		{
			if(@visible_checkbox != null)
			{
				window.remove_title_before(visible_checkbox);
			}
		}
	}
	
	private DataSetMode get_event_mode(EventInfo@ event, const bool opened, const bool cancelled) const
	{
		return opened
			? DataSetMode::Store
			: cancelled ? DataSetMode::Restore : DataSetMode::Set;
	}
	
	// //////////////////////////////////////////////////////////
	// Events
	// //////////////////////////////////////////////////////////
	
	private void on_toolbar_button_click(EventInfo@ event)
	{
		const string name = event.target.name;
		
		if(name == 'edit')
		{
			if(edit_button.selected)
			{
				start_editing();
			}
			else
			{
				stop_editing(true);
			}
		}
	}
	
	private void on_accept_click(EventInfo@ event)
	{
		stop_editing(true);
	}
	
	private void on_cancel_click(EventInfo@ event)
	{
		stop_editing(false);
	}
	
	// Text Properties
	
	private void on_text_change(EventInfo@ event)
	{
		if(ignore_events)
			return;
		
		const string text = text_box.text;
		for(uint i = 0; i < select_list.length; i++)
		{
			selected(i).text = text;
		}
		
		text_box.has_colour = false;
	}
	
	private void on_text_accept(EventInfo@ event)
	{
		stop_editing(event.type == EventType::ACCEPT);
	}
	
	private void on_hidden_change(EventInfo@ event)
	{
		if(ignore_events)
			return;
		
		for(uint i = 0; i < select_list.length; i++)
		{
			selected(i).hidden = !visible_checkbox.checked;
		}
	}
	
	// Z Properties
	
	void on_colour_change(EventInfo@ event)
	{
		if(ignore_events)
			return;
		if(event.type == EventType::ACCEPT || event.type == EventType::CLOSE)
			return;
		
		const bool opened = event.type == EventType::OPEN;
		const bool cancelled = event.type == EventType::CANCEL;
		const DataSetMode mode = get_event_mode(event, opened, cancelled);
		
		if(opened)
		{
			colour_swatch_open_alpha = colour_swatch.alpha;
		}
		
		for(uint i = 0; i < select_list.length; i++)
		{
			selected(i).set_colour(colour_swatch.colour, mode);
		}
		
		if(!opened)
		{
			colour_swatch.alpha = cancelled ? colour_swatch_open_alpha : 1.0;
		}
	}
	
	void on_layer_select(EventInfo@ event)
	{
		if(ignore_events)
			return;
		if(event.type == EventType::ACCEPT)
			return;
		
		const bool opened = event.type == EventType::OPEN;
		const bool cancelled = event.type == EventType::CANCEL;
		const DataSetMode mode = get_event_mode(event, opened, cancelled);
		
		for(uint i = 0; i < select_list.length; i++)
		{
			TextTriggerHandlerData@ data = selected(i);
			
			if(opened || layer_button.layer_changed)
			{
				data.set_layer(layer_select.selected_layer, mode);
			}
			
			if(opened || layer_button.sub_layer_changed)
			{
				data.set_sub_layer(layer_select.selected_sub_layer, mode);
			}
		}
	}
	
	void on_rotation_change(EventInfo@ event)
	{
		if(ignore_events)
			return;
		
		for(uint i = 0; i < select_list.length; i++)
		{
			selected(i).rotation = int(rotation_wheel.degrees);
		}
		
		rotation_wheel.alpha = 1.0;
	}
	
	void on_scale_change(EventInfo@ event)
	{
		if(ignore_events)
			return;
		
		for(uint i = 0; i < select_list.length; i++)
		{
			selected(i).scale = scale_slider.value;
		}
		
		scale_slider.alpha = 1.0;
	}
	
	void on_font_change(EventInfo@ event)
	{
		if(ignore_events)
			return;
		
		for(uint i = 0; i < select_list.length; i++)
		{
			selected(i).font = font_select.selected_value;
		}
		
		ignore_next_font_size_update = true;
		update_font_sizes();
		
		font_select.alpha = 1.0;
		font_select.allow_reselect = false;
	}
	
	void on_font_size_change(EventInfo@ event)
	{
		if(ignore_events)
			return;
		
		if(font_size_select.selected_index == -1)
			return;
		
		const int new_size = font_sizes[font_size_select.selected_index];
		for(uint i = 0; i < select_list.length; i++)
		{
			selected(i).font_size = new_size;
		}
		
		if(!ignore_next_font_size_update)
		{
			selected_font_size = font_sizes[font_size_select.selected_index];
			font_size_select.alpha = 1.0;
			font_size_select.allow_reselect = false;
		}
		else
		{
			ignore_next_font_size_update = false;
		}
	}
	
}
