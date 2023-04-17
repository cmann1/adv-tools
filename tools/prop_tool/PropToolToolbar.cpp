#include '../../../../lib/ui3/elements/Checkbox.cpp';
#include '../../../../lib/ui3/elements/ColourSwatch.cpp';
#include '../../../../lib/ui3/elements/extra/PopupButton.cpp';
#include '../../../../lib/ui3/elements/extra/SelectButton.cpp';
#include '../../../../lib/ui3/elements/Select.cpp';
#include '../../../../lib/ui3/elements/Toolbar.cpp';
#include '../../../../lib/ui3/elements/NumberSlider.cpp';
#include '../../../../lib/ui3/layouts/GridLayout.cpp';
#include 'PropExportType.cpp';

const string EMBED_spr_origin_centre				= PROP_TOOL_SPRITES_BASE + 'origin_centre.png';
const string EMBED_spr_origin_top					= PROP_TOOL_SPRITES_BASE + 'origin_top.png';
const string EMBED_spr_origin_top_left				= PROP_TOOL_SPRITES_BASE + 'origin_top_left.png';
const string EMBED_spr_prop_tool_align_centre		= PROP_TOOL_SPRITES_BASE + 'prop_tool_align_centre.png';
const string EMBED_spr_prop_tool_align_left			= PROP_TOOL_SPRITES_BASE + 'prop_tool_align_left.png';
const string EMBED_spr_prop_tool_custom_anchor_lock	= PROP_TOOL_SPRITES_BASE + 'prop_tool_custom_anchor_lock.png';
const string EMBED_spr_prop_tool_custom_anchor_snap	= PROP_TOOL_SPRITES_BASE + 'prop_tool_custom_anchor_snap.png';
const string EMBED_spr_prop_tool_custom_grid		= PROP_TOOL_SPRITES_BASE + 'prop_tool_custom_grid.png';
const string EMBED_spr_prop_tool_dist_centre		= PROP_TOOL_SPRITES_BASE + 'prop_tool_dist_centre.png';
const string EMBED_spr_prop_tool_dist_left			= PROP_TOOL_SPRITES_BASE + 'prop_tool_dist_left.png';
const string EMBED_spr_prop_tool_show_info			= PROP_TOOL_SPRITES_BASE + 'prop_tool_show_info.png';
const string EMBED_spr_prop_tool_show_selection		= PROP_TOOL_SPRITES_BASE + 'prop_tool_show_selection.png';
const string EMBED_spr_prop_tool_tiles_blocking		= PROP_TOOL_SPRITES_BASE + 'prop_tool_tiles_blocking.png';
const string EMBED_spr_prop_tool_export				= PROP_TOOL_SPRITES_BASE + 'prop_tool_export.png';
const string EMBED_spr_prop_tool_correct			= PROP_TOOL_SPRITES_BASE + 'prop_tool_correct.png';
const string EMBED_spr_prop_tool_highlight			= PROP_TOOL_SPRITES_BASE + 'prop_tool_highlight.png';
const string EMBED_spr_prop_tool_outline			= PROP_TOOL_SPRITES_BASE + 'prop_tool_outline.png';

class PropToolToolbar
{
	
	private AdvToolScript@ script;
	private PropTool@ tool;
	
	private Toolbar@ toolbar;
	private PopupOptions@ info_popup;
	private Label@ info_label;
	
	private Button@ custom_grid_button;
	private PopupOptions@ custom_grid_tooltip;
	private PopupOptions@ custom_grid_popup;
	private NumberSlider@ custom_grid_slider;
	
	private SelectButton@ origin_button;
	private Image@ origin_img;
	private ListView@ origin_list_view;
	private PopupOptions@ origin_tooltip;
	
	private Button@ custom_anchor_snap_button;
	private Button@ correct_button;
	
	private Button@ align_button;
	private PopupOptions@ align_tooltip;
	private PopupOptions@ align_popup;
	
