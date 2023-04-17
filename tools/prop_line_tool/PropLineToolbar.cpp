#include '../../../../lib/ui3/elements/MultiButton.cpp';
#include '../../../../lib/ui3/elements/NumberSlider.cpp';
#include '../../../../lib/ui3/elements/extra/SelectButton.cpp';
#include '../../../../lib/ui3/elements/Toolbar.cpp';

const string EMBED_spr_prop_line_snap_angle				= PROP_LINE_TOOL_SPRITES_BASE + 'prop_line_snap_angle.png';
const string EMBED_spr_prop_line_spacing_fixed			= PROP_LINE_TOOL_SPRITES_BASE + 'prop_line_spacing_fixed.png';
const string EMBED_spr_prop_line_spacing_fill			= PROP_LINE_TOOL_SPRITES_BASE + 'prop_line_spacing_fill.png';
const string EMBED_spr_prop_line_auto_spacing			= PROP_LINE_TOOL_SPRITES_BASE + 'prop_line_auto_spacing.png';
const string EMBED_spr_prop_line_scroll_mode_spacing	= PROP_LINE_TOOL_SPRITES_BASE + 'prop_line_scroll_mode_spacing.png';
const string EMBED_spr_prop_line_scroll_mode_rotation	= PROP_LINE_TOOL_SPRITES_BASE + 'prop_line_scroll_mode_rotation.png';
const string EMBED_spr_prop_line_rotation_auto			= PROP_LINE_TOOL_SPRITES_BASE + 'prop_line_rotation_auto.png';

class PropLineToolbar
{
	
	private AdvToolScript@ script;
	private PropLineTool@ tool;
	
	private Toolbar@ toolbar;
	private MultiButton@ snap_mode_btn;
	private MultiButton@ spacing_btn;
	private NumberSlider@ spacing_slider;
	private Button@ rotation_btn;
	private NumberSlider@ rotation_slider;
	private NumberSlider@ repeat_count_slider;
	private NumberSlider@ repeat_spacing_slider;
	private Button@ auto_spacing_btn;
	private MultiButton@ scroll_mode_btn;
	
	void build_sprites(message@ msg)
	{
		build_sprite(msg, 'prop_line_snap_angle');
		build_sprite(msg, 'prop_line_spacing_fixed');
		build_sprite(msg, 'prop_line_spacing_fill');
		build_sprite(msg, 'prop_line_auto_spacing');
		build_sprite(msg, 'prop_line_scroll_mode_spacing');
		build_sprite(msg, 'prop_line_scroll_mode_rotation');
		build_sprite(msg, 'prop_line_rotation_auto');
	}
	
	void show(AdvToolScript@ script, PropLineTool@ tool)
	{
		if(@this.script == null)
		{
			@this.script = script;
			@this.tool = tool;
			
			create_ui();
		}
		
		script.ui.add_child(toolbar);
	}
	
	void hide()
	{
		script.ui.remove_child(toolbar);
	}
	
