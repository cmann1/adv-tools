#include '../../lib/std.cpp';
#include '../../lib/layer.cpp';
#include '../../lib/string.cpp';
#include '../../lib/math/math.cpp';
#include '../../lib/math/geom.cpp';
#include '../../lib/input/VK.cpp';
#include '../../lib/embed_utils.cpp';
#include '../../lib/drawing/common.cpp';
#include '../../lib/drawing/circle.cpp';
#include '../../lib/debug/Debug.cpp';
#include '../../lib/enums/ColType.cpp';
#include '../../lib/input/common.cpp';
#include '../../lib/input/GVB.cpp';
#include '../../lib/input/Mouse.cpp';
#include '../../lib/math/Line.cpp';
#include '../../lib/utils/colour.cpp';
#include '../../lib/utils/copy_vars.cpp';
#include '../../lib/utils/print_vars.cpp';

#include '../../lib/ui3/UI.cpp';
#include '../../lib/ui3/elements/Toolbar.cpp';
#include '../../lib/ui3/popups/PopupOptions.cpp';
#include '../../lib/ui3/window_manager/WindowManager.cpp';

#include '../../lib/debug/Debug.cpp';

#include 'handles/Handles.cpp';
#include 'misc/DragHandleType.cpp';
#include 'misc/EditorKey.cpp';
#include 'misc/InfoOverlay.cpp';
#include 'misc/IWorldBoundingBox.cpp';
#include 'misc/LayerInfoDisplay.cpp';
#include 'misc/ShortcutKeySorter.cpp';
#include 'misc/WorldBoundingBox.cpp';
#include 'settings/Config.cpp';
#include 'settings/Settings.cpp';
#include 'tools/edge_brush/EdgeBrushTool.cpp';
#include 'tools/emitter_tool/EmitterTool.cpp';
#include 'tools/shape_tool/ShapeTool.cpp';
#include 'tools/prop_line_tool/PropLineTool.cpp';
#include 'tools/prop_tool/PropTool.cpp';
#include 'tools/trigger_tool/ExtendedTriggerTool.cpp';
#include 'tools/ExtendedTileTool.cpp';
#include 'tools/ExtendedPropTool.cpp';
#include 'tools/HelpTool.cpp';
#include 'tools/ParticleEditorTool.cpp';
#include 'ToolGroup.cpp';

const string SCRIPT_BASE				= 'ed/adv_tools/';
const string SPRITES_BASE				= SCRIPT_BASE + 'sprites/';
const string EMBED_spr_icon_edit		= SPRITES_BASE + 'icon_edit.png';
const string EMBED_spr_icon_visible		= SPRITES_BASE + 'icon_visible.png';
const string EMBED_spr_icon_invisible	= SPRITES_BASE + 'icon_invisible.png';

const bool AS_EDITOR_PLUGIN = true;
const string SPRITE_SET = AS_EDITOR_PLUGIN ? 'plugin' : 'script';

class script : AdvToolScript {}

class AdvToolScript
{
	
	scene@ g = get_scene();
	editor_api@ editor =  get_editor_api();
	input_api@ input = get_input_api();
	camera@ cam;
	UI@ ui;
	Mouse@ mouse;
	Handles handles;
	Debug debug();
	Line line;
	sprites@ editor_spr;
	sprites@ script_spr;
	
	float zoom;
	bool mouse_in_gui;
	bool mouse_in_scene;
	bool ui_focus;
	bool scene_focus;
	bool in_editor = true;
	EditorKey ctrl = EditorKey(input, GVB::Control, ModifierKey::Ctrl);
	EditorKey shift = EditorKey(input, GVB::Shift, ModifierKey::Shift);
	EditorKey alt = EditorKey(input, GVB::Alt, ModifierKey::Alt);
	EditorKey space = EditorKey(input, GVB::Space);
	bool return_press, escape_press;
	bool space_on_press;
	bool pressed_in_scene;
	bool shortcut_keys_enabled = true;
	int layer = 19, sub_layer = 19;
	/// Kind of a hack, but can be set to "consume" a single shortcut key.
	/// Any tool shortcuts matching this will not be able to trigger
	ShortcutKey blocked_key;
	
	InfoOverlay info_overlay;
	
	bool debug_ui;
	string clipboard;
	WindowManager window_manager;
	PropsClipboardData props_clipboard;
	string selected_tool_name;
	
	Config config(this);
	
	private bool initialised;
	private bool state_persisted = true;
	private bool queue_load_config;
	private float frame;
	
	private Toolbar@ toolbar;
	private array<ToolGroup> tool_groups;
	private array<Tool@> tools;
	private array<Tool@> tools_shortcut;
	private int num_tools_shortcut;
	private dictionary tool_groups_map;
	private dictionary tools_map;
	private ButtonGroup@ button_group;
	
	private PopupOptions@ shortcut_keys_enabled_popup;
	
	private array<Image@> icon_images;
	
	private PopupOptions@ info_popup;
	private float info_popup_timer;
	private Label@ info_label;
	
	/// '_' = Tool has not been initialised yet
	private string selected_tab = '_';
	private string previous_selected_tab = '_';
	private Tool@ selected_tool;
	private int num_tool_group_popups;
	
	private EventCallback@ on_after_layout_delegate;
	
	private int pressed_key = -1;
	private bool pressed_key_active;
	private int pressed_timer;
	
	private bool ignore_toolbar_select_event;
	
	private bool hide_gui;
	private bool hide_panels_gui;
	private bool hide_layers_gui;
	private bool hide_gui_user = false;
	private bool hide_toolbar_user = false;
	private bool hide_panels_user = false;
	
	//
	
	float view_x, view_y;
	float view_x1, view_y1;
	float view_x2, view_y2;
	float view_w, view_h;
	float screen_w, screen_h;
	
	array<array<float>> layer_scales(23);
	/// Maps a layer index to it's order
	array<int> layer_positions(23);
	/// Maps an order to a layer index
	array<int> layer_indices(23);
	