	private Button@ distribute_button;
	private PopupOptions@ distribute_tooltip;
	private PopupOptions@ distribute_popup;
	
	private Image@ highlight_img;
	private Image@ outline_img;
	
	private PopupButton@ export_button;
	private Select@ export_type_select;
	private ColourSwatch@ export_colour_swatch;
	private Checkbox@ export_override_layer_checkbox;
	private Checkbox@ export_override_sublayer_checkbox;
	private NumberSlider@ export_layer_slider;
	private NumberSlider@ export_sublayer_slider;
	
	void build_sprites(message@ msg)
	{
		build_sprite(msg, 'origin_centre');
		build_sprite(msg, 'origin_top');
		build_sprite(msg, 'origin_top_left');
		build_sprite(msg, 'prop_tool_align_centre');
		build_sprite(msg, 'prop_tool_align_left');
		build_sprite(msg, 'prop_tool_custom_anchor_lock');
		build_sprite(msg, 'prop_tool_custom_anchor_snap');
		build_sprite(msg, 'prop_tool_custom_grid');
		build_sprite(msg, 'prop_tool_dist_centre');
		build_sprite(msg, 'prop_tool_dist_left');
		build_sprite(msg, 'prop_tool_show_info');
		build_sprite(msg, 'prop_tool_show_selection');
		build_sprite(msg, 'prop_tool_tiles_blocking');
		build_sprite(msg, 'prop_tool_export');
		build_sprite(msg, 'prop_tool_correct');
		build_sprite(msg, 'prop_tool_highlight');
		build_sprite(msg, 'prop_tool_outline');
	}
	
	void show(AdvToolScript@ script, PropTool@ tool)
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
		toolbar.name = 'PropToolToolbar';
		toolbar.x = 20;
		toolbar.y = 20;
		
		Button@ button;
		EventCallback@ button_click = EventCallback(on_toolbar_button_click);
		
		// Pick through tiles button
		//{
		@button = toolbar.create_button(SPRITE_SET, 'prop_tool_tiles_blocking', Settings::IconSize, Settings::IconSize);
		script.init_icon(button);
		button.name = 'pick_through_tiles';
		button.selectable = true;
		button.selected = tool.pick_through_tiles;
		@button.tooltip = PopupOptions(ui, 'Ignore Tiles');
		button.mouse_click.on(button_click);
		//}
		
		// Custom grid button
		//{
		@button = @custom_grid_button = toolbar.create_button(SPRITE_SET, 'prop_tool_custom_grid', Settings::IconSize, Settings::IconSize);
		script.init_icon(button);
		button.name = 'custom_grid';
		@button.tooltip = @custom_grid_tooltip = PopupOptions(ui, '');
		button.mouse_click.on(button_click);
		update_custom_grid_tooltip();
		
		@custom_grid_slider = NumberSlider(ui);
		custom_grid_slider.value = tool.custom_grid;
		custom_grid_slider.change.on(EventCallback(on_custom_grid_slider_change));
		
		@custom_grid_popup = PopupOptions(ui, custom_grid_slider, true, PopupPosition::Below, PopupTriggerType::Manual, PopupHideType::MouseDownOutside, false);
		custom_grid_popup.wait_for_mouse = true;
		custom_grid_popup.spacing = style.spacing;
		custom_grid_popup.padding = 0;
		custom_grid_popup.background_colour = 0;
		custom_grid_popup.blur_inset = 0;
		custom_grid_popup.show.on(EventCallback(on_custom_grid_slider_popup_show));
		custom_grid_popup.hide.on(EventCallback(on_custom_grid_slider_popup_hide));
		//}
		
		// Orgin button
		//{
		float icon_rotation;
		@origin_img = Image(ui, '', '', Settings::IconSize, Settings::IconSize);
		update_origin_icon();
		