	private void create_ui()
	{
		UI@ ui = script.ui;
		Style@ style = ui.style;
		
		@toolbar = Toolbar(ui, true, true);
		toolbar.name = 'PropLineToolToolbar';
		toolbar.x = 20;
		toolbar.y = 20;
		
		EventCallback@ button_select = EventCallback(on_toolbar_button_select);
		
		// Auto spacing button
		//{
			@auto_spacing_btn = toolbar.create_button(SPRITE_SET, 'prop_line_auto_spacing');
			auto_spacing_btn.selectable = true;
			auto_spacing_btn.name = 'auto_spacing';
			@auto_spacing_btn.tooltip = PopupOptions(ui, 'Auto spacing');
			auto_spacing_btn.select.on(button_select);
			toolbar.add(auto_spacing_btn);
			script.init_icon(auto_spacing_btn);
		//}
		
		// Spacing button
		//{
			@spacing_btn = MultiButton(ui);
			spacing_btn.add('fixed', SPRITE_SET, 'prop_line_spacing_fixed', Settings::IconSize, Settings::IconSize);
			spacing_btn.set_tooltip('Spacing: Fixed');
			spacing_btn.add('fill', SPRITE_SET, 'prop_line_spacing_fill', Settings::IconSize, Settings::IconSize);
			spacing_btn.set_tooltip('Spacing: Fill');
			spacing_btn.name = 'spacing';
			spacing_btn.fit_to_contents();
			spacing_btn.select.on(button_select);
			toolbar.add_child(spacing_btn);
			script.init_icon(spacing_btn);
		//}
		
		// Spacing slider
		//{
			@spacing_slider = NumberSlider(ui, 0, NAN, NAN, 0.1);
			@spacing_slider.tooltip = PopupOptions(ui, 'Spacing');
			spacing_slider.change.on(EventCallback(on_spacing_slider_change));
			spacing_slider.label_precision = 1;
			toolbar.add_child(spacing_slider);
		//}
		
		toolbar.create_divider();
		
		// Rotation button
		//{
			@rotation_btn = toolbar.create_button(SPRITE_SET, 'prop_line_rotation_auto');
			rotation_btn.selectable = true;
			rotation_btn.name = 'rotation';
			@rotation_btn.tooltip = PopupOptions(ui, 'Auto rotation');
			rotation_btn.select.on(button_select);
			toolbar.add(rotation_btn);
			rotation_btn.width = spacing_btn.width;
			script.init_icon(rotation_btn);
		//}
		
		// Rotation slider
		//{
			@rotation_slider = NumberSlider(ui, 0, NAN, NAN, 0.1);
			@rotation_slider.tooltip = PopupOptions(ui, 'Rotation');
			rotation_slider.change.on(EventCallback(on_rotation_slider_change));
			rotation_slider.label_precision = 1;
			toolbar.add_child(rotation_slider);
		//}
		
		toolbar.create_divider();
		
		// Repeat count slider
		//{
			@repeat_count_slider = NumberSlider(ui, 1, 1);
			@repeat_count_slider.tooltip = PopupOptions(ui, 'Repeat Count');
			repeat_count_slider.change.on(EventCallback(on_repeat_count_slider_change));
			toolbar.add_child(repeat_count_slider);
		//}
		
		// Repeat spacing slider
		//{
			@repeat_spacing_slider = NumberSlider(ui, 0, NAN, NAN, 0.1);
			@repeat_spacing_slider.tooltip = PopupOptions(ui, 'Repeat spacing');
			repeat_spacing_slider.change.on(EventCallback(on_repeat_spacing_slider_change));
			repeat_spacing_slider.label_precision = 1;
			toolbar.add_child(repeat_spacing_slider);
		//}
		
		toolbar.create_divider();
		
		// Snap mode button
		//{
			@snap_mode_btn = MultiButton(ui);
			snap_mode_btn.add('grid', SPRITE_SET, 'prop_tool_custom_grid');
			snap_mode_btn.set_tooltip('Snap: Grid');
			snap_mode_btn.add('angle', SPRITE_SET, 'prop_line_snap_angle');
			snap_mode_btn.set_tooltip('Snap: Angle');
			snap_mode_btn.name = 'snap';
			snap_mode_btn.fit_to_contents();
			snap_mode_btn.select.on(button_select);
			toolbar.add_child(snap_mode_btn);
			script.init_icon(snap_mode_btn);
		//}
		
		// Scroll mode button
		//{
			@scroll_mode_btn = MultiButton(ui);
			scroll_mode_btn.add('spacing', SPRITE_SET, 'prop_line_scroll_mode_spacing', Settings::IconSize, Settings::IconSize);
			scroll_mode_btn.set_tooltip('Scroll mode: Spacing');
			scroll_mode_btn.add('rotation', SPRITE_SET, 'prop_line_scroll_mode_rotation', Settings::IconSize, Settings::IconSize);
			scroll_mode_btn.set_tooltip('Scroll mode: Rotation');
			scroll_mode_btn.name = 'scroll_mode';
			scroll_mode_btn.fit_to_contents();
			scroll_mode_btn.select.on(button_select);
			toolbar.add_child(scroll_mode_btn);
			script.init_icon(scroll_mode_btn);
		//}
		
		update_snap_mode();
		update_spacing_mode();
		update_rotation_mode();
		update_spacing();
		update_spacing_offset();
		update_auto_spacing();
		update_rotation();
		update_rotation_offset();
		update_repeat_count();
		update_repeat_spacing();
		update_scroll_mode();
		
		ui.add_child(toolbar);
		script.window_manager.register_element(toolbar);
	}
	
