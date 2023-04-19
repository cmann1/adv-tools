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

#include 'EditingTextTriggerData.cpp';
#include 'TriggerToolHandler.cpp';

#include '__temp2.cpp';

namespace TextTriggerType
{
	
	const string Normal = 'text_trigger';
	const string ZTextProp = 'z_text_prop_trigger';
	const string Mixed = 'mixed';
	
}

class TextTriggerHandler : TriggerToolHandler
{
	
	private entity@ trigger;
	private string trigger_type = '';
	
	private Container@ dummy_overlay;
	private PopupOptions@ popup;
	private Toolbar@ toolbar;
	private Button@ edit_button;
	
	private Window@ window;
	private Checkbox@ visible_checkbox;
	private TextBox@ text_box;
	private Container@ properties_container;
	
	private ColourSwatch@ colour_swatch;
	private LayerButton@ layer_button;
	private LayerSelector@ layer_select;
	private RotationWheel@ rotation_wheel;
	private NumberSlider@ scale_slider;
	private Select@ font_select;
	private Select@ font_size_select;
	
	private array<EditingTextTriggerData@> editing;
	private string editing_type;
	
	private bool ignore_events;
	/// Lock the text box input for one frame after pressing Enter to start editing to
	/// prevent the Enter/text event being processed by the TextBox and overwrites the selected text.
	private bool lock_input;
	
	TextTriggerHandler(AdvToolScript@ script)
	{
		super(script);
	}
	
	void build_sprites(message@ msg) override
	{
		build_sprite(msg, 'icon_text');
	}
	
	bool should_handle(entity@ trigger, const string &in type) override
	{
		return type == 'text_trigger' || type == 'z_text_prop_trigger';
	}
	
	void select(entity@ trigger, const string &in type) override
	{
		update_selected_trigger(trigger, type);
	}
	
	void deselect() override
	{
		update_selected_trigger(null);
		
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
		else if(@popup != null && !popup.popup_visible)
		{
			script.ui.show_tooltip(popup, dummy_overlay);
		}
		
		check_triggers();
		check_keys();
		
		// Multi select
		if(editing.length > 0 && script.shift.down && script.mouse.left_press)
		{
			entity@ new_trigger = pick_trigger();
			if(@new_trigger != null)
			{
				start_editing(new_trigger, true);
			}
		}
		
		if(@trigger != null)
		{
			update_toolbar_position();
		}
	}
	