		@origin_button = SelectButton(ui, origin_img);
		origin_button.name = 'default_origin';
		@origin_button.tooltip = PopupOptions(ui, '');
		origin_button.mouse_click.on(button_click);
		origin_button.open.on(EventCallback(on_origin_select_open));
		script.init_icon(origin_button);
		toolbar.add(origin_button);
		update_origin_tooltip();
		
		@origin_list_view = origin_button.list_view;
		origin_list_view.select.on(EventCallback(on_origin_select));
		
		for(int i = 0, count = int(PropToolSettings::Origins.length()); i < count; i++)
		{
			float rotation;
			ListViewItem@ item = origin_list_view.add_item(PropToolSettings::Origins[i],
				SPRITE_SET, get_origin_icon(PropToolSettings::Origins[i], rotation),
				string::nice(PropToolSettings::Origins[i]), Settings::IconSize, Settings::IconSize);
			item.icon.rotation = rotation;
		}
		//}
		
		// Custom anchor lock button and snap
		//{
		@button = toolbar.create_button(SPRITE_SET, 'prop_tool_custom_anchor_lock', Settings::IconSize, Settings::IconSize);
		button.name = 'custom_anchor_lock';
		button.selectable = true;
		button.selected = tool.custom_anchor_lock;
		@button.tooltip = PopupOptions(ui, 'Lock custom anchor');
		button.mouse_click.on(button_click);
		script.init_icon(button);
		
		@button = toolbar.create_button(SPRITE_SET, 'prop_tool_custom_anchor_snap', Settings::IconSize, Settings::IconSize);
		button.name = 'custom_anchor_snap';
		@button.tooltip = PopupOptions(ui, 'Snap anchor');
		button.mouse_click.on(button_click);
		@custom_anchor_snap_button = button;
		script.init_icon(button);
		//}
		
		// Correct
		//{
		@correct_button = toolbar.create_button(SPRITE_SET, 'prop_tool_correct', Settings::IconSize, Settings::IconSize);
		correct_button.name = 'correct';
		@correct_button.tooltip = PopupOptions(ui, 'Correct values');
		correct_button.mouse_click.on(button_click);
		script.init_icon(correct_button);
		//}
		
		// Align button
		//{
		@align_button = toolbar.create_button(SPRITE_SET, 'prop_tool_align_left', Settings::IconSize, Settings::IconSize);
		align_button.name = 'align';
		@align_button.tooltip = PopupOptions(ui, 'Align');
		align_button.mouse_click.on(button_click);
		script.init_icon(align_button);
		
		Container@ align_buttons = create_align_buttons();
		
		@align_popup = PopupOptions(ui, align_buttons, true, PopupPosition::Below, PopupTriggerType::Manual, PopupHideType::MouseDownOutside, false);
		align_popup.wait_for_mouse = true;
		align_popup.padding = 0;
		align_popup.spacing = style.spacing;
		align_popup.show.on(EventCallback(on_align_popup_show));
		align_popup.hide.on(EventCallback(on_align_popup_hide));
		//}
		
		// Distribute button
		//{
		@distribute_button = toolbar.create_button(SPRITE_SET, 'prop_tool_dist_left', Settings::IconSize, Settings::IconSize);
		distribute_button.name = 'distribute';
		@distribute_button.tooltip = PopupOptions(ui, 'Distribute');
		distribute_button.mouse_click.on(button_click);
		script.init_icon(distribute_button);
		
		Container@ distribute_buttons = create_distribute_buttons();
		
		@distribute_popup = PopupOptions(ui, distribute_buttons, true, PopupPosition::Below, PopupTriggerType::Manual, PopupHideType::MouseDownOutside, false);
		distribute_popup.wait_for_mouse = true;
		distribute_popup.padding = 0;
		distribute_popup.spacing = style.spacing;
		distribute_popup.show.on(EventCallback(on_distribute_popup_show));
		distribute_popup.hide.on(EventCallback(on_distribute_popup_hide));
		//}
		
		toolbar.create_divider();
		