	void update_snap_mode()
	{
		snap_mode_btn.selected_index = tool.snap_to_grid ? 0 : 1;
	}
	
	void update_spacing_mode()
	{
		spacing_btn.selected_index = int(tool.spacing_mode);
	}
	
	void update_rotation_mode()
	{
		rotation_btn.selected = tool.rotation_mode == PropLineRotationMode::Auto;
		rotation_slider.value = rotation_btn.selected ? tool.rotation_offset : tool.rotation;
	}
	
	void update_spacing()
	{
		if(!tool.auto_spacing)
		{
			spacing_slider.value = tool.spacing;
		}
	}
	
	void update_spacing_offset()
	{
		if(tool.auto_spacing)
		{
			spacing_slider.value = tool.spacing_offset;
		}
	}
	
	void update_rotation()
	{
		if(tool.rotation_mode == PropLineRotationMode::Fixed)
		{
			rotation_slider.value = tool.rotation;
		}
	}
	
	void update_rotation_offset()
	{
		if(tool.rotation_mode == PropLineRotationMode::Auto)
		{
			rotation_slider.value = tool.rotation_offset;
		}
	}
	
	void update_repeat_count()
	{
		repeat_count_slider.value = tool.repeat_count;
	}
	
	void update_repeat_spacing()
	{
		repeat_spacing_slider.value = tool.repeat_spacing;
	}
	
	void update_auto_spacing()
	{
		auto_spacing_btn.selected = tool.auto_spacing;
		spacing_slider.min_value = tool.auto_spacing ? NAN : 0;
		spacing_slider.value = tool.auto_spacing ? tool.spacing_offset : tool.spacing;
	}
	
	void update_scroll_mode()
	{
		scroll_mode_btn.selected_index = tool.scroll_spacing ? 0 : 1;
	}
	
	// //////////////////////////////////////////////////////////
	// Events
	// //////////////////////////////////////////////////////////
	
	private void on_toolbar_button_select(EventInfo@ event)
	{
		const string name = event.target.name;
		
		if(name == 'snap')
		{
			tool.update_snap_mode(snap_mode_btn.selected_index == 0, false);
		}
		else if(name == 'spacing')
		{
			tool.update_spacing_mode(PropLineSpacingMode(spacing_btn.selected_index), false);
		}
		else if(name == 'auto_spacing')
		{
			tool.update_auto_spacing(auto_spacing_btn.selected, false);
		}
		else if(name == 'rotation')
		{
			tool.update_rotation_mode(rotation_btn.selected ? PropLineRotationMode::Auto : PropLineRotationMode::Fixed, false);
		}
		else if(name == 'scroll_mode')
		{
			tool.update_scroll_mode(scroll_mode_btn.selected_index == 0, false);
		}
	}
	
	private void on_spacing_slider_change(EventInfo@ event)
	{
		if(tool.auto_spacing)
		{
			tool.update_spacing_offset(spacing_slider.value, false);
		}
		else
		{
			tool.update_spacing(spacing_slider.value, false);
		}
	}
	
	private void on_rotation_slider_change(EventInfo@ event)
	{
		if(tool.rotation_mode == PropLineRotationMode::Auto)
		{
			tool.update_rotation_offset(rotation_slider.value, false);
		}
		else
		{
			tool.update_rotation(rotation_slider.value, false);
		}
	}
	
	private void on_repeat_count_slider_change(EventInfo@ event)
	{
		tool.update_repeat_count(int(repeat_count_slider.value), false);
	}
	
	private void on_repeat_spacing_slider_change(EventInfo@ event)
	{
		tool.update_repeat_spacing(repeat_spacing_slider.value, false);
	}
	
}