	int min_layer = 6;
	int min_sub_layer = 0;
	float min_layer_scale = 1;
	int min_fg_layer = 6;
	int min_fg_sub_layer = 0;
	float min_fg_layer_scale = 1;
	
	Toolbar@ main_toolbar { get { return @toolbar; } }
	
	// //////////////////////////////////////////////////////////
	// Init
	// //////////////////////////////////////////////////////////
	
	AdvToolScript()
	{
		puts('>> Initialising AdvTools');
		@cam = get_active_camera();
		
		@ui = UI(true);
		@mouse = Mouse(false, 22, 22);
		mouse.use_input(input);
		
		for(int i = 0; i <= 22; i++)
		{
			layer_scales[i].resize(25);
		}
		
		blocked_key.init(this);
		
		@button_group = ButtonGroup(ui, false);
		
		if(@editor == null)
		{
			initialised = true;
			return;
		}
		
		layer = editor.selected_layer;
		sub_layer = editor.selected_sub_layer;
		
		editor.hide_gui(false);
		do_load_config(false);
		create_tools();
		
		//
		
		@ui.debug = debug;
		debug.text_font = font::ENVY_BOLD;
		debug.text_size = 20;
		debug.text_align_y = 1;
		debug.text_display_newset_first = false;
	}
  
	void editor_loaded()
	{
		in_editor = true;
		
		if(@selected_tool != null)
		{
			selected_tool.on_editor_loaded();
		}
		
		editor.hide_toolbar_gui(true);
		
		editor.hide_gui(hide_gui);
		hide_gui_panels(hide_panels_gui);
		hide_gui_layers(hide_layers_gui);
		
		store_layer_values();
	}

	void editor_unloaded()
	{
		in_editor = false;
		
		if(@selected_tool != null)
		{
			selected_tool.on_editor_unloaded();
		}
		
		editor.hide_toolbar_gui(false);
		
		editor.hide_gui(false);
		editor.hide_panels_gui(false);
		editor.hide_layers_gui(false);
	}
	
	void build_sprites(message@ msg)
	{
		build_sprite(@msg, 'icon_edit', 0, 0);
		build_sprite(@msg, 'icon_visible', 0, 0);
		build_sprite(@msg, 'icon_invisible', 0, 0);
		
		for(uint i = 0; i < tools.length(); i++)
		{
			tools[i].build_sprites(msg);
		}
	}
	
	void reload_config()
	{
		queue_load_config = true;
	}
	
	private bool do_load_config(const bool trigger_load=true)
	{
		if(!config.load())
			return false;
		
		bool requires_reload = false;
		
		if(!requires_reload && !config.compare_float('UISpacing', ui.style.spacing))
			requires_reload = true;
		if(!requires_reload && !config.compare_colour('UIBGColour', ui.style.normal_bg_clr))
			requires_reload = true;
		if(!requires_reload && !config.compare_colour('UIBorderColour', ui.style.normal_border_clr))
			requires_reload = true;
		
		if(!requires_reload)
		{
			initialise_ui_style();
			
			for(uint i = 0; i < icon_images.length; i++)
			{
				icon_images[i].colour = config.UIIconColour;
			}
		}
		
		for(uint i = 0; i < tools.length(); i++)
		{
			// Just easier to reload everything if a shortcut changes
			if(tools[i].reload_shortcut_key())
			{
				return true;
			}
		}
		
		if(trigger_load && !requires_reload)
		{
			for(uint i = 0; i < tools.length(); i++)
			{
				tools[i].on_settings_loaded();
			}
			for(uint i = 0; i < tool_groups.length(); i++)
			{
				tool_groups[i].on_settings_loaded();
			}
			
			ui.after_layout.on(on_after_layout_delegate);
			position_toolbar();
		}
		
		return requires_reload;
	}
	
	private void do_full_reload()
	{
		@ui = UI(true);
		@toolbar = null;
		tool_groups.resize(0);
		tools.resize(0);
		tools_shortcut.resize(0);
		num_tools_shortcut = 0;
		tool_groups_map.deleteAll();
		tools_map.deleteAll();
		icon_images.resize(0);
		selected_tab = '';
		@selected_tool = null;
		num_tool_group_popups = 0;
		
		create_tools();
		initialised = false;
	}
	
	private void initialise()
	{
		@editor_spr = create_sprites();
		editor_spr.add_sprite_set('editor');
		@script_spr = create_sprites();
		script_spr.add_sprite_set(SPRITE_SET);
		
		initialise_ui();
		initialise_tools();
		
		select_tool(selected_tool_name != '' ? selected_tool_name : editor.editor_tab(), false);
		
		info_overlay.init(this);
		handles.init(this);
	}
	