		// Highlight button
		//{
		@highlight_img = Image(ui, SPRITE_SET, 'prop_tool_highlight');
		@outline_img = Image(ui, SPRITE_SET, 'prop_tool_outline');
		script.init_icon(highlight_img);
		script.init_icon(outline_img);
		highlight_img.x = 6;
		highlight_img.y = 4;
		Container@ highlight_container = Container(ui);
		highlight_container.add_child(highlight_img);
		highlight_container.add_child(outline_img);
		highlight_container.fit_to_contents(true);
		@button = Button(ui, highlight_container);
		button.name = 'highlight';
		@button.tooltip = PopupOptions(ui, 'Highlight/Outline selected props');
		button.mouse_click.on(button_click);
		toolbar.add(button);
		update_highlight_button();
		//}
		
		// Selection button
		//{
		@button = toolbar.create_button(SPRITE_SET, 'prop_tool_show_selection', Settings::IconSize, Settings::IconSize);
		button.name = 'show_selection';
		button.selectable = true;
		button.selected = tool.show_selection;
		@button.tooltip = PopupOptions(ui, 'Highlight selection while dragging');
		button.mouse_click.on(button_click);
		script.init_icon(button);
		//}
		
		// Info button
		//{
		@button = toolbar.create_button(SPRITE_SET, 'prop_tool_show_info', Settings::IconSize, Settings::IconSize);
		button.name = 'show_info';
		button.selectable = true;
		button.selected = tool.show_info;
		@button.tooltip = PopupOptions(ui, 'Show prop info');
		button.mouse_click.on(button_click);
		script.init_icon(button);
		//}
		
		// Export button
		//{
		@export_button = PopupButton(ui, SPRITE_SET, 'prop_tool_export', Settings::IconSize, Settings::IconSize);
		@export_button.tooltip = PopupOptions(ui, 'Export');
		toolbar.add(export_button);
		script.init_icon(export_button);
		
		Container@ export_contents = Container(ui);
		
		// Export type
		@export_type_select = Select(ui);
		export_type_select.add_value('0', 'SpriteBatch');
		export_type_select.add_value('1', 'SpriteGroup');
		export_type_select.width = 160;
		export_type_select.selected_index = PropExportType::SpriteBatch;
		export_contents.add_child(export_type_select);
		
		// Export layer
		@export_override_layer_checkbox = Checkbox(ui);
		@export_layer_slider = NumberSlider(ui, 0, -20, 20, 1);
		export_layer_slider.x = export_type_select.x;
		export_layer_slider.y = export_type_select.y + export_type_select.height + style.spacing;
		export_layer_slider.width = export_type_select.width - export_override_layer_checkbox.width - style.spacing;
		@export_layer_slider.tooltip = PopupOptions(ui, 'Layer');
		export_contents.add_child(export_layer_slider);
		
		// Export override layer
		export_override_layer_checkbox.height = export_layer_slider.height;
		export_override_layer_checkbox.x = export_layer_slider.x + export_layer_slider.width + style.spacing;
		export_override_layer_checkbox.y = export_layer_slider.y;
		@export_override_layer_checkbox.tooltip = PopupOptions(ui, 'Override/Add layer');
		export_contents.add_child(export_override_layer_checkbox);
		
		// Export sublayer
		@export_override_sublayer_checkbox = Checkbox(ui);
		@export_sublayer_slider = NumberSlider(ui, 0, -24, 24, 1);
		export_sublayer_slider.x = export_layer_slider.x;
		export_sublayer_slider.y = export_layer_slider.y + export_layer_slider.height + style.spacing;
		export_sublayer_slider.width = export_layer_slider.width;
		@export_sublayer_slider.tooltip = PopupOptions(ui, 'Sublayer');
		export_contents.add_child(export_sublayer_slider);
		
