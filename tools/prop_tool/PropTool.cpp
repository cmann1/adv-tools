#include '../../misc/SelectAction.cpp';
#include '../../misc/AlignmentEdge.cpp';
#include '../../../../lib/tiles/common.cpp';
#include '../../../../lib/props/common.cpp';
#include '../../../../lib/props/data.cpp';
#include '../../../../lib/props/outlines.cpp';

#include 'PropToolSettings.cpp';
#include 'PropToolState.cpp';
#include 'PropSortingData.cpp';
#include 'PropAlignData.cpp';
#include 'PropData.cpp';
#include 'PropToolToolbar.cpp';
#include 'PropsClipboardData.cpp';
#include 'PropExportType.cpp';
#include 'PropToolExporter.cpp';

const string PROP_TOOL_SPRITES_BASE = SPRITES_BASE + 'prop_tool/';
const string EMBED_spr_icon_prop_tool = SPRITES_BASE + 'icon_prop_tool.png';

class PropTool : Tool
{
	
	private PropToolState state = Idle;
	private bool performing_action;
	
	private Mouse@ mouse;
	private bool mouse_press_moved;
	private bool mouse_press_modified;
	
	private int prop_data_pool_size = 32;
	private int prop_data_pool_count;
	private array<PropData@> prop_data_pool(prop_data_pool_size);
	
	private int props_under_mouse_size = 32;
	private array<PropSortingData> props_under_mouse(props_under_mouse_size);
	
	private int props_align_data_size = 32;
	private array<PropAlignData> props_align_data(props_align_data_size);
	
	private int highlighted_props_size = 32;
	private int highlighted_props_count;
	private array<PropData@> highlighted_props(highlighted_props_size);
	private dictionary highlighted_props_map;
	
	private PropData@ previous_hovered_prop;
	private PropData@ hovered_prop;
	private PropData@ pressed_prop;
	private int selected_props_size = 32;
	private int selected_props_count;
	private array<PropData@> selected_props(selected_props_size);
	private bool temporary_selection;
	
	private int hover_index_offset;
	
	private float drag_start_x, drag_start_y;
	private float drag_offset_angle;
	private bool drag_rotation_handle;
	private float drag_scale_start_distance;
	private float drag_selection_x1, drag_selection_y1;
	private float drag_selection_x2, drag_selection_y2;
	
	private int select_rect_pending;
	private int action_layer;
	private int action_sub_layer;
	
	private int selection_layer;
	private int selection_sub_layer;
	private float selection_x, selection_y;
	private float selection_x1, selection_y1;
	private float selection_x2, selection_y2;
	private float selection_angle;
	private float selection_drag_start_x, selection_drag_start_y;
	private float selection_drag_start_angle;
	private WorldBoundingBox selection_bounding_box;
	
	float origin_align_x, origin_align_y;
	
	dictionary locked_props;
	int num_locked_props;
	array<bool> locked_sublayers(23 * 25);
	
	private bool has_custom_anchor;
	private int custom_anchor_layer = 19;
	private int custom_anchor_sub_layer = 0;
	private float custom_anchor_x, custom_anchor_y;
	private float custom_anchor_offset_x, custom_anchor_offset_y;
	
	private PropToolToolbar toolbar;
	private int selected_props_info = -1;
	
	// Settings
	
	bool pick_through_tiles = false;
	float custom_grid = 5;
	string default_origin = 'centre';
	bool custom_anchor_lock = false;
	PropToolHighlight highlight_selection = Both;
	bool show_selection = true;
	bool show_info = true;
	PropToolExporter exporter;
	
	bool pick_ignore_holes = true;
	float pick_radius = 2;
	
	array<PropData@>@ highlighted_props_list { get { return @highlighted_props; } }
	int highlighted_props_list_count { get { return highlighted_props_count; } }
	
	PropTool(AdvToolScript@ script)
	{
		super(script, 'Props', 'Prop Tool');
		
		init_shortcut_key(VK::Q);
		
		exporter.init(script);
	}
	
	void build_sprites(message@ msg) override
	{
		build_sprite(msg, 'icon_prop_tool');
		
		toolbar.build_sprites(msg);
	}
	
	void create(ToolGroup@ group) override
	{
		set_icon(SPRITE_SET, 'icon_prop_tool');
	}
	
	void on_init() override
	{
		@mouse = @script.mouse;
		@selection_bounding_box.script = script;
		
		update_alignments_from_origin();
	}
	
	// //////////////////////////////////////////////////////////
	// Callbacks
	// //////////////////////////////////////////////////////////
	
	protected void on_editor_unloaded_impl() override
	{
		select_none();
		clear_highlighted_props(true);
		clear_temporary_selection();
		state = PropToolState::Idle;
	}
	
	protected void on_select_impl()
	{
		script.hide_gui_panels(true);
		
		toolbar.show(script, this);
		
		reset();
	}
	
	protected void on_deselect_impl()
	{
		script.hide_gui_panels(false);
		script.hide_info_popup();
		
		reset();
		toolbar.hide();
	}
	
	protected void step_impl() override
	{
		// Reset hover index offset when the mouse moves
		
		if(mouse.delta_x != 0 || mouse.delta_y != 0)
		{
			hover_index_offset = 0;
			mouse_press_moved = true;
		}
		
		if(mouse.left_press)
		{
			mouse_press_moved = false;
		}
		
		switch(state)
		{
			case PropToolState::Idle: state_idle(); break;
			case PropToolState::Moving: state_moving(); break;
			case PropToolState::Rotating: state_rotating(); break;
			case PropToolState::Scaling: state_scaling(); break;
			case PropToolState::Selecting: state_selecting(); break;
		}
		
		for(int i = highlighted_props_count - 1; i >= 0; i--)
		{
			highlighted_props[i].step();
		}
		
		if((@hovered_prop != null || selected_props_count == 1) && show_info)
		{
			if(@hovered_prop != @previous_hovered_prop)
			{
				toolbar.show_prop_info(@hovered_prop != null ? hovered_prop : selected_props[0]);
				@previous_hovered_prop = hovered_prop;
			}
			
			selected_props_info = -1;
		}
		else if(selected_props_count > 0 && show_info)
		{
			// Only update once when count changes otherwise this popup
			// will always be on top of other popups
			if(selected_props_info != selected_props_count)
			{
				toolbar.show_info(
					selected_props_count + ' prop' + (selected_props_count != 1 ? 's' : '') + ' selected');
				selected_props_info = selected_props_count;
			}
			
			@previous_hovered_prop = null;
		}
		else
		{
			script.hide_info_popup();
			@previous_hovered_prop = null;
			selected_props_info = -1;
		}
		
		performing_action = state != Idle && state != Selecting;
	}
	
	protected void draw_impl(const float sub_frame) override
	{
		const bool highlight = !performing_action || show_selection;
		
		if(highlight)
		{
			// Highlights
			for(int i = 0; i < highlighted_props_count; i++)
			{
				PropData@ data = @highlighted_props[i];
				if(data.pending_selection == -2)
					continue;
				
				data.draw(highlight_selection);
			}
			
			// Bounding box
			
			if(selected_props_count > 0 && !temporary_selection)
			{
				draw_selection_bounding_box();
			}
		}
		
		// Anchor points
		
		if(highlight && !has_custom_anchor && selected_props_count > 0)
		{
			draw_rotation_anchor(selection_x, selection_y, selection_layer, selection_sub_layer);
		}
		
		if(highlight && !has_custom_anchor && @hovered_prop != null && !hovered_prop.selected)
		{
			draw_rotation_anchor(hovered_prop.anchor_x, hovered_prop.anchor_y, hovered_prop.layer, hovered_prop.sub_layer);
		}
		
		if(has_custom_anchor)
		{
			draw_rotation_anchor(custom_anchor_x, custom_anchor_y, custom_anchor_layer, custom_anchor_sub_layer, true);
			
			if(selected_props_count > 0 && !script.is_same_layer_scale(custom_anchor_layer, custom_anchor_sub_layer, selection_layer, selection_sub_layer))
			{
				const uint clr = multiply_alpha(Settings::BoundingBoxColour, 0.5);
				float x1, y1, x2, y2;
				
				script.transform(custom_anchor_x, custom_anchor_y, custom_anchor_layer, custom_anchor_sub_layer, 22, 22, x1, y1);
				script.transform(custom_anchor_x, custom_anchor_y, selection_layer, selection_sub_layer, 22, 22, x2, y2);
				
				script.g.draw_line_world(22, 22, x1, y1, x2, y2, 1 / script.zoom, clr);
				draw_rotation_anchor(custom_anchor_x, custom_anchor_y, selection_layer, selection_sub_layer, true, 1, clr);
			}
		}
		
		// Selection rect
		
		if(state == Selecting)
		{
			script.draw_select_rect(drag_start_x, drag_start_y, mouse.x, mouse.y);
		}
	}
	