	private void initialise_ui()
	{
		@ui.debug = debug;
		ui.clipboard = clipboard;
		ui.clipboard_change.on(EventCallback(on_clipboard_change));
		ui.auto_fit_screen = true;
		
		initialise_ui_style();
		
		// ui.style.text_clr                        = 0xffffffff;
		// ui.style.normal_bg_clr                   = 0xd9050505;
		// ui.style.normal_border_clr               = 0x33ffffff;
		// ui.style.highlight_bg_clr                = 0xd933307c;
		// ui.style.highlight_border_clr            = 0xd96663c2;
		// ui.style.selected_bg_clr                 = 0xd933307c;
		// ui.style.selected_border_clr             = 0xff7d7acb;
		// ui.style.selected_highlight_bg_clr       = 0xd9423fa0;
		// ui.style.selected_highlight_border_clr   = 0xff7d7acb;
		// ui.style.disabled_bg_clr                 = 0xa6000000;
		// ui.style.disabled_border_clr             = 0x26ffffff;
		// ui.style.secondary_bg_clr                = 0x667f7daf;
		// ui.style.scrollbar_light_bg_clr          = 0xd9111111;
		
		window_manager.initialise(ui);
		
		@toolbar = Toolbar(ui, false, true);
		toolbar.name = 'ToolsToolbar';
		toolbar.x = 400;
		toolbar.y = 200;
		
		button_group.allow_reselect = true;
		button_group.select.on(EventCallback(on_tool_button_select));
		
		for(uint i = 0; i < tool_groups.length(); i++)
		{
			ToolGroup@ group = @tool_groups[i];
			group.init_ui();
			toolbar.add_child(group.button);
		}
		
		ui.add_child(toolbar);
		position_toolbar();
		
		// Info popup
		//{
		@info_label = Label(ui, '', true, font::SANS_BOLD, 20);
		info_label.scale_x = 0.75;
		info_label.scale_y = 0.75;
		
		@info_popup = PopupOptions(ui, info_label, false, PopupPosition::BelowLeft, PopupTriggerType::Manual, PopupHideType::Manual);
		info_popup.spacing = 0;
		info_popup.background_colour = multiply_alpha(ui.style.normal_bg_clr, 0.5);
		//}
		
		@on_after_layout_delegate = EventCallback(on_after_layout);
		ui.after_layout.on(on_after_layout_delegate);
		
		ui.screen_resize.on(EventCallback(on_screen_resize));
		
		update_shortcut_keys_enabled_popup();
	}
	
	private void initialise_ui_style()
	{
		ui.style.spacing = config.get_float('UISpacing', ui.style.spacing);
		
		if(config.has_value('UITextColour'))
			ui.style.auto_text_colour(config.get_colour('UITextColour'));
		if(config.has_value('UIBGColour'))
			ui.style.auto_base_colour(config.get_colour('UIBGColour'));
		if(config.has_value('UIBorderColour'))
			ui.style.auto_border_colour(config.get_colour('UIBorderColour'));
		if(config.has_value('UIAccentColour'))
			ui.style.auto_accent_colour(config.get_colour('UIAccentColour'));
	}
	
	private void initialise_tools()
	{
		for(uint i = 0; i < tool_groups.length(); i++)
		{
			tool_groups[i].on_init();
		}
	}
	
	private void create_tools()
	{
		// Built in
		
		add_tool(Tool(this, 'Select')			.set_icon('editor',  'selecticon').init_shortcut_key(VK::R));
		add_tool(ExtendedTileTool(this));
		add_tool(ExtendedPropTool(this));
		add_tool(Tool(this, 'Entities')			.set_icon('editor',  'entityicon').init_shortcut_key(VK::E));
		add_tool(ExtendedTriggerTool(this));
		add_tool(Tool(this, 'Camera')			.set_icon('editor',  'cameraicon').init_shortcut_key(VK::C));
		add_tool(EmitterTool(this));
		add_tool(Tool(this, 'Level Settings')	.set_icon('editor',  'settingsicon'));
		add_tool(Tool(this, 'Scripts')			.set_icon('dustmod', 'scripticon').init_shortcut_key(VK::S));
		add_tool(HelpTool(this, 'Help')			.set_icon('editor',  'helpicon'));
		
		@tools_map[''] = null;
		@tools_map['Wind'] = null;
		@tools_map['Particle'] = ParticleEditorTool(this);
		
		// Custom
		add_tool('Tiles',		EdgeBrushTool(this));
		add_tool('Tiles',		ShapeTool(this));
		add_tool('Props',		PropTool(this));
		add_tool('Props',		PropLineTool(this));
		
		sort_shortcut_tools();
	}
	
	private void sort_shortcut_tools()
	{
		array<ShortcutKeySorter> sort_list(tools_shortcut.length);
		for(uint i = 0; i < tools_shortcut.length; i++)
		{
			@sort_list[i].tool = tools_shortcut[i];
			sort_list[i].index = i;
		}
		
		dictionary shortcut_map;
		sort_list.sortAsc();
		for(uint i = 0; i < sort_list.length; i++)
		{
			Tool@ tool = sort_list[i].tool;
			const string map_key = tool.key.to_string();
			
			@tool.shortcut_key_group = shortcut_map.exists(map_key)
				? cast<Tool@>(shortcut_map[map_key]).shortcut_key_group
				: array<Tool@>();
			tool.shortcut_key_group.insertLast(tool);
			
			@tools_shortcut[i] = tool;
			@shortcut_map[map_key] = tool;
		}
	}
	
	// //////////////////////////////////////////////////////////
	// Main Methods
	// //////////////////////////////////////////////////////////
	