		// Export override sublayer
		export_override_sublayer_checkbox.x = export_override_layer_checkbox.x;
		export_override_sublayer_checkbox.y = export_sublayer_slider.y;
		export_override_sublayer_checkbox.height = export_sublayer_slider.height;
		@export_override_sublayer_checkbox.tooltip = PopupOptions(ui, 'Override/Add sublayer');
		export_contents.add_child(export_override_sublayer_checkbox);
		
		// Export button
		@button = Button(ui, 'Export');
		button.name = 'export';
		button.fit_to_contents();
		button.x = export_type_select.x + export_type_select.width - button.width;
		button.y = export_sublayer_slider.y + export_sublayer_slider.height + style.spacing;
		button.mouse_click.on(button_click);
		export_contents.add_child(button);
		
		// Export colour swatch
		@export_colour_swatch = ColourSwatch(ui);
		export_colour_swatch.x = export_type_select.x;
		export_colour_swatch.y = button.y;
		export_colour_swatch.width  = button.height;
		export_colour_swatch.height = button.height;
		export_colour_swatch.colour = 0xffffffff;
		@export_colour_swatch.tooltip = PopupOptions(ui, 'Colour');
		export_colour_swatch.activate.on(EventCallback(on_export_colour_activate));
		export_contents.add_child(export_colour_swatch);
		
		export_contents.fit_to_contents(true);
		@export_button.popup.content_element = export_contents;
		export_button.popup.padding = style.spacing;
		//}
		
		ui.add_child(toolbar);
		script.window_manager.register_element(toolbar);
		