	private void draw_selection_bounding_box()
	{
		float sx, sy, sx1, sy1, sx2, sy2;
		float x1, y1, x2, y2, x3, y3, x4, y4;
		get_selection_rect(sx, sy, sx1, sy1, sx2, sy2);
		
		rotate(sx1, sy1, selection_angle, x1, y1);
		rotate(sx2, sy1, selection_angle, x2, y2);
		rotate(sx2, sy2, selection_angle, x3, y3);
		rotate(sx1, sy2, selection_angle, x4, y4);
		
		x1 += sx;
		y1 += sy;
		x2 += sx;
		y2 += sy;
		x3 += sx;
		y3 += sy;
		x4 += sx;
		y4 += sy;
		
		const float thickness = Settings::BoundingBoxLineWidth / script.zoom;
		
		script.g.draw_line_world(22, 22, x1, y1, x2, y2, thickness, Settings::BoundingBoxColour);
		script.g.draw_line_world(22, 22, x2, y2, x3, y3, thickness, Settings::BoundingBoxColour);
		script.g.draw_line_world(22, 22, x3, y3, x4, y4, thickness, Settings::BoundingBoxColour);
		script.g.draw_line_world(22, 22, x4, y4, x1, y1, thickness, Settings::BoundingBoxColour);
		const float mx = (x1 + x2) * 0.5;
		const float my = (y1 + y2) * 0.5;
		
		const float nx = cos(selection_angle - HALF_PI);
		const float ny = sin(selection_angle - HALF_PI);
		const float oh = (Settings::RotationHandleOffset - Settings::RotateHandleSize) / script.zoom;
		
		script.g.draw_line_world(22, 22,
			mx, my,
			mx + nx * oh, my + ny * oh,
			Settings::BoundingBoxLineWidth / script.zoom, Settings::BoundingBoxColour);
	}
	
	private void draw_rotation_anchor(
	
		float x, float y, const int from_layer, const int from_sub_layer,
		const bool lock=false, const float size = 1.4, const uint clr=Settings::BoundingBoxColour)
	{
		script.transform(x, y, from_layer, from_sub_layer, 22, 2, x, y);
		
		const float length = (!lock ? 5 : 8) * size / script.zoom;
		
		script.g.draw_rectangle_world(22, 22,
			x - length, y - 1 / script.zoom,
			x + length, y + 1 / script.zoom, 0, clr);
		script.g.draw_rectangle_world(22, 22,
			x - length, y - 1 / script.zoom,
			x + length, y + 1 / script.zoom, 90, clr);
		
		if(lock)
		{
			drawing::circle(script.g, 22, 22, x, y, 4 * size / script.zoom, 12, 2 / script.zoom, clr);
		}
	}
	
	// //////////////////////////////////////////////////////////
	// States
	// //////////////////////////////////////////////////////////
	
	private void state_idle()
	{
		clear_hovered_props();
		
		const bool start_rotating = check_rotation_handle();
		const bool start_scaling  = check_scale_handle();
		
		// Start rotating from handle
		
		if(start_rotating)
		{
			drag_rotation_handle = true;
			idle_start_rotating();
			return;
		}
		
		// Start scaling from handle
		
		if(start_scaling)
		{
			idle_start_scaling();
			return;
		}
		
		// Start dragging selection
		
		if(script.mouse_in_scene && script.alt.down && mouse.left_press)
		{
			if(!script.shift.down && !script.ctrl.down)
			{
				select_none();
			}
			
			select_rect_pending = script.shift.down ? 1 : script.ctrl.down ? -1 : 0;
			
			drag_start_x = mouse.x;
			drag_start_y = mouse.y;
			clear_highlighted_props();
			state = Selecting;
			script.ui.mouse_enabled = false;
			return;
		}
		
		// Start moving
		
		if(@pressed_prop != null && (mouse.delta_x != 0 || mouse.delta_y != 0))
		{
			idle_start_move();
			return;
		}
		
		// Pick props
		if(script.mouse_in_scene && !script.space.down && !script.handles.mouse_over)
		{
			pick_props();
			do_mouse_selection();
		}
		else
		{
			@pressed_prop = null;
		}
		
		// Keyboard shortcuts
		if(script.scene_focus)
		{
			// Lock
			if(script.input.key_check_pressed_vk(VK::L))
			{
				if(script.ctrl.down)
				{
					if(script.alt.down)
						unlock_all_sublayers();
					else
						lock_selected_sublayers(script.shift.down);
				}
				else
				{
					if(script.alt.down)
						unlock_all();
					else
						lock_selected();
				}
			}
			
			// Delete
			if(script.input.key_check_gvb(GVB::Delete))
			{
				delete_selected();
			}
			
			// Move
			if(script.key_repeat_gvb(GVB::LeftArrow))
			{
				shift_props(script.ctrl.down ? -20 : script.shift.down ? -10 : -1, 0);
			}
			if(script.key_repeat_gvb(GVB::RightArrow))
			{
				shift_props(script.ctrl.down ? 20 : script.shift.down ? 10 : 1, 0);
			}
			if(script.key_repeat_gvb(GVB::UpArrow))
			{
				shift_props(0, script.ctrl.down ? -20 : script.shift.down ? -10 : -1);
			}
			if(script.key_repeat_gvb(GVB::DownArrow))
			{
				shift_props(0, script.ctrl.down ? 20 : script.shift.down ? 10 : 1);
			}
			
			// Flip/mirror
			if(script.input.key_check_pressed_gvb(GVB::BracketOpen))
			{
				if(script.shift.down)
					flip_props(true, false);
				else
					mirror_selected(true);
				
				try_update_info();
			}
			if(script.input.key_check_pressed_gvb(GVB::BracketClose))
			{
				if(script.shift.down)
					flip_props(false, true);
				else
					mirror_selected(false);
				
				try_update_info();
			}
			
			// Cycle palette
			if(
				script.input.key_check_pressed_vk(VK::PageUp) ||
				script.input.key_check_pressed_vk(VK::PageDown))
			{
				cycle_selected_palettes(script.input.key_check_pressed_vk(VK::PageUp) ? -1 : 1);
			}
			
			// Copy/Cut/Paste
			if(
				selected_props_count > 0 && script.ctrl.down &&
				(script.input.key_check_pressed_vk(VK::C) || script.input.key_check_pressed_vk(VK::X)))
			{
				copy_selected_props();
				if(script.input.key_check_pressed_vk(VK::X))
				{
					delete_selected();
				}
			}
			if(script.ctrl.down && script.input.key_check_pressed_vk(VK::V))
			{
				paste(script.shift, script.alt);
			}
		}
		
		// Start rotating from hovered prop
		if(script.mouse_in_scene && @hovered_prop != null && !script.shift.down && !script.alt.down && mouse.middle_press)
		{
			drag_rotation_handle = false;
			idle_start_rotating();
			return;
		}
		
		// Set or clear custom anchor position, or set custom anchor layer
		if(
			script.mouse_in_scene && script.shift.down && !script.ctrl.down && !script.alt.down &&
			mouse.scroll != 0 && has_custom_anchor)
		{
			adjust_custom_anchor_layer(mouse.scroll);
			show_custom_anchor_info();
		}
		if(script.mouse_in_scene && mouse.middle_press)
		{
			if(script.shift.down || script.ctrl.down)
			{
				if(!has_custom_anchor || script.ctrl.down)
				{
					if(selected_props_count > 0)
					{
						custom_anchor_layer = selection_layer;
						custom_anchor_sub_layer = selection_sub_layer;
					}
					else if(@hovered_prop != null)
					{
						custom_anchor_layer = hovered_prop.layer;
						custom_anchor_sub_layer = hovered_prop.sub_layer;
					}
				}
				
				if(!script.ctrl.down)
				{
					script.mouse_layer(custom_anchor_layer, custom_anchor_sub_layer, custom_anchor_x, custom_anchor_y);
				}
				else
				{
					custom_anchor_x = selection_x;
					custom_anchor_y = selection_y;
				}
				
				has_custom_anchor = true;
				toolbar.update_buttons(selected_props_count);
			}
			else if(script.alt.down)
			{
				has_custom_anchor = false;
				toolbar.update_buttons(selected_props_count);
			}
		}
		
		// Scroll hover index offset
		if(mouse.scroll != 0 && !script.space.down && !script.ctrl.down && !script.alt.down && !script.shift.down)
		{
			hover_index_offset -= mouse.scroll;
		}
		
		// Adjust layer/sublayer
		if(mouse.scroll != 0 && (script.ctrl.down || script.alt.down))
		{
			idle_adjust_layer();
		}
		
		// Delete
		if(script.mouse_in_scene && @hovered_prop != null && (mouse.right_press || script.shift.down && mouse.right_down))
		{
			if(hovered_prop.selected)
			{
				select_prop(hovered_prop, SelectAction::Remove);
			}
			
			script.g.remove_prop(hovered_prop.prop);
			hovered_prop.hovered = false;
			@hovered_prop = null;
		}
		
		clear_highlighted_props();
	}
	