	void editor_step()
	{
		if(@editor == null)
			return;
		
		if(!initialised)
		{
			initialise();
			initialised = true;
		}
		
		screen_w = g.hud_screen_width(false);
		screen_h = g.hud_screen_height(false);
		
		view_x = cam.x();
		view_y = cam.y();
		
		cam.get_layer_draw_rect(0, 22, 22, view_x1, view_y1, view_w, view_h);
		view_x2 = view_x1 + view_w;
		view_y2 = view_y1 + view_h;
		
		zoom = cam.editor_zoom();
		
		ctrl.update(frame);
		shift.update(frame);
		alt.update(frame);
		space.update(frame);
		return_press = input.key_check_pressed_gvb(GVB::Return);
		escape_press = input.key_check_pressed_gvb(GVB::Escape);
		
		layer = editor.selected_layer;
		sub_layer = editor.selected_sub_layer;
		
		mouse_in_gui = editor.mouse_in_gui();
		mouse_in_scene = !mouse_in_gui && !ui.is_mouse_over_ui && !ui.is_mouse_active && !space.down;
		ui_focus = input.is_polling_keyboard();
		scene_focus = @ui.focus ==  null && !ui_focus;
		
		handle_keyboard();
		handles.step();
		mouse.step(space.down || !mouse_in_scene);
		
		const string new_selected_tab = editor.editor_tab();
		if(
			new_selected_tab != selected_tab && (
			new_selected_tab == '' || new_selected_tab == 'Help' ||
			new_selected_tab == 'Particle' || new_selected_tab == 'Wind'))
		{
			select_tool(new_selected_tab, false);
		}
		
		if(mouse.left_press)
		{
			space_on_press = space.down;
			pressed_in_scene = mouse_in_scene;
		}
		else if(mouse.left_release)
		{
			space_on_press = false;
			pressed_in_scene = false;
		}
		
		if(
			config.EnableShortcuts && @ui.focus == null && shortcut_keys_enabled && !ui_focus &&
			(@selected_tool == null || !selected_tool.active))
		{
			if(config.KeyPrevTool.check())
			{
				select_next_tool(-1);
			}
			else if(config.KeyNextTool.check())
			{
				select_next_tool(1);
			}
			else
			{
				//if(selected_tab == 'Particle')
				for(int i = num_tools_shortcut - 1; i >= 0; i--)
				{
					Tool@ tool = @tools_shortcut[i];
					
					if(!tool.key.matches(blocked_key) && tool.key.check())
					{
						select_tool(tool.on_shortcut_key());
						persist_state();
						break;
					}
				}
			}
			
			if(config.KeyToggleToolbars.check())
			{
				hide_toolbar_user = !hide_toolbar_user;
				ui.visible = !hide_toolbar_user;
			}
			if(config.KeyToggleUI.check())
			{
				hide_gui_user = !hide_gui_user;
				hide_gui_panels(hide_panels_gui);
				hide_gui_layers(hide_layers_gui);
			}
			if(config.KeyTogglePanels.check())
			{
				hide_panels_user = !hide_panels_user;
				hide_gui_panels(hide_panels_gui);
			}
			if(config.KeyPreviewLayer.check())
			{
				preview_layer();
			}
		}
		
		if(@selected_tool != null)
		{
			selected_tool.step();
		}
		
		// get_tool('Prop Tool').step();
		
		if(info_popup_timer > 0)
		{
			info_popup_timer = max(info_popup_timer - DT, 0.0);
			if(info_popup_timer <= 0)
			{
				hide_info_popup();
			}
		}
		
		info_overlay.step();
		
		ui.step();
		debug.step();
		
		if(toolbar.hovered || num_tool_group_popups > 0)
		{
			if(toolbar.alpha < 1)
			{
				toolbar.alpha = min(toolbar.alpha + Settings::UIFadeSpeed * DT, 1.0);
			}
		}
		else if(toolbar.alpha > Settings::UIFadeAlpha)
		{
			toolbar.alpha = max(toolbar.alpha - Settings::UIFadeSpeed * DT, Settings::UIFadeAlpha);
		}
		
		state_persisted = false;
		
		if(queue_load_config)
		{
			if(do_load_config())
			{
				do_full_reload();
			}
			
			queue_load_config = false;
		}
		
		frame++;
	}
	
	private void handle_keyboard()
	{
		if(input.key_check_pressed_vk(VK::Pause))
		{
			shortcut_keys_enabled = !shortcut_keys_enabled;
			update_shortcut_keys_enabled_popup();
		}
		
		if(!shortcut_keys_enabled)
			return;
		
		if(pressed_key != -1)
		{
			pressed_key_active = false;
			
			if(ui_focus || !input.key_check_gvb(pressed_key))
			{
				pressed_key = -1;
			}
			else
			{
				if(--pressed_timer == 0)
				{
					pressed_key_active = true;
					pressed_timer = Settings::KeyRepeatPeriod;
				}
				
				return;
			}
		}
		
		if(!ui_focus)
		{
			for(int i = int(Settings::RepeatKeys.length()) - 1; i >= 0; i--)
			{
				const int key = Settings::RepeatKeys[i];

				if(!input.key_check_pressed_gvb(key))
					continue;

				pressed_key = key;
				pressed_timer = Settings::KeyPressDelay;
				pressed_key_active = true;
				break;
			}
		}
	}
	
	private void preview_layer()
	{
		const int start_layer = editor.get_selected_layer() + 1;
		const bool visible = !editor.get_layer_visible(start_layer);
		
		for(int i = start_layer; i <= 20; i++)
		{
			editor.set_layer_visible(i, visible);
		}
	}
	
	void editor_draw(float sub_frame)
	{
		if(@selected_tool != null)
		{
			selected_tool.draw(sub_frame);
		}
		
		// get_tool('Prop Tool').draw(sub_frame);
		
		handles.draw();
		ui.draw();
		
		if(debug_ui)
		{
			ui.debug_draw();
		}
		
		debug.draw(sub_frame);
	}
	
	// //////////////////////////////////////////////////////////
	// Public Methods
	// //////////////////////////////////////////////////////////
	
	Tool@ get_tool(const string name)
	{
		if(!tools_map.exists(name))
			return null;
		
		return cast<Tool@>(tools_map[name]);
	}
	
	bool select_tool(const string &in name, const bool update_editor_tab=true)
	{
		if(!tools_map.exists(name))
			return false;
		
		select_tool(cast<Tool@>(tools_map[name]), update_editor_tab);
		return true;
	}
	
	void select_next_tool(const int dir=1)
	{
		if(@selected_tool == null)
		{
			select_tool(tools[0]);
			return;
		}
		
		ToolGroup@ group = selected_tool.group;
		Tool@ next_tool = selected_tool;
		
		do
		{
			if(@next_tool != null)
			{
				@next_tool = group.get_next_selectable_tool(next_tool, dir);
			}
			
			if(@next_tool != null)
				break;
			
			const int index = tool_groups.findByRef(group);
			
			if(index == -1)
			{
				@group = tool_groups[0];
			}
			else
			{
				@group = tool_groups[mod(index + (dir >= 0 ? 1 : -1), tool_groups.length())];
			}
			
			@next_tool = dir >= 1
				? group.get_first_selectable_tool()
				: group.get_last_selectable_tool();
		}
		while(@next_tool == null);
		
		select_tool(next_tool);
	}
	