		update_buttons(0);
	}
	
	private Container@ create_align_buttons()
	{
		UI@ ui = script.ui;
		Style@ style = ui.style;
		
		Container@ c = Container(ui);
		@c.layout = GridLayout(ui, 3);
		EventCallback@ align_button_click = EventCallback(on_align_button_click);
		
		Button@ button;
		
		create_button(c, 'left', 'Left Edges', 'align_left', 0, 1, 1, align_button_click);
		create_button(c, 'centre', 'Horizontal Centres', 'align_centre', 0, 1, 1, align_button_click);
		create_button(c, 'right', 'Right Edges', 'align_left', 0, -1, 1, align_button_click);
		
		create_button(c, 'top', 'Top Edges', 'align_left', 90, 1, -1, align_button_click);
		create_button(c, 'middle', 'Vertical Centres', 'align_centre', -90, 1, 1, align_button_click);
		create_button(c, 'bottom', 'Bottom Edges', 'align_left', -90, 1, 1, align_button_click);
		
		c.fit_to_contents(true);
		return c;
	}
	
	private Container@ create_distribute_buttons()
	{
		UI@ ui = script.ui;
		Style@ style = ui.style;
		
		Container@ c = Container(ui);
		@c.layout = GridLayout(ui, 3);
		EventCallback@ distibute_button_click = EventCallback(on_distribute_button_click);
		
		Button@ button;
		
		create_button(c, 'left', 'Left Edges', 'dist_left', 0, 1, 1, distibute_button_click);
		create_button(c, 'centre', 'Horizontal Centres', 'dist_centre', 0, 1, 1, distibute_button_click);
		create_button(c, 'right', 'Right Edges', 'dist_left', 0, -1, 1, distibute_button_click);
		
		create_button(c, 'top', 'Top Edges', 'dist_left', 90, 1, -1, distibute_button_click);
		create_button(c, 'middle', 'Vertical Centres', 'dist_centre', -90, 1, 1, distibute_button_click);
		create_button(c, 'bottom', 'Bottom Edges', 'dist_left', -90, 1, 1, distibute_button_click);
		
		create_button(c, 'horizontal', 'Spaced Horizontally', 'dist_centre', 0, 1, 1, distibute_button_click);
		create_button(c, 'vertical', 'Spaced Vertically', 'dist_centre', -90, 1, 1, distibute_button_click);
		
		c.fit_to_contents(true);
		return c;
	}
	
	Button@ create_button(Container@ c, const string type, const string tooltip, const string icon, const float rotation, const float scale_x, const float scale_y, EventCallback@ click_handler)
	{
		Button@ button = Button(script.ui, SPRITE_SET, 'prop_tool_' + icon, Settings::IconSize, Settings::IconSize);
		button.name = type;
		button.icon.rotation = rotation;
		button.icon.scale_x = scale_x;
		button.icon.scale_y = scale_y;
		button.mouse_click.on(click_handler);
		@button.tooltip = PopupOptions(script.ui, tooltip);
		button.fit_to_contents();
		c.add_child(button);
		
		return button;
	}
	
	private void update_custom_grid_tooltip()
	{
		custom_grid_tooltip.content_string = 'Custom grid: ' + int(tool.custom_grid) + 'px';
	}
	
	private void update_origin_tooltip()
	{
		origin_button.tooltip.content_string = 'Origin: ' + string::nice(tool.default_origin);
	}
	
	private string get_origin_icon(const string origin, float &out rotation)
	{
		const int index = PropToolSettings::Origins.find(origin);
		
		if(index <= 0)
		{
			rotation = 0;
			return 'origin_centre';
		}
		
		rotation = (index - 1) / 2 * 90;
		return ((index - 1) % 2) == 0 ? 'origin_top_left' : 'origin_top';
	}
	
	private void update_origin_icon()
	{
		float rotation;
		origin_img.set_sprite(SPRITE_SET, get_origin_icon(tool.default_origin, rotation), Settings::IconSize, Settings::IconSize);
		origin_img.rotation = rotation;
	}
	
	private void update_highlight_button()
	{
		highlight_img.alpha = (tool.highlight_selection & Highlight != 0) ? 1.0 : 0.5;
		outline_img.alpha = (tool.highlight_selection & Outline != 0) ? 1.0 : 0.5;
	}
	
	// //////////////////////////////////////////////////////////
	// Methods
	// //////////////////////////////////////////////////////////
	
	void show_info(const string &in text)
	{
		if(!tool.show_info)
			return;
		
		script.show_info_popup(text, toolbar);
	}
	
	void show_prop_info(PropData@ prop_data)
	{
		if(!tool.show_info)
			return;
		
		prop@ p = prop_data.prop;
		int index = prop_index_to_array_index(p.prop_set(), p.prop_group(), p.prop_index());
		const PropIndex@ prop_index = @PROP_INDICES[p.prop_group()][index];
		
		string text =
			// Group and name
			string::nice(PROP_GROUP_NAMES[p.prop_group()]) + '::' + prop_index.name + '\n' +
			prop_data.sprite_set + '::' + prop_data.sprite_name + '\n' +
			'[' + p.prop_set() + '.' + p.prop_group() + '.' + p.prop_index() + '|' + p.palette() + '] [' + p.id() + ']\n' +
			// Layer
			p.layer() + '.' + p.sub_layer();
		
		// Scale
		if(p.scale_x() != 1 && p.scale_x() != -1)
		{
			text += '\nScale: ' + string::nice_float(p.scale_x(), 4);
		}
		
		script.show_info_popup(text, toolbar);
	}
	
	void update_buttons(const int selected_props_count)
	{
		align_button.disabled = selected_props_count < 2;
		distribute_button.disabled = selected_props_count < 3;
		export_button.disabled = selected_props_count == 0;
	}
	
	private AlignmentEdge alignment_from_name(const string name)
	{
		if(name == 'top')
			return Top;
		
		if(name == 'middle')
			return Middle;
		
		if(name == 'bottom')
			return Bottom;
		
		if(name == 'left')
			return Left;
		
		if(name == 'centre')
			return Centre;
		
		if(name == 'right')
			return Right;
		
		if(name == 'vertical')
			return Vertical;
		
		if(name == 'horizontal')
			return Horizontal;
		
		return None;
	}
	
	// //////////////////////////////////////////////////////////
	// Events
	// //////////////////////////////////////////////////////////
	
	private void on_toolbar_button_click(EventInfo@ event)
	{
		Button@ button = cast<Button@>(event.target);
		const string name = button.name;
		
		if(name == 'pick_through_tiles')
		{
			tool.pick_through_tiles = button.selected;
		}
		else if(name == 'custom_grid')
		{
			custom_grid_button.selected = true;
			custom_grid_button.selectable = true;
			script.ui.show_tooltip(custom_grid_popup, custom_grid_button);
		}
		else if(name == 'custom_anchor_lock')
		{
			tool.custom_anchor_lock = button.selected;
		}
		else if(name == 'custom_anchor_snap')
		{
			tool.snap_custom_anchor();
		}
		else if(name == 'highlight')
		{
			tool.cycle_highlight_selection();
			update_highlight_button();
		}
		else if(name == 'show_selection')
		{
			tool.show_selection = button.selected;
		}
		else if(name == 'show_info')
		{
			tool.show_info = button.selected;
		}
		else if(name == 'correct')
		{
			tool.correct_prop_values();
		}
		else if(name == 'align')
		{
			align_button.selected = true;
			align_button.selectable = true;
			script.ui.show_tooltip(align_popup, align_button);
		}
		else if(name == 'distribute')
		{
			distribute_button.selected = true;
			distribute_button.selectable = true;
			script.ui.show_tooltip(distribute_popup, distribute_button);
		}
		else if(name == 'export')
		{
			tool.export_selected_props(
				PropExportType(export_type_select.selected_index),
				int(export_layer_slider.value),
				int(export_sublayer_slider.value),
				export_override_layer_checkbox.checked,
				export_override_sublayer_checkbox.checked,
				export_colour_swatch.colour);
			export_button.close();
		}
	}
	
	// Grid
	
	private void on_custom_grid_slider_change(EventInfo@ event)
	{
		tool.custom_grid = round(custom_grid_slider.value);
		update_custom_grid_tooltip();
	}
	
	private void on_custom_grid_slider_popup_show(EventInfo@ event)
	{
		custom_grid_tooltip.enabled = false;
	}
	
	private void on_custom_grid_slider_popup_hide(EventInfo@ event)
	{
		custom_grid_button.selectable = false;
		custom_grid_tooltip.enabled = true;
	}
	
	// Origin
	
	private void on_origin_select_open(EventInfo@ event)
	{
		if(event.type != EventType::BEFORE_OPEN)
			return;
		
		origin_list_view.select.enabled = false;
		origin_list_view.select_none();
		origin_list_view.select_item(tool.default_origin);
		origin_list_view.select.enabled = true;
	}
	
	private void on_origin_select(EventInfo@ event)
	{
		if(tool.default_origin == event.value)
			return;
		
		tool.default_origin = event.value;
		tool.update_alignments_from_origin(true);
		
		update_origin_icon();
		update_origin_tooltip();
	}
	
	// Align
	
	private void on_align_popup_show(EventInfo@ event)
	{
		align_button.tooltip.enabled = false;
	}
	
	private void on_align_popup_hide(EventInfo@ event)
	{
		align_button.selectable = false;
		align_button.tooltip.enabled = true;
	}
	
	private void on_align_button_click(EventInfo@ event)
	{
		tool.align(alignment_from_name(event.target.name));
	}
	
	// Distribute
	
	private void on_distribute_popup_show(EventInfo@ event)
	{
		distribute_button.tooltip.enabled = false;
	}
	
	private void on_distribute_popup_hide(EventInfo@ event)
	{
		distribute_button.selectable = false;
		distribute_button.tooltip.enabled = true;
	}
	
	private void on_distribute_button_click(EventInfo@ event)
	{
		tool.distribute(alignment_from_name(event.target.name));
	}
	
	// Export
	
	private void on_export_colour_activate(EventInfo@ event)
	{
		export_button.popup.locked = event.type == EventType::OPEN;
	}
	
}