	private void idle_start_move()
	{
		if(!pressed_prop.selected)
		{
			select_prop(pressed_prop, SelectAction::Set);
			clear_highlighted_props();
			temporary_selection = true;
		}
		
		if(selected_props_count == 0)
		{
			selection_layer = pressed_prop.layer;
			selection_sub_layer = pressed_prop.sub_layer;
		}
		
		action_layer = pressed_prop.layer;
		action_sub_layer = pressed_prop.sub_layer;
		
		drag_start_x = mouse.x;
		drag_start_y = mouse.y;
		script.transform(drag_start_x, drag_start_y, 22, 22, action_layer, action_sub_layer, drag_start_x, drag_start_y);
		selection_drag_start_x = selection_x;
		selection_drag_start_y = selection_y;
		
		for(int i = 0; i < selected_props_count; i++)
		{
			selected_props[i].start_drag();
		}
		
		pressed_prop.hovered = true;
		@hovered_prop = pressed_prop;
		@pressed_prop = null;
		state = Moving;
		script.ui.mouse_enabled = false;
	}
	
	private void idle_start_rotating()
	{
		if(!drag_rotation_handle && !hovered_prop.selected)
		{
			select_prop(hovered_prop, SelectAction::Set);
			clear_highlighted_props();
			temporary_selection = true;
		}
		
		if(selected_props_count == 1 && temporary_selection)
		{
			selection_layer = hovered_prop.layer;
			selection_sub_layer = hovered_prop.sub_layer;
			selection_angle = hovered_prop.prop.rotation() * DEG2RAD;
		}
		
		selection_drag_start_angle = selection_angle;
		
		const float anchor_x = has_custom_anchor ? custom_anchor_x : selection_x;
		const float anchor_y = has_custom_anchor ? custom_anchor_y : selection_y;
		
		for(int i = 0; i < selected_props_count; i++)
		{
			selected_props[i].start_rotate(anchor_x, anchor_y, selection_angle * RAD2DEG);
		}
		
		float x, y;
		script.mouse_layer(selection_layer, selection_sub_layer, x, y);
		drag_offset_angle = atan2(anchor_y - y, anchor_x - x) - selection_angle;
		
		if(has_custom_anchor)
		{
			custom_anchor_offset_x = selection_x - custom_anchor_x;
			custom_anchor_offset_y = selection_y - custom_anchor_y;
		}
		
		clear_highlighted_props();
		state = Rotating;
		script.ui.mouse_enabled = false;
	}
	
	private void idle_start_scaling()
	{
		const float anchor_x = has_custom_anchor ? custom_anchor_x : selection_x;
		const float anchor_y = has_custom_anchor ? custom_anchor_y : selection_y;
		
		for(int i = 0; i < selected_props_count; i++)
		{
			selected_props[i].start_scale(anchor_x, anchor_y);
		}
		
		drag_selection_x1 = selection_x1;
		drag_selection_y1 = selection_y1;
		drag_selection_x2 = selection_x2;
		drag_selection_y2 = selection_y2;
		
		float x, y;
		script.mouse_layer(selection_layer, selection_sub_layer, x, y);
		drag_scale_start_distance = distance(x, y, anchor_x, anchor_y);
		drag_start_x = x;
		drag_start_y = y;
		
		if(has_custom_anchor)
		{
			custom_anchor_offset_x = selection_x - custom_anchor_x;
			custom_anchor_offset_y = selection_y - custom_anchor_y;
		}
		
		clear_highlighted_props();
		state = Scaling;
		script.ui.mouse_enabled = false;
	}
	
	private void idle_adjust_layer()
	{
		PropData@ prop_data = null;
		IWorldBoundingBox@ bounding_box = null;
		
		if(script.shift.down)
		{
			for(int i = 0; i < selected_props_count; i++)
			{
				@prop_data = @selected_props[i];
				prop_data.shift_layer(mouse.scroll, script.alt.down);
			}
			
			selection_bounding_box.x1 = selection_x + selection_x1;
			selection_bounding_box.y1 = selection_y + selection_y1;
			selection_bounding_box.x2 = selection_x + selection_x2;
			selection_bounding_box.y2 = selection_y + selection_y2;
		}
		else if(@hovered_prop != null)
		{
			@prop_data = hovered_prop;
			hovered_prop.shift_layer(mouse.scroll, script.alt.down);
			@bounding_box = prop_data;
		}
		
		if(@prop_data != null)
		{
			selection_angle = 0;
			recalculate_selection_bounds();
			update_selection_layer();
		}
		
		if(@bounding_box != null)
		{
			script.show_layer_sub_layer_overlay(bounding_box, prop_data.prop.layer(), prop_data.prop.sub_layer());
		}
		else if(@prop_data != null)
		{
			selection_bounding_box.layer = selection_layer;
			script.show_layer_sub_layer_overlay(
				@selection_bounding_box,
				prop_data.prop.layer(), prop_data.prop.sub_layer());
		}
		
		try_update_info();
	}
	
	private void state_moving()
	{
		if(script.escape_press || !mouse.left_down)
		{
			if(script.escape_press)
			{
				for(int i = 0; i < selected_props_count; i++)
				{
					selected_props[i].cancel_drag();
				}
				
				selection_x = selection_drag_start_x;
				selection_y = selection_drag_start_y;
			}
			
			clear_temporary_selection();
			
			if(show_selection)
			{
				check_rotation_handle();
			}
			
			state = Idle;
			script.ui.mouse_enabled = true;
			return;
		}
		
		float start_x, start_y;
		float mouse_x, mouse_y;
		script.mouse_layer(action_layer, action_sub_layer, mouse_x, mouse_y);
		script.snap(drag_start_x, drag_start_y, start_x, start_y, custom_grid);
		script.snap(mouse_x, mouse_y, mouse_x, mouse_y, custom_grid);
		const float drag_delta_x = mouse_x - start_x;
		const float drag_delta_y = mouse_y - start_y;
		
		for(int i = 0; i < selected_props_count; i++)
		{
			selected_props[i].do_drag(drag_delta_x, drag_delta_y);
		}
		
		selection_x = selection_drag_start_x + drag_delta_x;
		selection_y = selection_drag_start_y + drag_delta_y;
		
		if(show_selection)
		{
			check_rotation_handle();
			check_scale_handle();
		}
	}
	
	private void state_rotating()
	{
		if(script.space.down || script.escape_press || (drag_rotation_handle ? !mouse.left_down : !mouse.middle_down))
		{
			for(int i = 0; i < selected_props_count; i++)
			{
				selected_props[i].stop_rotate(script.escape_press);
			}
			
			if(script.escape_press)
			{
				selection_angle = selection_drag_start_angle;
			}
			
			if(!temporary_selection && show_selection)
			{
				check_rotation_handle();
				check_scale_handle();
			}
			
			drag_rotation_handle = false;
			clear_temporary_selection();
			state = Idle;
			script.ui.mouse_enabled = true;
			return;
		}
		
		const float anchor_x = has_custom_anchor ? custom_anchor_x : selection_x;
		const float anchor_y = has_custom_anchor ? custom_anchor_y : selection_y;
		
		float x, y;
		script.mouse_layer(selection_layer, selection_sub_layer, x, y);
		const float angle = atan2(anchor_y - y, anchor_x - x);
		selection_angle = angle - drag_offset_angle;
		script.snap(selection_angle, selection_angle);
		
		for(int i = 0; i < selected_props_count; i++)
		{
			selected_props[i].do_rotation(selection_angle * RAD2DEG);
		}
		
		if(has_custom_anchor)
		{
			rotate(custom_anchor_offset_x, custom_anchor_offset_y, selection_angle - selection_drag_start_angle, x, y);
			selection_x = custom_anchor_x + x;
			selection_y = custom_anchor_y + y;
		}
		
		if(show_selection)
		{
			for(int i = 0; i < selected_props_count; i++)
			{
				selected_props[i].update();
			}
			
			check_rotation_handle(true);
			check_scale_handle();
		}
	}
	