	void track_tool_group_popups(const bool open)
	{
		num_tool_group_popups += open ? 1 : -1;
	}
	
	void init_icon(Image@ img)
	{
		if(@img == null)
			return;
		
		img.colour = config.UIIconColour;
		icon_images.insertLast(img);
	}
	
	void init_icon(Button@ button)
	{
		init_icon(button.icon);
	}
	
	void init_icon(MultiButton@ mbutton)
	{
		for(uint i = 0, count = mbutton.num_items; i < count; i++)
		{
			init_icon(mbutton.get_image(i));
		}
	}
	
	void world_to_hud(
		const int layer, const int sub_layer,
		const float x, const float y,
		float &out hud_x, float &out hud_y, const bool ui_coords=true)
	{
		float wx, wy;
		transform(x, y, layer, sub_layer, 19, 0, wx, wy);
		hud_x = (wx - view_x1) / view_w * screen_w;
		hud_y = (wy - view_y1) / view_h * screen_h;
		
		if(!ui_coords)
		{
			hud_x -= screen_w * 0.5;
			hud_y -= screen_h * 0.5;
		}
	}
	
	void world_to_hud(
		const int layer, const int sub_layer, entity@ e,
		float &out hud_x, float &out hud_y, const bool ui_coords=true)
	{
		world_to_hud(layer, sub_layer, e.x(), e.y(), hud_x, hud_y, ui_coords);
	}
	
	void world_to_hud(Mouse@ mouse, float &out hud_x, float &out hud_y, const bool ui_coords=true)
	{
		world_to_hud(mouse.layer, mouse.sub_layer, mouse.x, mouse.y, hud_x, hud_y, ui_coords);
	}
	
	int layer_position(const int layer_index)
	{
		return layer_positions[layer_index];
	}
	
	int layer_index(const int layer_position)
	{
		return layer_indices[layer_position];
	}
	
	float layer_scale(const uint layer, const uint sub_layer)
	{
		return layer_scales[layer][sub_layer];
	}
	
	float layer_scale(const int from_layer, const int from_sub_layer, const int to_layer, const int to_sub_layer)
	{
		return layer_scales[from_layer][from_sub_layer] / layer_scales[to_layer][to_sub_layer];
	}
	
	void select_layer(const int layer)
	{
		if(this.layer == layer)
			return;
		
		this.layer = layer;
		editor.selected_layer = layer;
	}
	
	void select_sub_layer(const int sub_layer)
	{
		if(this.sub_layer == sub_layer)
			return;
		
		this.sub_layer = sub_layer;
		editor.selected_sub_layer = sub_layer;
	}
	
	/// Change layer/sub layer with mouse wheel.
	bool scroll_layer(
		const bool allow_layer_update=true, const bool allow_sub_layer_update=true,
		const bool tiles_only=false, LayerInfoDisplay show_info=LayerInfoDisplay::Individual,
		IWorldBoundingBox@ target=null, Element@ target_element=null,
		const float time=0.5)
	{
		if(mouse.scroll == 0)
			return false;
		
		int updated = 0;
		int updated_layer = 0;
		
		if(allow_layer_update && ctrl.down)
		{
			const int prev_layer = this.layer;
			int layer = clamp(prev_layer + mouse.scroll,
				tiles_only ? 6 : 0, 20);
			
			if(tiles_only && layer == 18)
			{
				layer += mouse.scroll;
			}
			
			if(layer != prev_layer)
			{
				select_layer(layer);
				updated = 1;
				updated_layer = layer;
			}
		}
		else if(allow_sub_layer_update && alt.down)
		{
			const int prev_sub_layer = this.sub_layer;
			int sub_layer = clamp(prev_sub_layer + mouse.scroll, 0, 24);
			
			if(sub_layer != prev_sub_layer)
			{
				select_sub_layer(sub_layer);
				updated = 2;
				updated_layer = sub_layer;
			}
		}
		
		if(show_info != LayerInfoDisplay::None && updated != 0)
		{
			if(show_info == LayerInfoDisplay::Compound && (!allow_sub_layer_update || !allow_layer_update))
			{
				show_info = LayerInfoDisplay::Individual;
			}
			
			const string info = show_info == LayerInfoDisplay::Individual
				? (updated == 1 ? 'Layer: ' : 'Sub Layer: ') + updated_layer
				: 'Layer: ' + layer + '.' + sub_layer;
			
			if(@target_element != null)
			{
				info_overlay.show(target_element, info, time);
			}
			else if(@target != null)
			{
				info_overlay.show(target, info, time);
			}
			else
			{
				info_overlay.show(mouse, info, time);
			}
		}
		
		return updated != 0;
	}
	
	void transform(
		const float x, const float y,
		const int from_layer, const int from_sub_layer, const int to_layer, const int to_sub_layer,
		float &out out_x, float &out out_y)
	{
		const float scale = layer_scales[from_layer][from_sub_layer] / layer_scales[to_layer][to_sub_layer];
		
		const float dx = (x - view_x) * scale;
		const float dy = (y - view_y) * scale;
		
		out_x = view_x + dx;
		out_y = view_y + dy;
	}
	
	void transform(
		const float x, const float y,
		const int from_layer, const int from_sub_layer, Mouse@ mouse,
		float &out out_x, float &out out_y)
	{
		transform(x, y, from_layer, from_sub_layer, mouse.layer, mouse.sub_layer, out_x, out_y);
	}
	
	float transform_size(
		const float size,
		const int from_layer, const int from_sub_layer, const int to_layer, const int to_sub_layer)
	{
		return size * (layer_scales[from_layer][from_sub_layer] / layer_scales[to_layer][to_sub_layer]);
	}
	
	void transform_size(
		const float x, const float y,
		const int from_layer, const int from_sub_layer, const int to_layer, const int to_sub_layer,
		float &out out_x, float &out out_y)
	{
		const float scale = layer_scales[from_layer][from_sub_layer] / layer_scales[to_layer][to_sub_layer];
		out_x = x * scale;
		out_y = y * scale;
	}
	
