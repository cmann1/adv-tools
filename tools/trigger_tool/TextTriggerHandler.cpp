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

class TextTriggerHandler : TriggerToolHandler
{
	
	/** The firsts selected z trigger if there is one. */
	private TextTriggerHandlerData@ selected_z_trigger;
	/** The firsts selected text trigger if there is one. */
	private TextTriggerHandlerData@ selected_normal_trigger;
	
	private Toolbar@ toolbar;
	
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
	
	private const array<int>@ font_sizes;
	private int selected_font_size;
	
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
		
		update_selected_popup_position(TriggerHandlerState::Idle);
	}
	
	void draw(const float sub_frame) override
	{
		if(state == TriggerHandlerState::Idle)
		{
			draw_selected_popup_connections();
		}
		
		if(state == TriggerHandlerState::Rotating || state == TriggerHandlerState::Scaling)
		{
			float x1, y1, x2, y2;
			script.world_to_hud(selected_z_trigger.trigger.x(), selected_z_trigger.trigger.y(), x1, y1, false);
			script.world_to_hud(script.mouse.x, script.mouse.y, x2, y2, false);
			script.ui.style.draw_line(x1, y1, x2, y2, 4, (state == TriggerHandlerState::Rotating ? 0x9c3edf : 0xd85d45) | 0xbb000000);
		}
	}
	
	// //////////////////////////////////////////////////////////
	// Methods
	// //////////////////////////////////////////////////////////
	
	protected bool sub_ui_active() override
	{
		if(script.ui.is_mouse_active)
			return false;
		if(@colour_swatch == null)
			return false;
		
		return colour_swatch.open || layer_button.open;
	}
	
	/// Handle shortcut keys like Escape and Enter.
	private void check_keys()
	{
		if(state == TriggerHandlerState::Idle)
		{
			if(check_edit_keys() == 1)
			{
				lock_input = true;
				text_box.lock_input = true;
			}
		}
		
		// Consume Enter to prevent the default editing.
		if(@selected_trigger != null && !script.ui_focus && !sub_ui_active() && script.return_press)
		{
			script.input.key_clear_gvb(GVB::Return);
		}
	}
	
	private void check_mouse()
	{
		if(state == TriggerHandlerState::Idle)
		{
			if(!check_mouse_multi_select() && @selected_z_trigger != null)
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
			ignore_edit_ui_events = true;
			rotation_wheel.degrees = selected_z_trigger.rotation;
			ignore_edit_ui_events = false;
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
			ignore_edit_ui_events = true;
			scale_slider.value = selected_z_trigger.scale;
			ignore_edit_ui_events = false;
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
		do_selection_change_for_editing(true, primary, added, removed);
		
		if(!primary && select_list.length != 0)
		{
			if(!is_editing)
			{
				update_z_trigger(removed);
			}
		}
		else
		{
			update_z_trigger(true);
		}
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
			update_edit_properties();
		}
	}
	
	protected void start_editing() override
	{
		TriggerToolHandler::start_editing();
		
		@script.ui.focus = text_box;
		text_box.select_all();
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
	
	protected Element@ create_selected_popup_content() override
	{
		UI@ ui = script.ui;
		Style@ style = ui.style;
		
		@toolbar = Toolbar(ui, false, true);
		toolbar.name = 'TriggerToolTextToolbar';
		
		@edit_button = script.create_toolbar_button(toolbar, 'edit', 'icon_edit', 'Edit');
		edit_button.selectable = true;
		edit_button.mouse_click.on(EventCallback(on_toolbar_button_click));
		
		toolbar.fit_to_contents(true);
		
		return toolbar;
	}
	
	protected void create_edit_window() override
	{
		UI@ ui = script.ui;
		Style@ style = ui.style;
		
		@edit_window = Window(ui, 'Edit Text');
		edit_window.resizable = true;
		edit_window.name = 'TextToolTextProperties';
		edit_window.set_icon(SPRITE_SET, 'icon_edit', Settings::IconSize, Settings::IconSize);
		edit_window.x = 200;
		edit_window.y = 20;
		edit_window.min_width = 450;
		edit_window.min_height = 350;
		edit_window.width  = edit_window.min_width;
		edit_window.height = edit_window.min_height;
		@edit_window.layout = AnchorLayout(ui).set_padding(0);
		edit_window.close.on(EventCallback(on_cancel_click));
		
		@text_box = TextBox(ui);
		text_box.multi_line = true;
		text_box.select_all_on_focus = false;
		text_box.accept_on_blur = false;
		text_box.width  = 50;
		text_box.height = 30;
		text_box.anchor_left.pixel(0);
		text_box.anchor_right.pixel(0);
		text_box.anchor_top.pixel(0);
		text_box.anchor_bottom.pixel(0);
		text_box.change.on(EventCallback(on_text_change));
		text_box.accept.on(EventCallback(on_text_accept));
		edit_window.add_child(text_box);
		
		Button@ btn = Button(ui, 'Accept');
		btn.fit_to_contents();
		btn.mouse_click.on(EventCallback(on_accept_click));
		edit_window.add_button_left(btn);
		
		@btn = Button(ui, 'Cancel');
		btn.fit_to_contents();
		btn.mouse_click.on(EventCallback(on_cancel_click));
		edit_window.add_button_right(btn);
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
		
		Label@ font_label = script.create_label('Font', z_properties_container);
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
		
		Label@ font_size_label = script.create_label('Size', z_properties_container);
		font_size_label.anchor_right.sibling(font_size_select).padding(style.spacing);
		font_size_label.x = font_size_select.x - font_size_label.width - style.spacing;
		font_size_label.y = font_size_select.y;
		font_size_label.height = font_size_select.height;
		
		z_properties_container.fit_to_contents(true);
		@z_properties_container.layout = AnchorLayout(script.ui).set_padding(0);
		
		edit_window.add_child(z_properties_container);
	}
	
	protected void update_edit_properties() override
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
		
		const string types = @selected_z_trigger != null && @selected_normal_trigger == null ? 'Z Text' : 'Text';
		edit_window.title = 'Edit ' + types + ' Trigger' + (select_list.length > 1 ? 's' : '') +
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
			
			edit_window.add_title_before(visible_checkbox);
		}
		else
		{
			if(@visible_checkbox != null)
			{
				edit_window.remove_title_before(visible_checkbox);
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
		if(ignore_edit_ui_events)
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
		if(ignore_edit_ui_events)
			return;
		
		for(uint i = 0; i < select_list.length; i++)
		{
			selected(i).hidden = !visible_checkbox.checked;
		}
	}
	
	// Z Properties
	
	void on_colour_change(EventInfo@ event)
	{
		if(ignore_edit_ui_events)
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
		if(ignore_edit_ui_events)
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
		if(ignore_edit_ui_events)
			return;
		
		for(uint i = 0; i < select_list.length; i++)
		{
			selected(i).rotation = int(rotation_wheel.degrees);
		}
		
		rotation_wheel.alpha = 1.0;
	}
	
	void on_scale_change(EventInfo@ event)
	{
		if(ignore_edit_ui_events)
			return;
		
		for(uint i = 0; i < select_list.length; i++)
		{
			selected(i).scale = scale_slider.value;
		}
		
		scale_slider.alpha = 1.0;
	}
	
	void on_font_change(EventInfo@ event)
	{
		if(ignore_edit_ui_events)
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
		if(ignore_edit_ui_events)
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