	private void state_scaling()
	{
		if(script.space.down || script.escape_press || !mouse.left_down)
		{
			for(int i = 0; i < selected_props_count; i++)
			{
				selected_props[i].stop_scale(script.escape_press);
			}
			
			if(!temporary_selection && show_selection)
			{
				check_rotation_handle();
				check_scale_handle(false);
			}
			
			clear_temporary_selection();
			state = Idle;
			script.ui.mouse_enabled = true;
			return;
		}
		
		const float anchor_x = has_custom_anchor ? custom_anchor_x : selection_x;
		const float anchor_y = has_custom_anchor ? custom_anchor_y : selection_y;
		
		float x, y;
		script.mouse_layer(selection_layer, selection_sub_layer, x, y);
		const float new_drag_distance = distance(x, y, anchor_x, anchor_y);
		const float length = magnitude(drag_start_x - anchor_x, drag_start_y - anchor_y);
		project(
			x - anchor_x, y - anchor_y,
			(drag_start_x - anchor_x) / length, (drag_start_y - anchor_y) / length,
			x, y);
		const float distance_sign = dot(
				x, y,
				drag_start_x - anchor_x, drag_start_y - anchor_y) < 0 ? -1 : 1;
		const float scale = new_drag_distance / drag_scale_start_distance * distance_sign;
		
		for(int i = 0; i < selected_props_count; i++)
		{
			selected_props[i].do_scale(scale, scale, script.alt.down);
		}
		
		if(script.alt.down)
		{
			update_selection_bounds(scale < 0);
		}
		else
		{
			if(has_custom_anchor)
			{
				selection_x = custom_anchor_x + custom_anchor_offset_x * scale;
				selection_y = custom_anchor_y + custom_anchor_offset_y * scale;
			}
			
			selection_x1 = drag_selection_x1 * scale;
			selection_y1 = drag_selection_y1 * scale;
			selection_x2 = drag_selection_x2 * scale;
			selection_y2 = drag_selection_y2 * scale;
		}
		
		if(show_selection)
		{
			for(int i = 0; i < selected_props_count; i++)
			{
				selected_props[i].update();
			}
			
			check_rotation_handle();
			check_scale_handle(true);
		}
		
		try_update_info();
	}
	
	private void state_selecting()
	{
		clear_pending_highlighted_props(false);
		
		if(script.escape_press)
		{
			state = Idle;
			script.ui.mouse_enabled = true;
			clear_highlighted_props(true);
			return;
		}
		
		// Find props in selection rect
		
		const float y1 = min(drag_start_y, mouse.y);
		const float y2 = max(drag_start_y, mouse.y);
		const float x1 = min(drag_start_x, mouse.x);
		const float x2 = max(drag_start_x, mouse.x);
		
		int i = script.g.get_prop_collision(y1, y2, x1, x2);
		
		while(i-- > 0)
		{
			prop@ p = script.g.get_prop_collision_index(i);
			const uint layer = p.layer();
			const uint sub_layer = p.sub_layer();
			
			if(!script.editor.check_layer_filter(layer))
				continue;
			
			if(locked_sublayers[layer * 25 + sub_layer])
				continue;
			
			if(locked_props.exists(p.id() + ''))
				continue;
			
			const array<array<float>>@ outline = @PROP_OUTLINES[p.prop_set() - 1][p.prop_group()][p.prop_index() - 1];
			PropData@ prop_data = highlight_prop(p, outline);
			
			if(!prop_data.selected)
			{
				prop_data.pending_selection = -2;
			}
			
			if(!prop_data.intersects_aabb(x1, y1, x2, y2))
				continue;
			
			if(select_rect_pending == 0)
			{
				if(mouse.left_down)
				{
					prop_data.pending_selection = 1;
				}
				else
				{
					select_prop(prop_data, SelectAction::Add);
				}
			}
			else if(select_rect_pending == 1)
			{
				if(!prop_data.selected)
				{
					if(mouse.left_down)
					{
						prop_data.pending_selection = 1;
					}
					else
					{
						select_prop(prop_data, SelectAction::Add);
					}
				}
			}
			else if(select_rect_pending == -1)
			{
				if(mouse.left_down)
				{
					prop_data.pending_selection = prop_data.selected ? -1 : -2;
				}
				else
				{
					select_prop(prop_data, SelectAction::Remove);
				}
			}
		}
		
		check_rotation_handle();
		check_scale_handle();
		
		// Complete selection
		
		if(!mouse.left_down)
		{
			state = Idle;
			script.ui.mouse_enabled = true;
			clear_highlighted_props(true);
		}
	}
	
	//
	
	private void reset()
	{
		select_none();
		state = Idle;
		clear_custom_anchor();
		temporary_selection = false;
		clear_highlighted_props();
		drag_rotation_handle = false;
		@previous_hovered_prop = null;
		selected_props_info = -1;
	}
	
	private void get_handle_position(const bool vertical, const float offset, float &out x, float &out y, const bool allow_flipped=true)
	{
		float sx, sy, sx1, sy1, sx2, sy2;
		float x1, y1, x2, y2, x3, y3, x4, y4;
		get_selection_rect(sx, sy, sx1, sy1, sx2, sy2, allow_flipped);
		
		rotate(sx1, sy1, selection_angle, x1, y1);
		rotate(sx2, sy1, selection_angle, x2, y2);
		rotate(sx2, sy2, selection_angle, x3, y3);
		rotate(sx1, sy2, selection_angle, x4, y4);
		
		if(vertical)
		{
			sx += (x1 + x2) * 0.5;
			sy += (y1 + y2) * 0.5;
		}
		else
		{
			sx += (x2 + x3) * 0.5;
			sy += (y2 + y3) * 0.5;
		}
		
		const float angle = vertical ? selection_angle - HALF_PI : selection_angle;
		
		x = sx + cos(angle) * (offset / script.zoom);
		y = sy + sin(angle) * (offset / script.zoom);
	}
	
	private bool check_rotation_handle(const bool force_highlight=false)
	{
		if(selected_props_count == 0 || temporary_selection)
			return false;
		
		float x, y;
		get_handle_position(true, Settings::RotationHandleOffset, x, y, false);
		
		return script.handles.circle(
			x, y,
			Settings::RotateHandleSize, Settings::RotateHandleColour, Settings::RotateHandleHoveredColour, force_highlight);
	}
	
	private bool check_scale_handle(const bool force_highlight=false)
	{
		if(selected_props_count == 0 || temporary_selection)
			return false;
		
		float x, y;
		get_handle_position(false, 0, x, y);
		
		return script.handles.square(
			x, y,
			Settings::ScaleHandleSize, selection_angle * RAD2DEG,
			Settings::RotateHandleColour, Settings::RotateHandleHoveredColour, force_highlight);
	}
	
	private void show_custom_anchor_info()
	{
		float x, y;
		script.transform_size(5 / script.zoom, 5 / script.zoom, 22, 22, custom_anchor_layer, custom_anchor_sub_layer, x, y);
		
		selection_bounding_box.layer = custom_anchor_layer;
		selection_bounding_box.x1 = custom_anchor_x - x;
		selection_bounding_box.y1 = custom_anchor_y - y;
		selection_bounding_box.x2 = custom_anchor_x + x;
		selection_bounding_box.y2 = custom_anchor_y + y;
		
		script.info_overlay.show(
			selection_bounding_box,
			'Custom Anchor Layer: ' + custom_anchor_layer, 0.75);
	}
	
	private void adjust_custom_anchor_layer(int dir)
	{
		custom_anchor_layer = mod(custom_anchor_layer + mouse.scroll, 20);
	}
	
	private void shift_props(const float dx, const float dy)
	{
		for(int i = 0; i < selected_props_count; i++)
		{
			selected_props[i].move(dx, dy);
		}
		
		selection_x += dx;
		selection_y += dy;
	}
	
	private void flip_props(const bool x, const bool y)
	{
		const float anchor_x = has_custom_anchor ? custom_anchor_x : selection_x;
		const float anchor_y = has_custom_anchor ? custom_anchor_y : selection_y;
		
		if(x)
		{
			for(int i = 0; i < selected_props_count; i++)
			{
				PropData@ data = @selected_props[i];
				data.start_scale(anchor_x, anchor_y);
				data.do_scale(-1, 1);
				data.stop_scale(false);
			}
		}
		else
		{
			for(int i = 0; i < selected_props_count; i++)
			{
				PropData@ data = @selected_props[i];
				data.start_scale(anchor_x, anchor_y);
				data.do_scale(1, -1);
				data.stop_scale(false);
			}
		}
		
		selection_angle = 0;
		recalculate_selection_bounds(true, selection_x, selection_y);
	}
	