	float transform_size(const float size, Mouse@ mouse, const int to_layer, const int to_sub_layer)
	{
		return size * (layer_scales[mouse.layer][mouse.sub_layer] / layer_scales[to_layer][to_sub_layer]);
	}
	
	void transform_size(
		const float x, const float y, Mouse@ mouse, const int to_layer, const int to_sub_layer,
		float &out out_x, float &out out_y)
	{
		const float scale = layer_scales[mouse.layer][mouse.sub_layer] / layer_scales[to_layer][to_sub_layer];
		out_x = x * scale;
		out_y = y * scale;
	}
	
	void mouse_layer(const int to_layer, const int to_sub_layer, float &out out_x, float &out out_y)
	{
		transform(mouse.x, mouse.y, mouse.layer, mouse.sub_layer, to_layer, to_sub_layer, out_x, out_y);
	}
	
	entity@ pick_trigger()
	{
		if(!mouse_in_scene)
			return null;
		
		int i = g.get_entity_collision(mouse.y - 10, mouse.y + 10, mouse.x - 10, mouse.x + 10, ColType::Trigger);
		
		entity@ closest = null;
		float closest_dist = 10 * 10;
		
		while(i-- > 0)
		{
			entity@ e = g.get_entity_collision_index(i);
			
			const float dist = dist_sqr(e.x(), e.y(), mouse.x, mouse.y);
			
			if(dist < closest_dist)
			{
				closest_dist = dist;
				@closest = e;
			}
		}
		
		return closest;
	}
	
	tileinfo@ pick_tile(int &out tile_x, int &out tile_y, int &out layer)
	{
		if(!mouse_in_scene)
			return null;
		
		for(layer = 20; layer >= 6; layer--)
		{
			if(!editor.get_layer_visible(layer))
				continue;
			
			float layer_x, layer_y;
			mouse_layer(layer, 10, layer_x, layer_y);
			tile_x = floor_int(layer_x / 48);
			tile_y = floor_int(layer_y / 48);
			tileinfo@ tile = g.get_tile(tile_x, tile_y, layer);
			
			if(!tile.solid())
				continue;
			
			float _;
			if(!point_in_tile(layer_x, layer_y, tile_x, tile_y, tile.type(), _, _))
				continue;

			return tile;
		}
		
		return null;
	}
	
	int query_onscreen_entities(const ColType type, const bool expand_for_parallax=false)
	{
		float x1, y1, x2, y2;
		transform(view_x1, view_y1, 22, 22, min_fg_layer, min_fg_sub_layer, x1, y1);
		transform(view_x2, view_y2, 22, 22, min_fg_layer, min_fg_sub_layer, x2, y2);
		
		if(!expand_for_parallax)
		{
			x1 = view_x1;
			y1 = view_y1;
			x2 = view_x2;
			y2 = view_y2;
		}
		else
		{
			transform(view_x1, view_y1, 22, 22, min_fg_layer, min_fg_sub_layer, x1, y1);
			transform(view_x2, view_y2, 22, 22, min_fg_layer, min_fg_sub_layer, x2, y2);
		}
		
		const float padding = 100;
		return g.get_entity_collision(y1 - padding, y2 + padding, x1 - padding, x2 + padding, type);
	}
	
	void draw_select_rect(const float x1, const float y1, const float x2, const float y2)
	{
		g.draw_rectangle_world(
			22, 22,
			x1, y1, x2, y2,
			0, Settings::SelectRectFillColour);
		
		outline_rect(g, 22, 22,
			x1, y1, x2, y2,
			Settings::SelectRectLineWidth / zoom, Settings::SelectRectLineColour);
	}
	
	void snap(const float x, const float y, float &out out_x, float &out out_y, const float custom_snap_size=5,
		const bool default_shift=false)
	{
		const float snap = get_snap_size(custom_snap_size, default_shift);
		
		if(snap != 0)
		{
			out_x = round(x / snap) * snap;
			out_y = round(y / snap) * snap;
		}
		else
		{
			out_x = x;
			out_y = y;
		}
	}
	
	void snap(const float angle, float &out out_angle, const bool alt_small=true)
	{
		const float snap = get_snap_angle(alt_small) * DEG2RAD;
		
		if(snap != 0)
		{
			out_angle = round(angle / snap) * snap;
		}
		else
		{
			out_angle = angle;
		}
	}
	
	float get_snap_size(const float custom_snap_size=5, const bool default_shift=false)
	{
		if(shift.down || default_shift && !ctrl.down && !alt.down)
			return 48;
		
		if(ctrl.down)
			return 24;
		
		if(alt.down)
			return custom_snap_size;
		
		return 0;
	}
	
	float get_snap_angle(const bool alt_small=true)
	{
		if(!alt_small && shift.down && ctrl.down)
			return 5;
		
		if(shift.down)
			return 45;
		
		if(ctrl.down)
			return 22.5;
		
		if(alt_small && alt.down)
			return 5;
		
		return 0;
	}
	
	bool circle_segments(const float radius, const uint segments, uint &out out_segments,
		const float threshold=0, const bool world=true)
	{
		// Dynamically adjust the number of segments used to draw the circle
		// based on how big it is on screen
		const float view_height = 1080 / (world ? zoom : 1);
		float t = clamp01((radius * 2) / (view_height * 0.5));
		// Adjust the curve between high and low detail so the transition
		// to low detail doesn't happen too soon.
		// Easing out cubic
		t--;
		t = t * t * t + 1;
		
		// Don't draw circles that get too small
		if(segments * t < threshold)
		{
			out_segments = 0;
			return false;
		}
		
		out_segments = int(min(float(segments), 5 + segments * t));
		return true;
	}
	
