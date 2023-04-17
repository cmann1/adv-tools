#include '../../../../lib/ui3/elements/Button.cpp';
#include '../../../../lib/ui3/elements/ColourSwatch.cpp';
#include '../../../../lib/ui3/elements/Container.cpp';
#include '../../../../lib/ui3/elements/Label.cpp';
#include '../../../../lib/ui3/elements/LayerButton.cpp';
#include '../../../../lib/ui3/elements/NumberSlider.cpp';
#include '../../../../lib/ui3/elements/RotationWheel.cpp';
#include '../../../../lib/ui3/elements/Select.cpp';
#include '../../../../lib/ui3/elements/TextBox.cpp';
#include '../../../../lib/ui3/elements/Window.cpp';
#include '../../../../lib/ui3/layouts/AnchorLayout.cpp';

const string EMBED_spr_icon_text	= SPRITES_BASE + 'icon_text.png';

class TextTool : Tool
{
	
	private Container@ dummy_overlay;
	private PopupOptions@ popup;
	private Button@ edit_button;
	
	private entity@ hovered_trigger;
	private entity@ selected_trigger;
	private entity@ selected_trigger_original;
	private bool is_z_trigger;
	private varstruct@ vars;
	private varvalue@ text_var;
	
	private Window@ window;
	private Checkbox@ hidden_checkbox;
	private TextBox@ text_box;
	private Container@ properties_container;
	
	private ColourSwatch@ colour_swatch;
	private LayerButton@ layer_button;
	private LayerSelector@ layer_select;
	private RotationWheel@ rotation_wheel;
	private NumberSlider@ scale_slider;
	private Select@ font_select;
	private Select@ font_size_select;
	
	private bool ignore_events;
	
	private const array<int>@ font_sizes;
	private int selected_font_size;
	private bool ignore_next_font_size_update;
	
	TextTool(AdvToolScript@ script)
	{
		super(script, 'Text Tool');
		
		selectable = false;
	}
	
	void on_init() override
	{
		Tool@ tool = script.get_tool('Triggers');
		
		if(@tool != null)
		{
			tool.register_sub_tool(this);
		}
	}
	
	void build_sprites(message@ msg) override
	{
		build_sprite(msg, 'icon_text');
	}
	