	private void mirror_selected(const bool x_axis=true)
	{
		const float anchor_x = has_custom_anchor ? custom_anchor_x : selection_x;
		const float anchor_y = has_custom_anchor ? custom_anchor_y : selection_y;
		
		for(int i = 0; i < selected_props_count; i++)
		{
			PropData@ data = @selected_props[i];
			prop@ p = data.prop;
			
			if(x_axis)
			{
				p.x(anchor_x - (p.x() - anchor_x));
				p.scale_x(-p.scale_x());
			}
			else
			{
				p.y(anchor_y - (p.y() - anchor_y));
				p.scale_y(-p.scale_y());
			}
			
			// Make sure the angle is in the range -180 < 0 < 180
			const float rotation_normalised = shortest_angle_degrees(p.rotation(), 0);
			// Angle relative to up - up=0, left/right=90, down=180
			const float y_axis_angle = abs(shortest_angle_degrees(rotation_normalised, -90));
			// How much to adjust the rotation by to account for the flipped scale.
			const float angle_flip =
				// up=+180, left/right=no rotation, down=-180
				(y_axis_angle - 90) / 90.0 * 180
				// The amount needs to be flipped when the current angle points
				// in the other direction
				* (abs(rotation_normalised) > 90 ? -1 : 1);
			
			p.rotation(p.rotation() + angle_flip);
			
			data.update(true);
		}
		
		selection_angle = 0;
		recalculate_selection_bounds();
	}
	
	private void cycle_selected_palettes(int dir=1)
	{
		dir = dir < 1 ? -1 : 1;
		
		for(int i = 0; i < selected_props_count; i++)
		{
			PropData@ data = @selected_props[i];
			prop@ p = data.prop;
			const int count = data.spr.get_palette_count(data.sprite_name);
			p.palette(mod(p.palette() + dir, count));
		}
		
		try_update_info();
	}
	
	// //////////////////////////////////////////////////////////
	// Selection
	// //////////////////////////////////////////////////////////
	
	private void do_mouse_selection()
	{
		if(mouse.left_press)
		{
			// Add or remove from selection on shift/ctrl press
			if(script.shift.down || script.ctrl.down)
			{
				select_prop(hovered_prop, script.shift.down ? SelectAction::Add : SelectAction::Remove);
				mouse_press_modified = true;
			}
			else
			{
				@pressed_prop = hovered_prop;
			}
		}
		else if(mouse.left_release)
		{
			// Deselect all on normal click in empty space
			if(!mouse_press_moved && !mouse_press_modified)
			{
				select_prop(hovered_prop, SelectAction::Set);
			}
			
			mouse_press_modified = false;
			@pressed_prop = null;
		}
	}
	
	private void select_prop(PropData@ prop_data, const SelectAction action)
	{
		if(action == SelectAction::Set)
		{
			while(selected_props_count > 0)
			{
				PropData@ selected_prop_data = @selected_props[--selected_props_count];
				selected_prop_data.selected = false;
			}
			
			selection_layer = 0;
			selection_sub_layer = 0;
			selection_angle = 0;
			clear_custom_anchor();
		}
		
		if(@prop_data == null)
		{
			toolbar.update_buttons(selected_props_count);
			return;
		}
		
		if(
			action == SelectAction::Remove && !prop_data.selected ||
			action == SelectAction::Add && prop_data.selected
		)
			return;
		
		if(action == SelectAction::Add || action == SelectAction::Set)
		{
			if(selected_props_count >= selected_props_size)
			{
				selected_props.resize(selected_props_size = selected_props_count + 32);
			}
			
			@selected_props[selected_props_count++] = prop_data;
			prop_data.selected = true;
			
			if(prop_data.layer >= selection_layer)
			{
				selection_layer = prop_data.layer;
				
				if(prop_data.sub_layer > selection_sub_layer)
				{
					selection_sub_layer = prop_data.sub_layer;
				}
			}
		}
		else
		{
			selected_props.removeAt(selected_props.findByRef(@prop_data));
			selected_props.resize(selected_props_size);
			prop_data.selected = false;
			selected_props_count--;
			
			if(int(prop_data.prop.layer()) >= selection_layer)
			{
				update_selection_layer();
			}
		}
		
		selection_angle = 0;
		recalculate_selection_bounds();
		
		toolbar.update_buttons(selected_props_count);
	}
	
	private void select_none()
	{
		select_prop(null, SelectAction::Set);
	}
	
	private void recalculate_selection_bounds(const bool set_origin=false, const float origin_x=0, const float origin_y=0)
	{
		if(selected_props_count == 0)
			return;
		
		PropData@ prop_data = @selected_props[0];
		prop_data.store_selection_bounds();
		selection_x1 = prop_data.x + prop_data.local_x1;
		selection_y1 = prop_data.y + prop_data.local_y1;
		selection_x2 = prop_data.x + prop_data.local_x2;
		selection_y2 = prop_data.y + prop_data.local_y2;
		
		for(int i = selected_props_count - 1; i >= 1; i--)
		{
			@prop_data = @selected_props[i];
			prop_data.store_selection_bounds();
			
			if(prop_data.x + prop_data.local_x1 < selection_x1) selection_x1 = prop_data.x + prop_data.local_x1;
			if(prop_data.y + prop_data.local_y1 < selection_y1) selection_y1 = prop_data.y + prop_data.local_y1;
			if(prop_data.x + prop_data.local_x2 > selection_x2) selection_x2 = prop_data.x + prop_data.local_x2;
			if(prop_data.y + prop_data.local_y2 > selection_y2) selection_y2 = prop_data.y + prop_data.local_y2;
		}
		
		if(set_origin)
		{
			selection_x = origin_x;
			selection_y = origin_y;
		}
		else
		{
			selection_x = selected_props_count > 1 ? selection_x1 + (selection_x2 - selection_x1) * origin_align_x : prop_data.anchor_x;
			selection_y = selected_props_count > 1 ? selection_y1 + (selection_y2 - selection_y1) * origin_align_y : prop_data.anchor_y;
		}
		
		selection_x1 -= selection_x;
		selection_y1 -= selection_y;
		selection_x2 -= selection_x;
		selection_y2 -= selection_y;
	}
	
	private void update_selection_bounds(const bool flipped=false)
	{
		selection_x1 = MAX_FLOAT;
		selection_y1 = MAX_FLOAT;
		selection_x2 = MIN_FLOAT;
		selection_y2 = MIN_FLOAT;
		
		for(int i = 0; i < selected_props_count; i++)
		{
			PropData@ p = selected_props[i];
			float ox, oy, x1, y1, x2, y2;
			rotate(p.x - selection_x, p.y - selection_y, -selection_angle, ox, oy);
			p.get_selection_bounds(x1, y1, x2, y2);
			selection_x1 = min(selection_x1, selection_x + ox + x1);
			selection_y1 = min(selection_y1, selection_y + oy + y1);
			selection_x2 = max(selection_x2, selection_x + ox + x2);
			selection_y2 = max(selection_y2, selection_y + oy + y2);
		}
		
		selection_x1 -= selection_x;
		selection_y1 -= selection_y;
		selection_x2 -= selection_x;
		selection_y2 -= selection_y;
		
		if(flipped)
		{
			const float tx = selection_x1;
			const float ty = selection_y1;
			selection_x1 = selection_x2;
			selection_y1 = selection_y2;
			selection_x2 = tx;
			selection_y2 = ty;
		}
	}
	
	private void update_selection_layer()
	{
		selection_layer = 0;
		selection_sub_layer = 0;
		
		for(int i = 0; i < selected_props_count; i++)
		{
			PropData@ p = selected_props[i];
			
			if(p.layer >= selection_layer)
			{
				selection_layer = p.layer;
				
				if(p.sub_layer > selection_sub_layer)
				{
					selection_sub_layer = p.sub_layer;
				}
			}
		}
	}
	
	private void clear_temporary_selection()
	{
		if(!temporary_selection)
			return;
		
		select_none();
		temporary_selection = false;
	}
	