	void circle(
		const uint layer, const uint sub_layer,
		const float x, const float y, const float radius, uint segments,
		const float thickness=4, const uint colour=0xFFFFFFFF,
		const float threshold=0, const bool world=true)
	{
		if(!circle_segments(radius, segments, segments, threshold, world))
			return;
		
		drawing::circle(
			g, layer, sub_layer,
			x, y, radius, segments,
			thickness / zoom, colour, world);
	}
	
	void fill_circle(
		const uint layer, const uint sub_layer,
		const float x, const float y, const float radius, uint segments,
		const uint inner_colour, const uint outer_colour,
		const float threshold=0, const bool world=true)
	{
		if(!circle_segments(radius, segments, segments, threshold, world))
			return;
		
		drawing::fill_circle(
			g, layer, sub_layer,
			x, y, radius / zoom, segments,
			inner_colour, outer_colour, world);
	}
	
	bool is_same_layer_scale(const int layer1, const int sub_layer1, const int layer2, const int sub_layer2)
	{
		return layer_scales[layer1][sub_layer1] != layer_scales[layer2][sub_layer2];
	}
	
	/**
	 * Shows a temporary popup below the toolbar.
	 */
	void show_info(const string info, const float time=0.75, const PopupPosition position=PopupPosition::Below)
	{
		info_overlay.show(toolbar, info, time);
		info_overlay.position = position;
	}
	
	void show_layer_sub_layer_overlay(const float x1, const float y1, const float x2, const float y2,
		const int layer, const int sub_layer)
	{
		info_overlay.show(x1, y1, x2, y2, layer + '.' + sub_layer, 0.75);
	}
	
	void show_layer_sub_layer_overlay(IWorldBoundingBox@ target, const int layer, const int sub_layer)
	{
		info_overlay.show(target, layer + '.' + sub_layer, 0.75);
	}
	
	void show_info_popup(
		const string &in info, Toolbar@ toolbar=null, const PopupPosition position=PopupPosition::BelowLeft,
		const float time=0)
	{
		info_label.text = info;
		info_popup.position = position;
		ui.show_tooltip(info_popup, @toolbar != null ? toolbar : this.toolbar);
		info_popup_timer = time;
	}
	
	void hide_info_popup()
	{
		ui.hide_tooltip(info_popup);
		info_popup_timer = 0;
	}
	
	bool key_repeat_gvb(const int gvb)
	{
		return pressed_key == gvb && pressed_key_active;
	}
	
	void world_to_local(
		const float x, const float y, const int layer, const int sub_layer,
		const float to_x, const float to_y, const float to_rotation,
		const int to_layer, const int to_sub_layer,
		float &out local_x, float &out local_y)
	{
		transform(x, y, layer, sub_layer, to_layer, to_sub_layer, local_x, local_y);
		local_x -= to_x;
		local_y -= to_y;
		rotate(local_x, local_y, -to_rotation * DEG2RAD, local_x, local_y);
	}
	
	void world_to_local(
		Mouse@ mouse,
		const float to_x, const float to_y, const float to_rotation,
		const int to_layer, const int to_sub_layer,
		float &out local_x, float &out local_y)
	{
		world_to_local(mouse.x, mouse.y, mouse.layer, mouse.sub_layer, to_x, to_y, to_rotation, to_layer, to_sub_layer, local_x, local_y);
	}
	
	void store_layer_values()
	{
		min_layer_scale = INFINITY;
		min_fg_layer_scale = INFINITY;
		
		for(uint i = 0; i <= 22; i++)
		{
			layer_positions[i] = g.get_layer_position(i);
			layer_indices[layer_positions[i]] = i;
			
			array<float>@ sub_layer_scales = layer_scales[i];
			for(uint j = 0; j <= 24; j++)
			{
				const float scale = g.sub_layer_scale(i, j);
				sub_layer_scales[j] = scale;
				
				if(scale < min_layer_scale)
				{
					if(i >= 6)
					{
						min_fg_layer_scale = scale;
						min_fg_layer = i;
						min_fg_sub_layer = j;
					}
					
					min_layer_scale = scale;
					min_layer = i;
					min_sub_layer = j;
				}
			}
		}
	}
	
	void hide_gui_panels(const bool hidden)
	{
		hide_panels_gui = hidden;
		editor.hide_panels_gui(hide_panels_gui ||  hide_panels_user || hide_gui_user);
	}
	
	void hide_gui_layers(const bool hidden)
	{
		hide_layers_gui = hidden;
		editor.hide_layers_gui(hide_layers_gui || hide_gui_user);
	}
	
	/// Returns true if the given global virtual button is down and then clears it
	bool consume_gvb(const int gvb)
	{
		if(input.key_check_gvb(gvb))
		{
			input.key_clear_gvb(gvb);
			return true;
		}
		
		return false;
	}
	
	/// Returns true if the given global virtual button is pressed and then clears it
	bool consume_pressed_gvb(const int gvb)
	{
		if(input.key_check_pressed_gvb(gvb))
		{
			if(return_press && gvb == GVB::Return)
			{
				return_press = false;
			}
			else if(escape_press && gvb == GVB::Escape)
			{
				escape_press = false;
			}
			
			input.key_clear_gvb(gvb);
			return true;
		}
		
		return false;
	}
	
	/**
	 * @brief Convenience method for creating a label.
	 * @param parent If provided the label is added to this parent.
	 * @return The created label.
	 */
	Label@ create_label(const string &in text, Container@ parent=null)
	{
		Label@ label = Label(ui, text);
		label.align_v = GraphicAlign::Middle;
		label.fit_to_contents();
		
		if(@parent != null)
		{
			parent.add_child(label);
		}
		
		return label;
	}
	
	/**
	 * @brief Convenience method to create a button with a name, icon, and tooltip.
	 * @param name Required. The name used to identify this button.
	 * @param icon Required. Assumes it's part of the script/plugin sprite set, and of the default `IconSize`.
	 * @param tooltip If not empty adds a tooltip.
	 * @return 
	 */
	Button@ create_toolbar_button(Toolbar@ toolbar, const string &in name, const string &in icon, const string &in tooltip='')
	{
		Button@ button = toolbar.create_button(SPRITE_SET, icon, Settings::IconSize, Settings::IconSize);
		button.name = name;
		button.fit_to_contents(true);
		
		if(tooltip != '')
		{
			@button.tooltip = PopupOptions(ui, tooltip);
		}
		
		init_icon(button);
		return button;
	}
	