	bool create_window()
	{
		if(@window != null)
			return false;
		
		UI@ ui = script.ui;
		Style@ style = ui.style;
		
		@window = Window(ui, 'Edit Text');
		window.resizable = true;
		window.min_width = 450;
		window.min_height = 350;
		window.name = 'TextToolTextProperties1';
		window.set_icon(SPRITE_SET, 'icon_text', 24, 24);
		ui.add_child(window);
		window.x = 200;
		window.y = 20;
		window.width  = 200;
		window.height = 200;
		window.contents.autoscroll_on_focus = false;
		window.close.on(EventCallback(on_cancel_click));
		
		@text_box = TextBox(ui);
		text_box.multi_line = true;
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
	
	void init_content_layout()
	{
		@window.layout = AnchorLayout(script.ui).set_padding(0);
	}
	
	void create_properties_container()
	{
		if(@properties_container != null)
			return;
		
		UI@ ui = script.ui;
		Style@ style = ui.style;
		
		@properties_container = Container(ui);
		properties_container.y = text_box.y + text_box.height + ui.style.spacing;
		properties_container.width = text_box.width;
		properties_container.anchor_left.pixel(0);
		properties_container.anchor_right.pixel(0);
		properties_container.anchor_bottom.pixel(0);
		
		text_box.anchor_bottom.sibling(properties_container).padding(style.spacing);
		
		@rotation_wheel = RotationWheel(ui);
		properties_container.add_child(rotation_wheel);
		
		// Colour swatch
		
		@colour_swatch = ColourSwatch(ui);
		colour_swatch.width  = rotation_wheel.height;
		colour_swatch.height = rotation_wheel.height;
		@colour_swatch.tooltip = PopupOptions(ui, 'Colour');
		colour_swatch.change.on(EventCallback(on_colour_change));
		properties_container.add_child(colour_swatch);
		
		// Layer button
		
		@layer_button = LayerButton(ui);
		layer_button.height = rotation_wheel.height;
		layer_button.auto_close = false;
		layer_button.x = colour_swatch.x + colour_swatch.width + style.spacing;
		@layer_button.tooltip = PopupOptions(ui, 'Layer');
		layer_button.change.on(EventCallback(on_layer_select));
		layer_button.select.on(EventCallback(on_layer_select));
		properties_container.add_child(layer_button);
		
		@layer_select = layer_button.layer_select;
		layer_select.multi_select = false;
		layer_select.min_select = 1;
		layer_select.min_select_layers = 1;
		
		// ROtation wheel
		
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
		properties_container.add_child(scale_slider);
		
		// Font select
		
		@font_select = Select(ui);
		font_select.anchor_right.pixel(0);
		properties_container.add_child(font_select);
		
		font_select.add_value(font::ENVY_BOLD, 'Envy Bold');
		font_select.add_value(font::SANS_BOLD, 'Sans Bold');
		font_select.add_value(font::CARACTERES, 'Caracteres');
		font_select.add_value(font::PROXIMANOVA_REG, 'ProximaNovaReg');
		font_select.add_value(font::PROXIMANOVA_THIN, 'ProximaNovaThin');
		
		font_select.width = 200;
		font_select.x = properties_container.width - font_select.width;
		font_select.y = colour_swatch.y;
		font_select.change.on(EventCallback(on_font_change));
		
		Label@ font_label = create_label('Font');
		font_label.anchor_right.sibling(font_select).padding(style.spacing);
		font_label.y = font_select.y;
		font_label.height = font_select.height;
		
		// Font size select
		
		@font_size_select = Select(ui);
		font_size_select.anchor_right.pixel(0);
		properties_container.add_child(font_size_select);
		
		font_size_select.width = 85;
		font_size_select.x = properties_container.width - font_size_select.width;
		font_size_select.y = font_select.y + font_select.height + style.spacing;
		font_size_select.change.on(EventCallback(on_font_size_change));
		
		Label@ font_size_label = create_label('Size');
		font_size_label.anchor_right.sibling(font_size_select).padding(style.spacing);
		font_size_label.x = font_size_select.x - font_size_label.width - style.spacing;
		font_size_label.y = font_size_select.y;
		font_size_label.height = font_size_select.height;
		
		properties_container.fit_to_contents(true);
		@properties_container.layout = AnchorLayout(script.ui).set_padding(0);
		
		window.add_child(properties_container);
	}
	
	private Label@ create_label(const string text)
	{
		Label@ label = Label(script.ui, text);
		label.set_padding(script.ui.style.spacing, script.ui.style.spacing, 0, 0);
		label.align_v = GraphicAlign::Middle;
		label.fit_to_contents();
		properties_container.add_child(label);
		
		return label;
	}
	
	// //////////////////////////////////////////////////////////
	// Tool Callbacks
	// //////////////////////////////////////////////////////////
	
	protected void on_editor_unloaded_impl() override
	{
		if(@window != null)
		{
			window.hide('user', false);
		}
		
		select(null);
		show_edit_button(null, true);
	}
	
	protected void on_select_impl() override
	{
		select(null);
		show_edit_button(null, true);
	}
	
	protected void step_impl() override
	{
		if(@selected_trigger != null)
		{
			if(@script.ui.focus == null)
			{
				if(script.escape_press)
				{
					if(!colour_swatch.open)
					{
						on_cancel_click(null);
					}
				}
				else if(script.return_press)
				{
					if(!colour_swatch.open)
					{
						select(null);
					}
				}
			}
			else if(text_box.has_focus)
			{
				if(!script.ctrl.down)
				{
					script.input.key_clear_gvb(GVB::Return);
				}
			}
			
			if(@selected_trigger != null && selected_trigger.destroyed())
			{
				select(null);
				show_edit_button(null, true);
			}
		}
		
		if(@hovered_trigger != null && hovered_trigger.destroyed())
		{
			@hovered_trigger = null;
			show_edit_button(null, true);
		}
		
		if(@hovered_trigger != null)
		{
			update_popup();
		}
		
		if(!script.mouse.left_down)
		{
			pick_trigger();
		}
	}
	
	protected void draw_impl(const float sub_frame) override
	{
		if(@selected_trigger == null)
			return;
		
		float x1, y1, x2, y2, _;
		script.world_to_hud(selected_trigger.x(), selected_trigger.y(), x1, y1, false);
		
		if(x1 < window.x1 || x1 > window.x2 || y1 < window.y1 || y1 > window.y2)
		{
			const float line_width = 2;
			const uint colour = multiply_alpha(script.ui.style.normal_bg_clr, 0.5);
			
			closest_point_to_rect(x1, y1,
				window.x1 + line_width, window.y1 + line_width,
				window.x2 - line_width, window.y2 - line_width,
				x2, y2);
			
			script.ui.style.draw_line(x1, y1, x2, y2, line_width, colour);
		}
	}
	
	// //////////////////////////////////////////////////////////
	// Methods
	// //////////////////////////////////////////////////////////
	
	private void pick_trigger()
	{
		entity@ closest = null;
		
		if(!script.ui.is_mouse_over_ui && !script.mouse_in_gui)
		{
			int i = script.g.get_entity_collision(script.mouse.y - 20, script.mouse.y + 20,
				script.mouse.x - 20, script.mouse.x + 20, ColType::Trigger);
			
			float closest_dist = MAX_FLOAT;
			
			while(i-- > 0)
			{
				entity@ e = script.g.get_entity_collision_index(i);
				
				if(e.type_name() != 'text_trigger' && e.type_name() != 'z_text_prop_trigger')
					continue;
				
				if(e.is_same(selected_trigger))
					continue;
				
				const float dist = dist_sqr(e.x(), e.y(), script.mouse.x, script.mouse.y);
				
				if(dist < closest_dist)
				{
					closest_dist = dist;
					@closest = e;
				}
			}
		}
		
		show_edit_button(closest);
	}
	
	private void show_edit_button(entity@ e, const bool force=false)
	{
		if(@e == null)
		{
			if(@dummy_overlay == null || !dummy_overlay.visible)
				return;
			
			if(force || @hovered_trigger == null || !dummy_overlay.check_mouse() || script.ui.is_mouse_over_ui && !popup.content_element.check_mouse())
			{
				
				dummy_overlay.visible = false;
				@hovered_trigger = null;
				script.ui.hide_tooltip(popup);
			}
			
			return;
		}
		
		create_popup();
		
		if(@hovered_trigger == null || !hovered_trigger.is_same(e))
		{
			@hovered_trigger = e;
			update_popup();
		}
	}
	
	private void update_popup()
	{
		float x1, y1, x2, y2;
		script.world_to_hud(hovered_trigger.x() - 10, hovered_trigger.y() - 10, x1, y1);
		script.world_to_hud(hovered_trigger.x() + 10, hovered_trigger.y() + 10, x2, y2);
		
		y1 -= edit_button._height + script.ui.style.spacing;
		const float diff = (x2 - x1) - edit_button._width;
		
		if(diff < 0)
		{
			x1 += diff * 0.5;
			x2 -= diff * 0.5;
		}
		
		dummy_overlay.x = x1;
		dummy_overlay.y = y1;
		dummy_overlay.width = x2 - x1;
		dummy_overlay.height = y2 - y1;
		dummy_overlay.visible = true;
		dummy_overlay.force_calculate_bounds();
		
		popup.interactable = !script.space.down;
		
		script.ui.move_to_back(dummy_overlay);
		script.ui.show_tooltip(popup, dummy_overlay);
	}
	
	private void create_popup()
	{
		if(@popup != null)
			return;
		
		@dummy_overlay = Container(script.ui);
		//dummy_overlay.background_colour = 0x55ff0000;
		dummy_overlay.mouse_self = false;
		script.ui.add_child(dummy_overlay);
		
		@edit_button = Button(script.ui, SPRITE_SET, 'icon_edit', 24, 24);
		edit_button.fit_to_contents(true);
		edit_button.mouse_click.on(EventCallback(on_edit_click));
		script.init_icon(edit_button);
		
		@popup = PopupOptions(script.ui, edit_button, true, PopupPosition::InsideTop, PopupTriggerType::Manual, PopupHideType::Manual);
		popup.spacing = 0;
		popup.padding = 0;
		popup.background_colour = 0;
		popup.border_colour = 0;
		popup.shadow_colour = 0;
		popup.background_blur = false;
	}
	
	private void select(entity@ e)
	{
		@selected_trigger = e;
		@selected_trigger_original = null;
		
		if(@selected_trigger == null)
		{
			if(@window != null)
			{
				window.hide();
			}
			
			return;
		}
		
		const bool is_window_created = create_window();
		
		@selected_trigger_original = create_entity(selected_trigger.type_name());
		copy_vars(selected_trigger, selected_trigger_original);
		
		is_z_trigger = e.type_name() != 'text_trigger';
		@vars = selected_trigger.vars();
		@text_var = vars.get_var(is_z_trigger ? 'text' : 'text_string');
		text_box.text = text_var.get_string();
		
		if(is_z_trigger)
		{
			create_properties_container();
			update_properties();
			properties_container.visible = true;
			
			if(@hidden_checkbox != null)
			{
				window.remove_title_before(hidden_checkbox);
			}
		}
		else
		{
			if(@hidden_checkbox == null)
			{
				@hidden_checkbox = Checkbox(script.ui);
				@hidden_checkbox.tooltip = PopupOptions(script.ui, 'Visible');
				hidden_checkbox.change.on(EventCallback(on_hidden_change));
			}
			
			window.add_title_before(hidden_checkbox);
			hidden_checkbox.checked = !vars.get_var('hide').get_bool();
			
			if(@properties_container != null)
			{
				properties_container.visible = false;
			}
		}
		
		window.title = is_z_trigger ? 'Edit Z Text Prop' : 'Edit Text Trigger';
		
		if(is_window_created)
		{
			window.fit_to_contents(true);
			window.centre();
			init_content_layout();
			script.window_manager.force_immediate_reposition(window);
		}
		
		window.show();
		@script.ui.focus = text_box;
		
		@hovered_trigger = null;
		script.ui.hide_tooltip(popup);
	}
	
	private void update_properties()
	{
		ignore_events = true;
		
		colour_swatch.colour = vars.get_var('colour').get_int32();
		layer_select.set_selected_layer(vars.get_var('layer').get_int32(), false);
		layer_select.set_selected_sub_layer(vars.get_var('sublayer').get_int32(), false);
		rotation_wheel.degrees = float(vars.get_var('text_rotation').get_int32());
		scale_slider.value = vars.get_var('text_scale').get_float();
		
		selected_font_size = vars.get_var('font_size').get_int32();
		font_select.selected_value = vars.get_var('font').get_string();
		update_font_sizes();
		
		ignore_events = false;
	}
	
	private void update_font_sizes()
	{
		font_size_select.clear();
		@font_sizes = font::get_valid_sizes(font_select.selected_value);
		
		if(@font_sizes == null)
			return;
		
		int selected_index = -1;
		
		for(uint i = 0; i < font_sizes.length(); i++)
		{
			const int size = font_sizes[i];
			font_size_select.add_value(size + '', size + '');
			
			if(size == selected_font_size)
			{
				selected_index = i;
			}
		}
		
		font_size_select.selected_index = max(0, selected_index);
	}
	
	// //////////////////////////////////////////////////////////
	// Events
	// //////////////////////////////////////////////////////////
	
	private void on_edit_click(EventInfo@ event)
	{
		select(hovered_trigger);
	}
	
	void on_accept_click(EventInfo@ event)
	{
		event.type = EventType::ACCEPT;
		on_text_accept(event);
	}
	
	void on_cancel_click(EventInfo@ event)
	{
		if(@selected_trigger != null)
		{
			copy_vars(selected_trigger_original, selected_trigger);
		}
		
		select(null);
	}
	
	void on_hidden_change(EventInfo@ event)
	{
		if(is_z_trigger || @selected_trigger == null)
			return;
		
		vars.get_var('hide').set_bool(!hidden_checkbox.checked);
	}
	
	void on_text_change(EventInfo@ event)
	{
		if(ignore_events)
			return;
		
		text_var.set_string(text_box.text);
	}
	
	void on_text_accept(EventInfo@ event)
	{
		if(event.type == EventType::ACCEPT)
		{
			text_var.set_string(text_box.text);
			select(null);
		}
		else
		{
			on_cancel_click(null);
		}
	}
	
	void on_colour_change(EventInfo@ event)
	{
		if(ignore_events)
			return;
		
		vars.get_var('colour').set_int32(colour_swatch.colour);
	}
	
	void on_layer_select(EventInfo@ event)
	{
		if(ignore_events)
			return;
		
		vars.get_var('layer').set_int32(layer_select.get_selected_layer());
		vars.get_var('sublayer').set_int32(layer_select.get_selected_sub_layer());
	}
	
	void on_rotation_change(EventInfo@ event)
	{
		if(ignore_events)
			return;
		
		vars.get_var('text_rotation').set_int32(int(rotation_wheel.degrees));
	}
	
	void on_scale_change(EventInfo@ event)
	{
		if(ignore_events)
			return;
		
		vars.get_var('text_scale').set_float(scale_slider.value);
	}
	
	void on_font_change(EventInfo@ event)
	{
		if(ignore_events)
			return;
		
		vars.get_var('font').set_string(font_select.selected_value);
		ignore_next_font_size_update = true;
		update_font_sizes();
	}
	
	void on_font_size_change(EventInfo@ event)
	{
		if(ignore_events)
			return;
		
		if(font_size_select.selected_index == -1)
			return;
		
		vars.get_var('font_size').set_int32(font_sizes[font_size_select.selected_index]);
		
		if(!ignore_next_font_size_update)
		{
			selected_font_size = font_sizes[font_size_select.selected_index];
		}
		else
		{
			ignore_next_font_size_update = false;
		}
	}
	
}