	private void copy_selected_props(const bool show_overlay=false)
	{
		PropsClipboardData@ props_clipboard = @script.props_clipboard;
		array<PropClipboardData>@ props = @props_clipboard.props;
		props.resize(selected_props_count);
		
		float ox = has_custom_anchor ? custom_anchor_x : selection_x;
		float oy = has_custom_anchor ? custom_anchor_y : selection_y;
		
		if(has_custom_anchor)
		{
			script.transform(ox, oy, custom_anchor_layer, custom_anchor_sub_layer, selection_layer, selection_sub_layer, ox, oy);
		}
		
		props_clipboard.x = ox;
		props_clipboard.y = oy;
		props_clipboard.layer = selection_layer;
		props_clipboard.sub_layer = selection_sub_layer;
		
		for(int i = 0; i < selected_props_count; i++)
		{
			PropData@ prop_data = @selected_props[i];
			PropClipboardData@ copy_data = @props[i];
			
			copy_data.prop_set		= prop_data.prop.prop_set();
			copy_data.prop_group	= prop_data.prop.prop_group();
			copy_data.prop_index	= prop_data.prop.prop_index();
			copy_data.palette		= prop_data.prop.palette();
			copy_data.layer			= prop_data.prop.layer();
			copy_data.sub_layer		= prop_data.prop.sub_layer();
			copy_data.x				= prop_data.x - ox;
			copy_data.y				= prop_data.y - oy;
			copy_data.rotation		= prop_data.prop.rotation();
			copy_data.scale_x		= prop_data.prop.scale_x();
			copy_data.scale_y		= prop_data.prop.scale_y();
			
			float x1 = prop_data.x + prop_data.local_x1 - ox;
			float y1 = prop_data.y + prop_data.local_y1 - oy;
			float x2 = prop_data.x + prop_data.local_x2 - ox;
			float y2 = prop_data.y + prop_data.local_y2 - oy;
			
			script.transform(x1, y1, copy_data.layer, copy_data.sub_layer, selection_layer, selection_sub_layer, x1, y1);
			script.transform(x2, y2, copy_data.layer, copy_data.sub_layer, selection_layer, selection_sub_layer, x2, y2);
			
			if(i == 0)
			{
				props_clipboard.x1 = x1;
				props_clipboard.y1 = y1;
				props_clipboard.x2 = x2;
				props_clipboard.y2 = y2;
			}
			else
			{
				if(x1 < props_clipboard.x1) props_clipboard.x1 = x1;
				if(y1 < props_clipboard.y1) props_clipboard.y1 = y1;
				if(x2 > props_clipboard.x2) props_clipboard.x2 = x2;
				if(y2 > props_clipboard.y2) props_clipboard.y2 = y2;
			}
		}
		
		if(show_overlay)
		{
			script.info_overlay.show(
				selection_x + selection_x1, selection_y + selection_y1,
				selection_x + selection_x2, selection_y + selection_y2,
				selected_props_count + ' prop' + (selected_props_count != 1 ? 's' : '') + ' copied', 0.75);
		}
	}
	
	private void paste(const bool into_place=false, const bool tile_aligned=false)
	{
		PropsClipboardData@ props_clipboard = @script.props_clipboard;
		array<PropClipboardData>@ props = @props_clipboard.props;
		const int count = int(props.length());
		
		if(count == 0)
			return;
		
		float x, y;
		
		if(into_place)
		{
			x = props_clipboard.x;
			y = props_clipboard.y;
		}
		else
		{
			float mx, my;
			script.mouse_layer(props_clipboard.layer, props_clipboard.sub_layer, mx, my);
			x = mx - props_clipboard.x1 - (props_clipboard.x2 - props_clipboard.x1) * 0.5;
			y = my - props_clipboard.y1 - (props_clipboard.y2 - props_clipboard.y1) * 0.5;
			
			if(tile_aligned)
			{
				x = floor(x / 48) * 48 + (props_clipboard.x - floor(props_clipboard.x / 48) * 48);
				y = floor(y / 48) * 48 + (props_clipboard.y - floor(props_clipboard.y / 48) * 48);
			}
		}
		
		select_none();
		origin_align_x = -props_clipboard.x1 / (props_clipboard.x2 - props_clipboard.x1);
		origin_align_y = -props_clipboard.y1 / (props_clipboard.y2 - props_clipboard.y1);
		
		for(int i = 0; i < count; i++)
		{
			PropClipboardData@ copy_data = @props[i];
			prop@ p = create_prop();
			p.prop_set       (copy_data.prop_set);
			p.prop_group     (copy_data.prop_group);
			p.prop_index     (copy_data.prop_index);
			p.palette        (copy_data.palette);
			p.layer          (copy_data.layer);
			p.sub_layer      (copy_data.sub_layer);
			p.x              (copy_data.x + x);
			p.y              (copy_data.y + y);
			p.rotation       (copy_data.rotation);
			p.scale_x        (copy_data.scale_x);
			p.scale_y        (copy_data.scale_y);
			
			const array<array<float>>@ outline = @PROP_OUTLINES[copy_data.prop_set - 1][copy_data.prop_group][copy_data.prop_index - 1];
			
			script.g.add_prop(p);
			
			PropData@ data = highlight_prop(p, @outline);
			select_prop(data, SelectAction::Add);
		}
		
		const float dx = selection_x1 - props_clipboard.x1;
		const float dy = selection_y1 - props_clipboard.y1;
		
		selection_x += dx;
		selection_y += dy;
		selection_x1 -= dx;
		selection_y1 -= dy;
		selection_x2 -= dx;
		selection_y2 -= dy;
		
		update_alignments_from_origin();
	}
	
	private void delete_selected()
	{
		for(int i = 0; i < selected_props_count; i++)
		{
			script.g.remove_prop(selected_props[i].prop);
		}
		
		select_none();
	}
	
	private void lock_selected()
	{
		if(selected_props_count == 0)
			return;
		
		for(int i = selected_props_count - 1; i >= 0; i--)
		{
			const string key = selected_props[i].prop.id() + '';
			
			if(locked_props.exists(key))
				continue;
			
			locked_props[key] = true;
			num_locked_props++;
		}
		
		show_prop_message(selected_props_count + ' props locked.');
		select_none();
	}
	
	private void unlock_all()
	{
		if(num_locked_props == 0)
			return;
		
		show_prop_message(num_locked_props + ' props unlocked.', true);
		
		locked_props.deleteAll();
		num_locked_props = 0;
	}
	
	private void lock_selected_sublayers(const bool all_layers)
	{
		if(selected_props_count == 0)
			return;
		
		int num_sub_layer = 0;
		
		for(int i = selected_props_count - 1; i >= 0; i--)
		{
			const uint layer_from = all_layers ? 0 : selected_props[i].prop.layer();
			const uint layer_to = all_layers ? 22 : layer_from;
			const uint sub_layer = selected_props[i].prop.sub_layer();
			for(uint layer = all_layers ? 0 : layer_from; layer <= layer_to; layer++)
			{
				const uint layer_index = layer * 25 + sub_layer;
				
				if(!locked_sublayers[layer_index])
				{
					locked_sublayers[layer_index] = true;
					num_sub_layer++;
				}
			}
		}
		
		show_prop_message(num_sub_layer + ' sub layers locked.');
		select_none();
	}
	
	private void unlock_all_sublayers()
	{
		int num_locked_sub_layers = 0;
		
		for(uint i = 0; i < locked_sublayers.length; i++)
		{
			if(!locked_sublayers[i])
				continue;
			
			locked_sublayers[i] = false;
			num_locked_sub_layers++;
		}
		
		show_prop_message(num_locked_sub_layers + ' sub layers unlocked.', true);
	}
	
	private void try_update_info()
	{
		if(@hovered_prop != null)
		{
			toolbar.show_prop_info(hovered_prop);
		}
		else if(selected_props_count == 1)
		{
			toolbar.show_prop_info(selected_props[0]);
		}
	}
	
	// Highlights
	
	private PropData@ is_prop_highlighted(prop@ prop)
	{
		const string key = prop.id() + '';
		
		return highlighted_props_map.exists(key)
			? cast<PropData@>(highlighted_props_map[key])
			: null;
	}
	
	private PropData@ highlight_prop(prop@ prop, const array<array<float>>@ outline)
	{
		const string key = prop.id() + '';
		PropData@ prop_data;
		
		if(highlighted_props_map.exists(key))
			return cast<PropData@>(highlighted_props_map[key]);
		
		@prop_data = prop_data_pool_count > 0
			? @prop_data_pool[--prop_data_pool_count]
			: PropData();
		
		prop_data.init(script, this, prop, @outline);
		
		if(highlighted_props_count >= highlighted_props_size)
		{
			highlighted_props.resize(highlighted_props_size += 32);
		}
		
		@highlighted_props[highlighted_props_count++] = @prop_data;
		@highlighted_props_map[key] = @prop_data;
		
		return prop_data;
	}
	
	private void clear_pending_highlighted_props(const bool visible=true)
	{
		const int pending = visible ? 0 : -2;
		for(int i = highlighted_props_count - 1; i >= 0; i--)
		{
			PropData@ d = @highlighted_props[i];
			d.pending_selection = d.selected ? 0 : pending;
		}
	}
	
	void clear_hovered_props()
	{
		for(int i = highlighted_props_count - 1; i >= 0; i--)
		{
			highlighted_props[i].hovered = false;
		}
		
		@hovered_prop = null;
	}
	
	void clear_highlighted_props(const bool clear_pending=false)
	{
		for(int i = highlighted_props_count - 1; i >= 0; i--)
		{
			PropData@ prop_data = @highlighted_props[i];
			
			if(clear_pending)
			{
				prop_data.pending_selection = 0;
				
				if(prop_data.selected)
					continue;
			}
			else if(prop_data.hovered || prop_data.selected || prop_data.pending_selection != 0)
			{
				continue;
			}
			
			if(prop_data_pool_count >= prop_data_pool_size)
			{
				prop_data_pool_size = ceil_int(prop_data_pool_size * 1.5);
				prop_data_pool.resize(prop_data_pool_size);
			}
			
			@prop_data_pool[prop_data_pool_count++] = @prop_data;
			@highlighted_props[i] = @highlighted_props[--highlighted_props_count];
			highlighted_props_map.delete(prop_data.key);
		}
	}
	