	// //////////////////////////////////////////////////////////
	// Private Methods
	// //////////////////////////////////////////////////////////
	
	/// Add a tool and create a group with the same name
	private void add_tool(Tool@ tool)
	{
		add_tool(tool.name, tool);
	}
	
	/// Add a tool to the group with the specified name, creating one if it does not exist.
	private void add_tool(const string group_name, Tool@ tool)
	{
		ToolGroup@ group;
		
		if(!tool_groups_map.exists(group_name))
		{
			tool_groups.resize(tool_groups.length() + 1);
			@group = @tool_groups[tool_groups.length() - 1];
			group.create(this, group_name, button_group);
			@tool_groups_map[group_name] = group;
		}
		else
		{
			@group = cast<ToolGroup@>(tool_groups_map[group_name]);
		}
		
		add_tool(group, tool);
	}
	
	/// Add a tool to an existing tool group
	private void add_tool(ToolGroup@ group, Tool@ tool)
	{
		if(@tool == null)
			return;
		
		if(tools_map.exists(tool.name))
		{
			puts('Tool "' + tool.name + '" already exists.');
			return;
		}
		
		@tools_map[tool.name] = tool;
		tools.insertLast(tool);
		
		if(@group != null)
		{
			group.add_tool(tool);
		}
		
		if(tool.key.is_set())
		{
			tools_shortcut.insertLast(@tool);
			num_tools_shortcut++;
		}
	}
	
	/// Select the specified tool and call the relevant callbacks.
	private void select_tool(Tool@ tool, const bool update_editor_tab=true, const bool allow_reselected=true)
	{
		// Don't trigger a reselect if this is the firs t tool to be selected ('_')
		if(@tool == @selected_tool && selected_tab != '_')
		{
			if(@selected_tool != null)
			{
				selected_tool.on_reselect();
			}
			
			do_update_editor_tab(update_editor_tab);
			on_tool_changed(allow_reselected);
			return;
		}
		
		if(@tool != null && !tool.on_before_select())
		{
			if(@selected_tool.group != null)
			{
				selected_tool.group.on_select();
			}
			
			return;
		}
		
		ignore_toolbar_select_event = true;
		
		// Don't allow the user to deselect, but allow select_tool(null) to deselect tool buttons
		button_group.allow_deselect = true;
		
		if(@selected_tool != null)
		{
			selected_tool.on_deselect();
			
			if(@selected_tool.group != null)
			{
				selected_tool.group.on_deselect();
			}
		}
		
		store_layer_values();
		
		@selected_tool = tool;
		selected_tool_name = @selected_tool != null ? selected_tool.name : '';
		do_update_editor_tab(update_editor_tab);
		
		if(@selected_tool != null)
		{
			selected_tool.on_select();
			
			if(@selected_tool.group != null)
			{
				selected_tool.group.on_select();
			}
		}
		
		ignore_toolbar_select_event = false;
		button_group.allow_deselect = false;
		
		selected_tab = selected_tool_name == ''
			? editor.editor_tab() : selected_tool_name;
		ui.mouse_enabled = true;
		on_tool_changed(false);
	}
	
	private void do_update_editor_tab(const bool do_update)
	{
		if(!do_update || @selected_tool == null)
			return;
		
		editor.editor_tab(selected_tool.name);
		
		if(editor.editor_tab() != selected_tool.name)
		{
			editor.editor_tab('Scripts');
		}
	}
	
	private void position_toolbar()
	{
		toolbar.x = (ui.region_width - toolbar.width) * 0.5;
		toolbar.y = 0;
	}
	
	private void update_shortcut_keys_enabled_popup()
	{
		if(shortcut_keys_enabled && @shortcut_keys_enabled_popup == null)
			return;
		
		if(@shortcut_keys_enabled_popup == null)
		{
			@shortcut_keys_enabled_popup = PopupOptions(ui, '', false, PopupPosition::Below, PopupTriggerType::Manual, PopupHideType::Manual);
		}
		
		if(!shortcut_keys_enabled)
		{
			shortcut_keys_enabled_popup.content_string = shortcut_keys_enabled ? 'Shortcut Keys Enabled' : 'Shortcut Keys Disabled';
			ui.show_tooltip(shortcut_keys_enabled_popup, toolbar);
		}
		else
		{
			ui.hide_tooltip(shortcut_keys_enabled_popup);
		}
	}
	
	void persist_state()
	{
		if(state_persisted)
			return;
		
		//controllable@ p = controller_controllable(0);
		//
		//if(@p != null)
		//{
		//	p.x(g.get_checkpoint_x(0));
		//	p.y(g.get_checkpoint_y(0));
		//}
		//
		//g.save_checkpoint(0, 0);
		state_persisted = true;
	}
	
	// ///////////////////////////////////////////
	// Events
	// ///////////////////////////////////////////
	
	private void on_tool_changed(const bool reselected)
	{
		previous_selected_tab = selected_tab;
	}
	
	private void on_clipboard_change(EventInfo@ event)
	{
		clipboard = event.value;
	}
	
	private void on_tool_button_select(EventInfo@ event)
	{
		if(ignore_toolbar_select_event)
			return;
		
		if(@event.target == null)
			return;
		
		select_tool(cast<Tool@>(tools_map[event.target.name]));
		persist_state();
	}
	
	//
	
	private void on_after_layout(EventInfo@ event)
	{
		position_toolbar();
		ui.after_layout.off(on_after_layout_delegate);
	}
	
	private void on_screen_resize(EventInfo@ event)
	{
		position_toolbar();
	}
	
}