	void draw(const float sub_frame) override
	{
		for(uint i = 0; i < editing.length; i++)
		{
			EditingTextTriggerData@ data = editing[i];
			float x1, y1, x2, y2, _;
			script.world_to_hud(data.trigger.x(), data.trigger.y(), x1, y1, false);
			
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
	}
	
	// //////////////////////////////////////////////////////////
	// Methods
	// //////////////////////////////////////////////////////////
	
	/// Removes/unselects triggers that have been deleted.
	private void check_triggers()
	{
		if(editing.length > 0)
		{
			bool changed = false;
			
			for(int i = int(editing.length) - 1; i >= 0; i--)
			{
				EditingTextTriggerData@ data = @editing[i];
				if(data.trigger.destroyed())
				{
					editing.removeAt(i);
					changed = true;
				}
			}
			
			if(editing.length == 0)
			{
				stop_editing(true);
			}
			else if(changed)
			{
				update_editing_ui_for_triggers();
			}
		}
		
		if(@trigger != null && trigger.destroyed())
		{
			if(editing.length > 0)
			{
				@trigger = editing[0].trigger;
				@script.editor.selected_trigger = trigger;
			}
			else
			{
				update_selected_trigger(null);
			}
		}
	}
	
	/// Handle shortcut keys like Escape and Enter.
	private void check_keys()
	{
		// Start editing with Enter.
		if(@trigger != null && editing.length == 0 && script.consume_pressed_gvb(GVB::Return))
		{
			lock_input = true;
			start_editing(trigger);
		}
		
		// Accept/Cancel with Enter/Escape when the textbox isn't focused..
		if(@script.ui.focus == null)
		{
			if(@colour_swatch == null || !colour_swatch.open)
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
		if(@trigger != null && script.return_press)
		{
			if(!script.ctrl.down && script.return_press)
			{
				script.input.key_clear_gvb(GVB::Return);
			}
		}
	}
	
	private void update_selected_trigger(entity@ trigger, const string &in type='')
	{
		if(@trigger == @this.trigger)
			return;
		
		@this.trigger = trigger;
		trigger_type = type;
		
		if(@trigger != null)
		{
			if(@popup == null)
			{
				create_toolbar();
			}
			
			script.ui.add_child(dummy_overlay);
			script.ui.move_to_back(dummy_overlay);
			update_toolbar_position();
			script.ui.show_tooltip(popup, dummy_overlay);
			
			if(editing.length > 0)
			{
				stop_editing(true, false);
				start_editing(trigger);
			}
		}
		else
		{
			script.ui.hide_tooltip(popup, script.in_editor);
			script.ui.remove_child(dummy_overlay);
			
			stop_editing(true);
		}
	}
	
	private void start_editing(entity@ trigger, const bool toggle=false)
	{
		if(@trigger == null)
			return;
		
		for(uint i = 0; i < editing.length; i++)
		{
			EditingTextTriggerData@ data = editing[i];
			if(trigger.is_same(data.trigger))
			{
				// Can't toggle the main trigger.
				if(toggle && !trigger.is_same(this.trigger))
				{
					editing.removeAt(i);
					update_editing_ui_for_triggers();
				}
				return;
			}
		}
		
		const bool is_window_created = create_window();
		
		EditingTextTriggerData@ data = EditingTextTriggerData(trigger);
		editing.insertLast(data);
		
		update_editing_ui_for_triggers();
		
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
	}
	
	private void stop_editing(const bool accept, const bool update_ui=true)
	{
		const string text = accept && @text_box != null ? text_box.text : '';
		
		for(uint i = 0; i < editing.length; i++)
		{
			EditingTextTriggerData@ data = editing[i];
			
			if(accept)
			{
				if(editing.length == 1)
				{
					data.text_var.set_string(text);
				}
			}
			else
			{
				copy_vars(data.restore_data, data.trigger);
			}
		}
		
		editing.resize(0);
		
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
	
	private entity@ pick_trigger()
	{
		if(script.ui.is_mouse_over_ui || script.mouse_in_gui)
			return null;
		
		entity@ closest = null;
		float closest_dist = MAX_FLOAT;
		
		int i = script.g.get_entity_collision(
			script.mouse.y, script.mouse.y,
			script.mouse.x, script.mouse.x,
			ColType::Trigger);
		
		while(i-- > 0)
		{
			entity@ e = script.g.get_entity_collision_index(i);
			
			const string type = e.type_name();
			if(type != TextTriggerType::Normal && type != TextTriggerType::ZTextProp)
				continue;
			
			if(e.is_same(trigger))
				continue;
			
			const float dist = dist_sqr(e.x(), e.y(), script.mouse.x, script.mouse.y);
			
			if(dist < closest_dist)
			{
				closest_dist = dist;
				@closest = e;
			}
		}
		
		return closest;
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
		
		@popup = PopupOptions(script.ui, toolbar, true, PopupPosition::InsideTop, PopupTriggerType::Manual, PopupHideType::Manual);
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
	
	private void update_toolbar_position()
	{
		float x1, y1, x2, y2;
		script.world_to_hud(trigger.x() - 10, trigger.y() - 10, x1, y1);
		script.world_to_hud(trigger.x() + 10, trigger.y() + 10, x2, y2);
		
		y1 -= toolbar._height + script.ui.style.spacing;
		const float diff = (x2 - x1) - toolbar._width;
		
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
	}
	
	private void update_editing_ui_for_triggers()
	{
		// 
		// Check properties
		// 
		
		EditingTextTriggerData@ data = editing[0];
		editing_type = data.trigger_type;
		bool same_text = true;
		bool same_hidden = true;
		EditingTextTriggerData@ z_trigger = editing_type == TextTriggerType::ZTextProp ? data : null;
		EditingTextTriggerData@ normal_trigger = editing_type == TextTriggerType::Normal ? data : null;
		
		for(uint i = 1; i < editing.length; i++)
		{
			EditingTextTriggerData@ data0 = editing[i - 1];
			EditingTextTriggerData@ data1 = editing[i];
			
			if(@z_trigger == null && data1.trigger_type == TextTriggerType::ZTextProp)
			{
				@z_trigger = data1;
			}
			if(data1.trigger_type == TextTriggerType::Normal)
			{
				if(@normal_trigger == null)
				{
					@normal_trigger = data1;
				}
				
				if(
					same_hidden && data0.trigger_type == TextTriggerType::Normal &&
					data0.hidden != data1.hidden
				)
				{
					same_hidden = false;
				}
			}
			
			if(editing_type != TextTriggerType::Mixed)
			{
				if(data0.trigger_type != data1.trigger_type)
				{
					editing_type = TextTriggerType::Mixed;
				}
				else
				{
					editing_type = data0.trigger_type;
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
		
		const string types = @z_trigger != null && @normal_trigger == null ? 'Z Text' : 'Text';
		window.title = 'Edit ' + types + ' Trigger' + (editing.length > 1 ? 's' : '') +
			(editing.length == 1 ? ' [' + data.trigger.id() + ']' : '');
		
		create_visible_checkbox(@normal_trigger != null);
		if(@normal_trigger != null)
		{
			visible_checkbox.state = same_hidden
				? (!normal_trigger.hidden ? CheckboxState::On : CheckboxState::Off)
				: CheckboxState::Indeterminate;
		}
		
		text_box.text = same_text ? data.text : '[Multiple values]';
		text_box.colour = same_text ? script.ui.style.text_clr : multiply_alpha(script.ui.style.text_clr, 0.5);
		text_box.has_colour = !same_text;
		text_box.lock_input = lock_input;
		
		ignore_events = false;
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
				start_editing(trigger);
			}
			else
			{
				stop_editing(true);
			}
		}
	}
	
	void on_accept_click(EventInfo@ event)
	{
		stop_editing(true);
	}
	
	void on_cancel_click(EventInfo@ event)
	{
		stop_editing(false);
	}
	
	void on_text_change(EventInfo@ event)
	{
		if(ignore_events)
			return;
		
		const string text = text_box.text;
		for(uint i = 0; i < editing.length; i++)
		{
			EditingTextTriggerData@ data = editing[i];
			data.text = text;
		}
		
		text_box.has_colour = false;
	}
	
	void on_text_accept(EventInfo@ event)
	{
		stop_editing(event.type == EventType::ACCEPT);
	}
	
	
	void on_hidden_change(EventInfo@ event)
	{
		if(ignore_events)
			return;
		
		for(uint i = 0; i < editing.length; i++)
		{
			EditingTextTriggerData@ data = editing[i];
			if(data.trigger_type == TextTriggerType::Normal)
			{
				data.hidden = !visible_checkbox.checked;
			}
		}
	}
	
}