	// //////////////////////////////////////////////////////////
	// Picking
	// //////////////////////////////////////////////////////////
	
	void pick_props()
	{
		// Find all props under the mouse
		
		const float radius = max(PropToolSettings::SmallPropRadius, pick_radius) / script.zoom;
		int i = script.g.get_prop_collision(mouse.y - radius, mouse.y + radius, mouse.x - radius, mouse.x + radius);
		
		array<PropSortingData>@ props_under_mouse = @this.props_under_mouse;
		int num_props_under_mouse = 0;
		
		if(props_under_mouse_size < i)
		{
			props_under_mouse.resize(props_under_mouse_size = i + 32);
		}
		
		while(i-- > 0)
		{
			prop@ p = script.g.get_prop_collision_index(i);
			const uint layer = p.layer();
			const uint sub_layer = p.sub_layer();
			
			if(!script.editor.check_layer_filter(layer))
				continue;
			
			if(locked_sublayers[layer * 25 + sub_layer])
				continue;
			
			if(locked_props.exists(p.id() + ''))
				continue;
			
			const float prop_x = p.x();
			const float prop_y = p.y();
			const float rotation = p.rotation() * DEG2RAD * (p.scale_x() >= 0 ? 1.0 : -1.0) * (p.scale_y() >= 0 ? 1.0 : -1.0);
			const float layer_scale = layer <= 5 ? script.layer_scale(layer, sub_layer) : 1.0;
			const float backdrop_scale = layer <= 5 ? 2.0 : 1.0;
			const float scale_x = p.scale_x() / layer_scale * backdrop_scale;
			const float scale_y = p.scale_y() / layer_scale * backdrop_scale;
			
			// Calculate mouse "local" position relative to prop rotation and scale
			
			float local_x, local_y;
			float layer_mx, layer_my;
			script.mouse_layer(layer, sub_layer, layer_mx, layer_my);
			
			rotate(
				(layer_mx - prop_x) / scale_x,
				(layer_my - prop_y) / scale_y,
				-rotation, local_x, local_y);
			
			// Check for overlap with tiles, but allow interacting with selected props through tiles
			
			PropData@ prop_data = is_prop_highlighted(p);
			
			if((@prop_data == null || !prop_data.selected) && !pick_through_tiles && hittest_tiles(layer, sub_layer))
				continue;
			
			// Check if the mouse is inside to the prop
			
			const array<array<float>>@ outline = @PROP_OUTLINES[p.prop_set() - 1][p.prop_group()][p.prop_index() - 1];
			const array<bool>@ holes = @PROP_OUTLINES_HOLES_INFO[p.prop_set() - 1][p.prop_group()][p.prop_index() - 1];
			
			bool is_inside = hittest_prop(p, local_x, local_y, outline, holes);
			
			// If the mouse is not directly inside the prop outline, check if it close by, within some threshold
			
			const bool is_close = !is_inside
				? check_prop_distance(p, local_x, local_y, outline, holes)
				: false;
			
			if(is_inside || is_close)
			{
				PropSortingData@ data = @props_under_mouse[num_props_under_mouse++];
				@data.prop = @p;
				@data.outline = @outline;
				data.is_inside = is_inside ? 1 : 0;
				data.selected = @prop_data != null ? prop_data.selected : false;
				data.layer_position = script.layer_position(p.layer());
				data.scene_index = i;
			}
		}
		
		if(num_props_under_mouse == 0)
			return;
		
		// Sort to find the top most prop
		
		props_under_mouse.sortAsc(0, num_props_under_mouse);
		const int selected_index = (num_props_under_mouse - (hover_index_offset % num_props_under_mouse) - 1) % num_props_under_mouse;
		
		PropSortingData@ sorting_data = @props_under_mouse[selected_index];
		PropData@ prop_data = highlight_prop(@sorting_data.prop, @sorting_data.outline);
		prop_data.hovered = true;
		@hovered_prop = prop_data;
	}
	
	private bool hittest_tiles(const int prop_layer, const int prop_sublayer)
	{
		for(int i = script.layer_position(prop_layer); i <= 20; i++)
		{
			const int layer = script.layer_index(i);
			
			if(layer == prop_layer && prop_sublayer >= 10)
				continue;
			
			if(!script.editor.get_layer_visible(layer))
				continue;
			
			float mx, my;
			script.mouse_layer(layer, 10, mx, my);
			const int tx = floor_int(mx / 48);
			const int ty = floor_int(my / 48);
			tileinfo@ tile = script.g.get_tile(tx, ty, layer);
			
			if(!tile.solid())
				continue;
			
			float normal_x, normal_y;
			
			if(point_in_tile(mx, my, tx, ty, tile.type(), normal_x, normal_y))
				return true;
		}
		
		return false;
	}
	
	private bool hittest_prop(prop@ p, const float local_x, const float local_y,
		const array<array<float>>@ outline,
		const array<bool>@ holes)
	{
		bool inside = false;
		
		for(uint i = 0; i < outline.length(); i++)
		{
			if(point_in_polygon(local_x, local_y, @outline[i]))
			{
				if(pick_ignore_holes)
					return true;
				
				inside = holes[i] ? !inside : true;
			}
		}
		
		return inside;
	}
	
	private bool check_prop_distance(prop@ p, const float local_x, const float local_y,
		const array<array<float>>@ outline,
		const array<bool>@ holes)
	{
		const string prop_key = p.prop_set() + '.' + p.prop_group() + '.' + p.prop_index();
		const float prop_radius = (PropToolSettings::PropRadii.exists(prop_key)
			? float(PropToolSettings::PropRadii[prop_key])
			: pick_radius) / script.zoom;
		
		for(uint i = 0; i < outline.length(); i++)
		{
			if(distance_to_polygon_sqr(local_x, local_y, @outline[0]) <= prop_radius * prop_radius)
				return true;
			
			if(pick_ignore_holes)
				break;
		}
		
		return false;
	}
	
	// //////////////////////////////////////////////////////////
	// Other
	// //////////////////////////////////////////////////////////
	
	private void clear_custom_anchor()
	{
		if(custom_anchor_lock)
			return;
		
		toolbar.update_buttons(selected_props_count);
		has_custom_anchor = false;
	}
	
	void update_alignments_from_origin(const bool force_selection_update=false)
	{
		if(default_origin == 'top_left')
		{
			origin_align_x = 0;
			origin_align_y = 0;
		}
		else if(default_origin == 'top')
		{
			origin_align_x = 0.5;
			origin_align_y = 0;
		}
		else if(default_origin == 'top_right')
		{
			origin_align_x = 1;
			origin_align_y = 0;
		}
		else if(default_origin == 'right')
		{
			origin_align_x = 1;
			origin_align_y = 0.5;
		}
		else if(default_origin == 'bottom_right')
		{
			origin_align_x = 1;
			origin_align_y = 1;
		}
		else if(default_origin == 'bottom')
		{
			origin_align_x = 0.5;
			origin_align_y = 1;
		}
		else if(default_origin == 'bottom_left')
		{
			origin_align_x = 0;
			origin_align_y = 1;
		}
		else if(default_origin == 'left')
		{
			origin_align_x = 0;
			origin_align_y = 0.5;
		}
		else
		{
			origin_align_x = 0.5;
			origin_align_y = 0.5;
		}
		
		if(force_selection_update)
		{
			selection_angle = 0;
			recalculate_selection_bounds();
		}
	}
	
	void snap_custom_anchor()
	{
		if(has_custom_anchor)
		{
			script.snap(custom_anchor_x, custom_anchor_y, custom_anchor_x, custom_anchor_y, custom_grid, true);
		}
		else if(selected_props_count > 0)
		{
			const float x = selection_x;
			const float y = selection_y;
			script.snap(selection_x, selection_y, selection_x, selection_y, custom_grid, true);
			const float dx = selection_x - x;
			const float dy = selection_y - y;
			selection_x1 -= dx;
			selection_y1 -= dy;
			selection_x2 -= dx;
			selection_y2 -= dy;
		}
	}
	
	void align(const AlignmentEdge align)
	{
		if(selected_props_count < 2)
			return;
		
		const bool horizontal = align == Left || align == Centre || align == Right;
		const int dir = align == Left || align == Top ? -1 : align == Right || align == Bottom ? 1 : 0;
		
		PropData@ p = @selected_props[0];
		float min = horizontal ? (p.x + p.local_x1) : (p.y + p.local_y1);
		float max = horizontal ? (p.x + p.local_x2) : (p.y + p.local_y2);
		
		for(int i = selected_props_count - 1; i >= 0; i--)
		{
			@p = @selected_props[i];
			
			if(horizontal)
			{
				if(p.x + p.local_x1 < min) min = p.x + p.local_x1;
				if(p.x + p.local_x2 > max) max = p.x + p.local_x2;
			}
			else
			{
				if(p.y + p.local_y1 < min) min = p.y + p.local_y1;
				if(p.y + p.local_y2 > max) max = p.y + p.local_y2;
			}
		}
		
		const float pos = dir == -1
			? min
			: dir == 1 ? max
				: (min + max) * 0.5;
		
		for(int i = selected_props_count - 1; i >= 0; i--)
		{
			@p = @selected_props[i];
			
			if(horizontal)
			{
				if(dir == -1)
					p.x = pos - p.local_x1;
				else if(dir == 1)
					p.x = pos - p.local_x2;
				else
					p.x = pos - (p.local_x1 + p.local_x2) * 0.5;
				
				p.prop.x(p.x);
			}
			else
			{
				if(dir == -1)
					p.y = pos - p.local_y1;
				else if(dir == 1)
					p.y = pos - p.local_y2;
				else
					p.y = pos - (p.local_y1 + p.local_y2) * 0.5;
				
				p.prop.y(p.y);
			}
		}
		
		selection_angle = 0;
		recalculate_selection_bounds();
	}
	
	void distribute(const AlignmentEdge align)
	{
		if(selected_props_count < 3)
			return;
		
		const bool horizontal = align == Left || align == Centre || align == Right || align == Horizontal;
		
		if(props_align_data_size < selected_props_count)
		{
			props_align_data.resize(props_align_data_size += selected_props_count + 32);
		}
		
		float props_width = 0;
		
		for(int i = selected_props_count - 1; i >= 0; i--)
		{
			PropData@ p = @selected_props[i];
			@props_align_data[i].data = p;
			props_align_data[i].x = horizontal ? p.x + p.local_x1 : p.y + p.local_y1;
			props_width += horizontal ? p.local_x2 - p.local_x1 : p.local_y2 - p.local_y1;
		}
		
		props_align_data.sortAsc(0, selected_props_count);
		
		PropData@ first = @props_align_data[0].data;
		PropData@ last  = @props_align_data[selected_props_count - 1].data;
		float min, max;
		
		switch(align)
		{
			case AlignmentEdge::Left:
			case AlignmentEdge::Top:
				min = horizontal ? first.x + first.local_x1 : first.y + first.local_y1;
				max = horizontal ? last.x + last.local_x1 : last.y + last.local_y1;
				break;
			case AlignmentEdge::Right:
			case AlignmentEdge::Bottom:
				min = horizontal ? first.x + first.local_x2 : first.y + first.local_y2;
				max = horizontal ? last.x + last.local_x2 : last.y + last.local_y2;
				break;
			case AlignmentEdge::Centre:
			case AlignmentEdge::Middle:
				min = horizontal ? first.x + (first.local_x1 + first.local_x2) * 0.5 : first.y + (first.local_y1 + first.local_y2) * 0.5;
				max = horizontal ? last.x + (last.local_x1 + last.local_x2) * 0.5 : last.y + (last.local_y1 + last.local_y2) * 0.5;
				break;
			case AlignmentEdge::Horizontal:
			case AlignmentEdge::Vertical:
				min = horizontal ? (first.x + first.local_x1) : (first.y + first.local_y1);
				max = horizontal ? (last.x  + last.local_x2)  : (last.y + last.local_y2);
				break;
		}
		
		const bool is_spaced = align == Horizontal || align == Vertical;
		const float spacing = (is_spaced
			? (max - min) - props_width
			: (max - min)
		) / (selected_props_count - 1);
		
		float x = min + spacing;
		
		if(is_spaced)
		{
			x += horizontal ? (first.local_x2 - first.local_x1) : (first.local_y2 - first.local_y1);
		}
		
		for(int i = 1; i < selected_props_count - 1; i++)
		{
			PropData@ p = @props_align_data[i].data;
			
			switch(align)
			{
				case AlignmentEdge::Horizontal:
				case AlignmentEdge::Left:		p.x = x - p.local_x1; break;
				
				case AlignmentEdge::Vertical:
				case AlignmentEdge::Top:		p.y = x - p.local_y1; break;
				
				case AlignmentEdge::Right:		p.x = x - p.local_x2; break;
				case AlignmentEdge::Bottom:		p.y = x - p.local_y2; break;
				case AlignmentEdge::Centre:		p.x = x - (p.local_x1 + p.local_x2) * 0.5; break;
				case AlignmentEdge::Middle:		p.y = x - (p.local_y1 + p.local_y2) * 0.5; break;
			}
			
			p.prop.x(p.x);
			p.prop.y(p.y);
			
			if(is_spaced)
			{
				x += horizontal ? p.local_x2 - p.local_x1 : p.local_y2 - p.local_y1;
			}
			
			x += spacing;
		}
		
		selection_angle = 0;
		recalculate_selection_bounds();
	}
	
	bool is_custom_anchor_active()
	{
		return has_custom_anchor;
	}
	
	void export_selected_props(
		const PropExportType type,
		const int layer, const int sub_layer, const bool override_layer, const bool override_sub_layer,
		const uint colour)	
	{
		if(selected_props_count == 0)
			return;
		
		const float origin_x = has_custom_anchor ? custom_anchor_x : selection_x;
		const float origin_y = has_custom_anchor ? custom_anchor_y : selection_y;
		
		switch(type)
		{
			case PropExportType::SpriteBatch:
				exporter.sprite_batch(
					@selected_props, selected_props_count, origin_x, origin_y,
					layer, sub_layer, override_layer, override_sub_layer);
				break;
			case PropExportType::SpriteGroup:
				exporter.sprite_group(
					@selected_props, selected_props_count, origin_x, origin_y,
					layer, sub_layer, override_layer, override_sub_layer,
					colour);
				break;
		}
	}
	
	void correct_prop_values()
	{
		const float anchor_x = has_custom_anchor ? custom_anchor_x : selection_x;
		const float anchor_y = has_custom_anchor ? custom_anchor_y : selection_y;
		
		for(int i = 0; i < selected_props_count; i++)
		{
			PropData@ data = @selected_props[i];
			prop@ p = data.prop;
			
			data.anchor_world(anchor_x, anchor_y);
			data.set_prop_rotation(round(p.rotation()));
			data.update();
			data.init_anchors();
			
			data.start_scale(anchor_x, anchor_y);
			//puts(
			//	p.scale_y(),
			//	get_valid_prop_scale(abs(p.scale_y())),
			//	get_valid_prop_scale(abs(p.scale_y())) / abs(p.scale_y()) );
			data.do_scale(
				get_valid_prop_scale(abs(p.scale_x())) / abs(p.scale_x()),
				get_valid_prop_scale(abs(p.scale_y())) / abs(p.scale_y())
			);
			data.stop_scale(false);
			
			p.x(round(p.x()));
			p.y(round(p.y()));
			
			data.update(true);
			data.init_anchors();
		}
		
		selection_angle = 0;
		recalculate_selection_bounds();
	}
	
	void cycle_highlight_selection()
	{
		highlight_selection = PropToolHighlight((highlight_selection - 1) % (PropToolHighlight::Both + 1));
	}
	
	void get_selection_rect(
		float &out x, float &out y, float &out x1, float &out y1, float &out x2, float &out y2,
		const bool allow_flipped=false)
	{
		script.transform(selection_x, selection_y, selection_layer, selection_sub_layer, 22, 22, x, y);
		script.transform_size(
			allow_flipped ? selection_x1 : min(selection_x1, selection_x2),
			allow_flipped ? selection_y1 : min(selection_y1, selection_y2),
			selection_layer, selection_sub_layer, 22, 22, x1, y1);
		script.transform_size(
			allow_flipped ? selection_x2 : max(selection_x1, selection_x2),
			allow_flipped ? selection_y2 : max(selection_y1, selection_y2),
			selection_layer, selection_sub_layer, 22, 22, x2, y2);
	}
	
	void show_prop_message(const string &in msg, const bool mouse = false, const float display_time=0.75)
	{
		if(mouse)
		{
			script.info_overlay.show(script.mouse, msg, display_time);
			return;
		}
		
		float sx, sy, sx1, sy1, sx2, sy2;
		get_selection_rect(sx, sy, sx1, sy1, sx2, sy2);
		script.info_overlay.show(
			sx + sx1, sy + sy1,
			sx + sx2, sy + sy2,
			msg, display_time);
	}
	
}
