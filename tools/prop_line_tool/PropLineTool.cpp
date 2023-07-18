#include '../../../../lib/drawing/Sprite.cpp';
#include '../../../../lib/props/common.cpp';
#include '../../../../lib/string.cpp';

#include 'PropLineProp.cpp';
#include 'PropLineRotationMode.cpp';
#include 'PropLineRotationOffsets.cpp';
#include 'PropLineToolbar.cpp';
#include 'PropLineToolState.cpp';

const string PROP_LINE_TOOL_SPRITES_BASE = SPRITES_BASE + 'prop_line_tool/';
const string EMBED_spr_icon_prop_line_tool = SPRITES_BASE + 'icon_prop_line_tool.png';

class PropLineTool : Tool
{
	
	private PropLineToolState state = Idle;
	private Mouse@ mouse;
	private PropTool@ prop_tool;
	private array<PropData@>@ highlighted_props;
	private PropData@ pick_data;
	
	private int prop_set = -1;
	private int prop_group;
	private int prop_index;
	private int prop_palette;
	private float x1, y1;
	private float x2, y2;
	private int scale_index = 0;
	private float scale = 1.0;
	private float scale_x, scale_y;
	float rotation;
	private int layer, sub_layer;
	private bool has_auto_rotation_offset;
	
	private Sprite spr;
	private string sprite_set, sprite_name;
	
	private bool recaclulate_props;
	/** A per prop value used to inset certain props, e.g. moon and sun */
	private float auto_spacing_adjustment;
	private float start_dx, start_dy;
	private float drag_ox, drag_oy;
	private DragHandleType drag_handle = DragHandleType::None;
	
	private bool dragging_start_point;
	private bool mouse_moved;
	private bool is_angle_locked;
	private bool is_angle_lock_persisted;
	private float lock_angle;
	
	private int props_list_size = 32;
	private int props_count;
	private array<PropLineProp> props(props_list_size);
	
	private PropLineToolbar toolbar;
	
	// Settings
	
	/** If false, will snap to angle instead. */
	bool snap_to_grid = false;
	/** If false, scroll wheel adjusts rotation */
	bool scroll_spacing = true;
	PropLineSpacingMode spacing_mode = PropLineSpacingMode::Fixed;
	PropLineRotationMode rotation_mode = PropLineRotationMode::Auto;
	float spacing = 50;
	float spacing_offset;
	float rotation_offset;
	bool auto_spacing = true;
	int repeat_count = 1;
	float repeat_spacing = 50;
	
	PropLineTool(AdvToolScript@ script)
	{
		super(script, 'Props', 'Prop Line Tool');
		
		init_shortcut_key(VK::Q, ModifierKey::Alt);
	}
	
	void build_sprites(message@ msg) override
	{
		build_sprite(msg, 'icon_prop_line_tool');
		
		toolbar.build_sprites(msg);
	}
	
	void create(ToolGroup@ group) override
	{
		set_icon(SPRITE_SET, 'icon_prop_line_tool');
	}
	
	void on_init() override
	{
		@prop_tool = cast<PropTool@>(script.get_tool('Prop Tool'));
		@mouse = @script.mouse;
	}
	
	// //////////////////////////////////////////////////////////
	// Callbacks
	// //////////////////////////////////////////////////////////
	
	protected void on_editor_unloaded_impl() override
	{
		reset();
	}
	
	protected void on_select_impl()
	{
		script.hide_gui_panels(true);
		
		toolbar.show(script, this);
		
		if(prop_set == -1 || state == PropLineToolState::Picking)
		{
			show_pick_popup();
		}
	}
	
	protected void on_deselect_impl()
	{
		script.hide_info_popup();
		script.hide_gui_panels(false);
		
		reset();
		toolbar.hide();
	}
	
	protected void step_impl() override
	{
		switch(state)
		{
			case PropLineToolState::Idle: state_idle(); break;
			case PropLineToolState::Picking: state_picking(); break;
			case PropLineToolState::Dragging: state_dragging(); break;
			case PropLineToolState::Pending: state_pending(); break;
		}
	}
	
	protected void draw_impl(const float sub_frame) override
	{
		if(state == PropToolState::Idle || state == PropToolState::Picking)
		{
			if(prop_set != -1)
			{
				spr.draw(
					script.layer, script.sub_layer,
					0, prop_palette, x1, y1,
					rotation + (rotation_mode == PropLineRotationMode::Auto ? rotation_offset : 0.0),
					scale_x * scale, scale_y * scale,
					state == PropToolState::Picking || script.space.down ? 0x77ffffff : 0xffffffff,
					calc_bg_scale(script.layer));
			}
		}
		
		if(state == PropToolState::Picking)
		{
			if(@pick_data != null)
			{
				pick_data.draw(PropToolHighlight::Both);
			}
		}
		
		if(state >= PropToolState::Dragging)
		{
			for(int i = 0; i < props_count; i++)
			{
				PropLineProp@ p = @props[i];
				spr.draw(
					layer, sub_layer,
					0, prop_palette, p.x, p.y, p.rotation, p.scale_x, p.scale_y,
					0xffffffff, calc_bg_scale(layer));
			}
			
			const uint clr = 0x77ffffff;
			float x3, y3, x4, y4;
			script.transform(x1, y1, script.layer, 22, x3, y3);
			script.transform(x2, y2, script.layer, 22, x4, y4);
			script.g.draw_line_world(22, 22, x3, y3, x4, y4, 2 / script.zoom, clr);
		}
	}
	
	// //////////////////////////////////////////////////////////
	// States
	// //////////////////////////////////////////////////////////
	
	private void state_idle()
	{
		if(!is_angle_lock_persisted)
		{
			is_angle_locked = false;
		}
		
		user_update_scroll_mode();
		user_update_layer();
		user_update_rotation(rotation_mode != PropLineRotationMode::Auto);
		user_update_scale();
		user_update_mirror();
		user_update_snap();
		user_update_lock_angle(true);
		
		if(is_angle_locked)
		{
			lock_angle = rotation * DEG2RAD;
		}
		
		update_start_point();
		layer = script.layer;
		sub_layer = script.sub_layer;
		
		if(mouse.left_press && prop_set != -1)
		{
			mouse_moved = false;
			dragging_start_point = false;
			update_end_point();
			calculate_props();
			state = PropLineToolState::Dragging;
			active = true;
			return;
		}
		
		if(mouse.right_press)
		{
			show_pick_popup();
			state = PropLineToolState::Picking;
			return;
		}
	}
	
	private void state_picking()
	{
		update_start_point();
		
		prop_tool.clear_hovered_props();
		@pick_data = null;
		
		if(script.mouse_in_scene && !script.space.down && !script.handles.mouse_over)
		{
			prop_tool.pick_props();
		}
		
		prop_tool.clear_highlighted_props();
		
		if(prop_tool.highlighted_props_list_count > 0)
		{
			prop@ prev_prop = @pick_data != null ? pick_data.prop : null;
			@pick_data = @prop_tool.highlighted_props_list[0];
			prop_set = pick_data.prop.prop_set();
			prop_group = pick_data.prop.prop_group();
			prop_index = pick_data.prop.prop_index();
			prop_palette = pick_data.prop.palette();
			scale_x = sign(pick_data.prop.scale_x());
			scale_y = sign(pick_data.prop.scale_y());
			scale_index = round_int(log(pick_data.prop.scale_x()) / log(50.0) * 24.0);
			scale = pow(50.0, scale_index / 24.0);
			rotation = pick_data.prop.rotation();
			update_sprite();
			
			const string key = prop_set + '.' + prop_group + '.' + prop_index;
			
			auto_spacing_adjustment = PropLineAutoOffsets.exists(key)
				? float(PropLineAutoOffsets[key])
				: 0.0;
			
			if(rotation_mode == PropLineRotationMode::Auto)
			{
				if(PropLineRotationOffsets.exists(key))
				{
					rotation_offset = float(PropLineRotationOffsets[key]);
					has_auto_rotation_offset = true;
				}
				else if(has_auto_rotation_offset)
				{
					rotation_offset = 0.0;
					has_auto_rotation_offset = false;
				}
			}
		}
		
		if(prop_set != -1 && !mouse.right_down)
		{
			if(auto_spacing)
			{
				calculate_auto_spacing();
			}
			
			state = PropLineToolState::Idle;
			script.hide_info_popup();
		}
	}
	
	private void state_dragging()
	{
		if(!mouse_moved && mouse.moved)
		{
			mouse_moved = true;
		}
		
		user_update_snap();
		user_update_scroll_mode();
		user_update_spacing();
		user_update_mirror();
		user_update_repeat();
		user_update_lock_angle();
		update_end_point(
			!dragging_start_point || !mouse_moved,
			dragging_start_point ? drag_ox : 0.0,
			dragging_start_point ? drag_oy : 0.0,
			!dragging_start_point && is_angle_locked);
		
		if(mouse.right_press)
		{
			dragging_start_point = true;
			mouse_moved = false;
			float mx, my;
			script.transform(mouse.x, mouse.y, 22, layer, mx, my);
			drag_ox = (x2 - mx);
			drag_oy = (y2 - my);
			start_dx = x1 - x2;
			start_dy = y1 - y2;
		}
		
		if(dragging_start_point)
		{
			x1 = x2 + start_dx;
			y1 = y2 + start_dy;
			recaclulate_props = true;
			
			if(!mouse.right_down)
			{
				dragging_start_point = false;
			}
		}
		
		if(recaclulate_props)
		{
			if(auto_spacing)
			{
				calculate_auto_spacing();
			}
			
			calculate_props();
		}
		
		if(script.input.key_check_pressed_gvb(GVB::Return))
		{
			mouse_moved = true;
			state = PropLineToolState::Pending;
			active = false;
			script.show_info_popup(
				'Enter : Place props\n' + 
				'Escape: Cancel',
				null , PopupPosition::Below);
			return;
		}
		
		if(!mouse.left_down || script.escape_press)
		{
			if(!script.escape_press)
			{
				place_props();
			}
			
			state = PropLineToolState::Idle;
			update_start_point();
			active = false;
			return;
		}
	}
	
	private void state_pending()
	{
		user_update_snap();
		user_update_scroll_mode();
		user_update_spacing();
		user_update_mirror();
		user_update_repeat();
		user_update_lock_angle();
		
		const bool drag_p1 = script.handles.circle(x1, y1, Settings::RotateHandleSize,
			Settings::RotateHandleColour, Settings::RotateHandleHoveredColour);
		const bool drag_p2 = script.handles.circle(x2, y2, Settings::RotateHandleSize,
			Settings::RotateHandleColour, Settings::RotateHandleHoveredColour);
		const bool drag_line = script.handles.line(x1, y1, x2, y2, Settings::RotateHandleSize * 0.5,
			Settings::RotateHandleColour, Settings::RotateHandleHoveredColour);
		
		if(drag_handle == DragHandleType::None)
		{
			if(drag_p1 || drag_p2 || drag_line)
			{
				float mx, my;
				script.transform(mouse.x, mouse.y, 22, layer, mx, my);
				drag_handle = drag_p1 ? DragHandleType::Start : drag_p2 ? DragHandleType::End : DragHandleType::Segment;
				drag_ox = (drag_p1 || drag_line ? x1 : x2) - mx;
				drag_oy = (drag_p1 || drag_line ? y1 : y2) - my;
				
				start_dx = x1 - x2;
				start_dy = y1 - y2;
			}
		}
		else if(mouse.left_down)
		{
			if(drag_handle == DragHandleType::Start)
			{
				update_start_point(true, drag_ox, drag_oy, is_angle_locked);
			}
			else if(drag_handle == DragHandleType::End)
			{
				update_end_point(true, drag_ox, drag_oy, is_angle_locked);
			}
			else
			{
				update_start_point(false, drag_ox, drag_oy);
				x2 = x1 - start_dx;
				y2 = y1 - start_dy;
			}
		}
		else
		{
			drag_handle = DragHandleType::None;
		}
		
		if(recaclulate_props)
		{
			if(auto_spacing)
			{
				calculate_auto_spacing();
			}
			
			calculate_props();
		}
		
		if(script.scene_focus && (script.input.key_check_pressed_gvb(GVB::Return) || script.escape_press))
		{
			if(!script.escape_press)
			{
				place_props();
			}
			
			state = PropLineToolState::Idle;
			update_start_point();
			active = false;
			script.hide_info_popup();
			return;
		}
	}
	
	private void show_pick_popup()
	{
		script.show_info_popup(
			prop_set == -1 ? 'Hold right mouse to select a prop' : 'Release to select a prop',
			null , PopupPosition::Below);
	}
	
	private void update_start_point(const bool allow_angle_snap=false, const float ox=0, const float oy=0, const bool do_lock_angle=false)
	{
		const float x1_prev = x1;
		const float y1_prev = y1;
		
		script.transform(mouse.x + ox, mouse.y + oy, 22, layer, x1, y1);
		
		if(do_lock_angle)
		{
			const float dir_x = -cos(lock_angle);
			const float dir_y = -sin(lock_angle);
			project(x1 - x2, y1 - y2, dir_x, dir_y, x1, y1);
			x1 += x2;
			y1 += y2;
		}
		else if(snap_to_grid || !allow_angle_snap)
		{
			script.snap(x1, y1, x1, y1);
		}
		else
		{
			snap_angle(x2, y2, x1, y1, x1, y1);
		}
		
		recaclulate_props = recaclulate_props || (x1 != x1_prev || y1 != y1_prev);
	}
	
	private void update_end_point(const bool allow_angle_snap=true, const float ox=0, const float oy=0, const bool do_lock_angle=false)
	{
		const float x2_prev = x2;
		const float y2_prev = y2;
		
		script.transform(mouse.x + ox, mouse.y + oy, 22, layer, x2, y2);
		
		if(do_lock_angle)
		{
			const float dir_x = cos(lock_angle);
			const float dir_y = sin(lock_angle);
			project(x2 - x1, y2 - y1, dir_x, dir_y, x2, y2);
			x2 += x1;
			y2 += y1;
		}
		else if(snap_to_grid || !allow_angle_snap)
		{
			script.snap(x2, y2, x2, y2);
		}
		else
		{
			snap_angle(x1, y1, x2, y2, x2, y2);
		}
		
		recaclulate_props = recaclulate_props || (x2 != x2_prev || y2 != y2_prev);
	}
	
	private void snap_angle(const float x1, const float y1, const float x2, const float y2, float &out ox, float &out oy)
	{
		float angle = atan2(y2 - y1, x2 - x1);
		float snapped_angle;
		script.snap(angle, snapped_angle);
		
		if(angle != snapped_angle)
		{
			const float length = distance(x1, y1, x2, y2);
			ox = x1 + cos(snapped_angle) * length;
			oy = y1 + sin(snapped_angle) * length;
		}
		else
		{
			ox = x2;
			oy = y2;
		}
	}
	
	// //////////////////////////////////////////////////////////
	// Other
	// //////////////////////////////////////////////////////////
	
	private void reset()
	{
		state = PropLineToolState::Idle;
	}
	
	private void update_sprite()
	{
		sprite_from_prop(prop_set, prop_group, prop_index, sprite_set, sprite_name);
		spr.set(sprite_set, sprite_name);
	}
	
	private void calculate_props()
	{
		const float length = distance(x1, y1, x2, y2);
		const float dx = x2 - x1;
		const float dy = y2 - y1;
		const float nx = length > 0 ? dx / length : 1.0;
		const float ny = length > 0 ? dy / length : 0.0;
		const float angle = atan2(dy, dx);
		
		float final_rotation;
		
		if(rotation_mode == PropLineRotationMode::Auto)
		{
			if(mouse_moved)
			{
				rotation = angle * RAD2DEG;
			}
			final_rotation = rotation + rotation_offset;
		}
		else
		{
			final_rotation = rotation;
		}
		
		const float repeat_spacing = max(this.repeat_spacing, 0.1);
		float spacing = this.spacing;
		
		if(auto_spacing)
		{
			spacing += auto_spacing_adjustment * scale + spacing_offset;
		}
		
		spacing = max(layer <= 5 ? spacing / script.g.layer_scale(layer) * 2 : spacing, 0.1);
		const int row_count = ceil_int(max((length + spacing * 0.5) / spacing, 1.0));
		props_count = row_count * repeat_count;
		
		while(props_count > props_list_size)
		{
			props.resize(props_list_size *= 2);
		}
		
		if(spacing_mode == PropLineSpacingMode::Fill)
		{
			if(row_count > 1)
			{
				spacing = (length - spacing * 0.5) / (row_count - 1);
			}
		}
		
		
		int prop_index = 0;
		for(int j = 0; j < repeat_count; j++)
		{
			float x = x1 - ny * repeat_spacing * j;
			float y = y1 + nx * repeat_spacing * j;
			
			for(int i = 0; i < row_count; i++)
			{
				PropLineProp@ p = @props[prop_index++];
				p.x = x;
				p.y = y;
				p.rotation = final_rotation;
				p.scale_x = scale_x * scale;
				p.scale_y = scale_y * scale;
				
				x += nx * spacing;
				y += ny * spacing;
			}
		}
		
		recaclulate_props = false;
	}
	
	/// Must call `calculate_props` before calling this.
	private void place_props()
	{
		for(int i = 0; i < props_count; i++)
		{
			PropLineProp@ data = @props[i];
			float px, py;
			spr.real_position(data.x, data.y, data.rotation, px, py, data.scale_x, data.scale_y,
				calc_bg_scale(layer));
			prop@ p = create_prop(prop_set, prop_group, prop_index, px, py, layer, sub_layer, data.rotation);
			p.palette(prop_palette);
			p.scale_x(data.scale_x);
			p.scale_y(data.scale_y);
			script.g.add_prop(p);
		}
	}
	
	private float calc_bg_scale(const int layer)
	{
		return layer <= 5 ? 2.0 / script.g.layer_scale(layer) : 1.0;
	}
	
	//
	
	private void user_update_layer()
	{
		if(script.shift.down || script.ctrl.down && script.alt.down)
			return;
		
		script.scroll_layer(true, true, false, LayerInfoDisplay::Compound, null, script.main_toolbar, 0.75);
		layer = script.layer;
		sub_layer = script.sub_layer;
		update_start_point();
	}
	
	private void user_update_rotation(const bool notify=true)
	{
		if(mouse.scroll == 0 || (!script.shift.down && (script.ctrl.down || script.alt.down)))
			return;
		
		float dir = sign(mouse.scroll);
		
		if(script.shift.down && script.ctrl.down)
			dir *= 1;
		else if(script.shift.down)
			dir *= 5;
		else
			dir *= 10;
		
		update_rotation(rotation - dir, notify);
	}
	
	private void user_update_scale()
	{
		if(mouse.scroll == 0 || !script.ctrl.down || !script.alt.down)
			return;
		
		const int prev_scale_index = scale_index;
		
		scale_index = int(min(24, max(-24, scale_index + mouse.scroll)));
		scale = pow(50.0, scale_index / 24.0);
		
		if(scale_index != prev_scale_index)
		{
			script.show_info('Scale: ' + string::nice_float(scale, 3));
		}
	}
	
	private void user_update_mirror()
	{
		if(!script.scene_focus)
			return;
		
		bool updated = false;
		
		if(script.input.key_check_pressed_gvb(GVB::BracketOpen))
		{
			scale_x = -scale_x;
			updated = true;
		}
		
		if(script.input.key_check_pressed_gvb(GVB::BracketClose))
		{
			scale_y = -scale_y;
			updated = true;
		}
		
		if(updated)
		{
			recaclulate_props = true;
		}
	}
	
	private void user_update_snap()
	{
		if(script.shift.double_press)
		{
			update_snap_mode(!snap_to_grid);
		}
	}
	
	private void user_update_spacing()
	{
		if(mouse.scroll == 0)
			return;
		
		if(scroll_spacing)
		{
			float amount = 2;
			
			if(script.shift.down)
				amount = 20;
			else if(script.ctrl.down)
				amount = 5;
			else if(script.alt.down)
				amount = 1;
			
			if(auto_spacing)
			{
				update_spacing_offset(spacing_offset - mouse.scroll * amount);
			}
			else
			{
				update_spacing(spacing - mouse.scroll * amount);
			}
		}
		else if(rotation_mode == PropLineRotationMode::Auto)
		{
			float dir = sign(mouse.scroll);
			
			if(script.shift.down && script.ctrl.down)
				dir *= 1;
			else if(script.shift.down)
				dir *= 5;
			else
				dir *= 10;
			
			update_rotation_offset(normalize_degress(rotation_offset - dir));
		}
		else
		{
			user_update_rotation();
		}
	}
	
	private void user_update_repeat()
	{
		if(script.key_repeat_gvb(GVB::UpArrow))
		{
			update_repeat_count(repeat_count + (script.shift.down ? 10 : script.ctrl.down ? 2 : 1));
		}
		else if(script.key_repeat_gvb(GVB::DownArrow))
		{
			update_repeat_count(repeat_count - (script.shift.down ? 10 : script.ctrl.down ? 2 : 1));
		}
		
		if(script.key_repeat_gvb(GVB::RightArrow))
		{
			update_repeat_spacing(repeat_spacing + (script.shift.down ? 10.0 : script.ctrl.down ? 2.0 : 1.0));
		}
		else if(script.key_repeat_gvb(GVB::LeftArrow))
		{
			update_repeat_spacing(repeat_spacing - (script.shift.down ? 10.0 : script.ctrl.down ? 2.0 : 1.0));
		}
	}
	
	private void user_update_lock_angle(const bool lock_angle_from_rotation = false)
	{
		if(!script.input.key_check_pressed_vk(VK::A))
			return;
		
		if(!is_angle_locked)
		{
			is_angle_locked = true;
			
			lock_angle = lock_angle_from_rotation
				? rotation * DEG2RAD : atan2(y2 - y1, x2 - x1);
		}
		else if(!is_angle_lock_persisted)
		{
			is_angle_lock_persisted = true;
		}
		else
		{
			is_angle_locked = false;
			is_angle_lock_persisted = false;
		}
		
		script.show_info('Angle ' + (is_angle_locked ? '' : 'un') + 'locked' + (is_angle_locked && !is_angle_lock_persisted ? ' (temp)' : ''), 0.75);
	}
	
	private void calculate_auto_spacing()
	{
		if(prop_set == -1)
			return;
		
		const float bg_scale = calc_bg_scale(layer);
		
		const float length = distance(x1, y1, x2, y2);
		const float dx = x2 - x1;
		const float dy = y2 - y1;
		const float nx = length > 0 ? dx / length : 1.0;
		const float ny = length > 0 ? dy / length : 0.0;
		const float angle = atan2(dy, dx);
		
		// Caclulate the line in local prop space
		// --------------------------------------
		
		const float prop_angle = rotation_mode == PropLineRotationMode::Auto
			? rotation_offset * DEG2RAD
			: rotation * DEG2RAD;
		
		const float ray_size = sqrt(
			spr.sprite_width * spr.sprite_width * 0.25 +
			spr.sprite_height * spr.sprite_height * 0.25) * 1.5;
		float lox = 0;
		float loy = 0;
		float l1x = -ray_size;
		float l1y = 0;
		float l2x = ray_size;
		float l2y = 0;
		
		if(
			rotation_mode != PropLineRotationMode::Auto &&
			(state == PropLineToolState::Dragging || state == PropLineToolState::Pending))
		{
			rotate(l1x, l1y, angle, l1x, l1y);
			rotate(l2x, l2y, angle, l2x, l2y);
		}
		
		rotate(l1x, l1y, -prop_angle, l1x, l1y);
		rotate(l2x, l2y, -prop_angle, l2x, l2y);
		
		lox *= scale_x; loy *= scale_y;
		l1x *= scale_x; l1y *= scale_y;
		l2x *= scale_x; l2y *= scale_y;
		
		float prop_ox = spr.sprite_offset_x - spr.sprite_width * spr.origin_x;
		float prop_oy = spr.sprite_offset_y - spr.sprite_height * spr.origin_y;
		
		lox -= prop_ox; loy -= prop_oy;
		l1x -= prop_ox; l1y -= prop_oy;
		l2x -= prop_ox; l2y -= prop_oy;
		
		// --------------------------------------
		
		float t1_max = -1;
		float t2_max = -1;
		
		const array<array<float>>@ outline = @PROP_OUTLINES[prop_set - 1][prop_group][prop_index - 1];
		for(int i = 0, path_count = int(outline.length()); i < path_count; i++)
		{
			const array<float>@ path = @outline[i];
			const int count = int(path.length());
			float p1x = path[count - 2];
			float p1y = path[count - 1];
			
			for(int j = 0; j < count; j += 2)
			{
				float p2x = path[j];
				float p2y = path[j + 1];
				
				float ix, iy, t;
				if(line_line_intersection(lox, loy, l1x, l1y, p1x, p1y, p2x, p2y, ix, iy, t) && t > t1_max)
				{
					t1_max = t;
				}
				if(line_line_intersection(lox, loy, l2x, l2y, p1x, p1y, p2x, p2y, ix, iy, t) && t > t2_max)
				{
					t2_max = t;
				}
				
				p1x = p2x;
				p1y = p2y;
			}
		}
		
		if(t1_max == -1 || t2_max == -1)
			return;
		
		const float spacing_prev = spacing;
		spacing = max(ray_size * t1_max + ray_size * t2_max, 0.0) * scale;
		recaclulate_props = recaclulate_props || spacing != spacing_prev;
	}
	
	private void user_update_scroll_mode()
	{
		if(script.alt.double_press)
		{
			update_scroll_mode(!scroll_spacing);
		}
	}
	
	//
	
	void update_snap_mode(const bool new_snap, const bool notify=true)
	{
		if(snap_to_grid == new_snap)
			return;
		
		snap_to_grid = new_snap;
		toolbar.update_snap_mode();
		
		if(notify)
		{
			script.show_info('Snap: ' + (snap_to_grid ? 'Grid' : 'Angle'));
		}
	}
	
	void update_spacing_mode(const PropLineSpacingMode new_spacing, const bool notify=true)
	{
		if(spacing_mode == new_spacing)
			return;
		
		spacing_mode = new_spacing;
		recaclulate_props = true;
		
		toolbar.update_spacing_mode();
		
		if(notify)
		{
			script.show_info('Spacing: ' + (spacing_mode == PropLineSpacingMode::Fixed ? 'Fixed' : 'Fill'));
		}
	}
	
	void update_rotation_mode(const PropLineRotationMode new_rotation, const bool notify=true)
	{
		if(rotation_mode == new_rotation)
			return;
		
		rotation_mode = new_rotation;
		recaclulate_props = true;
		
		toolbar.update_rotation_mode();
		
		if(rotation_mode == PropLineRotationMode::Fixed)
		{
			update_rotation(round(rotation), false);
		}
		
		if(notify)
		{
			script.show_info('Rotation: ' + (rotation_mode == PropLineRotationMode::Fixed ? 'Fixed' : 'Auto'));
		}
	}
	
	void update_spacing(float new_spacing, const bool notify=true)
	{
		if(new_spacing < 0)
		{
			new_spacing = 0;
		}
		
		if(spacing == new_spacing)
			return;
		
		spacing = new_spacing;
		recaclulate_props = true;
		
		toolbar.update_spacing();
		
		if(notify)
		{
			script.show_info('Spacing: ' + string::nice_float(spacing, 1, true));
		}
	}
	
	void update_spacing_offset(const float new_spacing_offset, const bool notify=true)
	{
		if(spacing_offset == new_spacing_offset)
			return;
		
		spacing_offset = new_spacing_offset;
		recaclulate_props = true;
		
		toolbar.update_spacing_offset();
		
		if(notify)
		{
			script.show_info('Spacing: ' + string::nice_float(spacing_offset, 1, true));
		}
	}
	
	void update_rotation(float new_rotation, const bool notify=true)
	{
		new_rotation = normalize_degress(new_rotation);
		
		if(rotation == new_rotation)
			return;
		
		rotation = new_rotation;
		has_auto_rotation_offset = false;
		recaclulate_props = true;
		
		toolbar.update_rotation();
		
		if(notify)
		{
			script.show_info('Rotation: ' + string::nice_float(rotation, 1, true));
		}
	}
	
	void update_rotation_offset(const float new_rotation_offset, const bool notify=true)
	{
		if(rotation_offset == new_rotation_offset)
			return;
		
		rotation_offset = new_rotation_offset;
		recaclulate_props = true;
		
		toolbar.update_rotation_offset();
		
		if(notify)
		{
			script.show_info('Rotation Offset: ' + string::nice_float(rotation_offset, 1, true));
		}
	}
	
	void update_repeat_count(int new_repeat_count, const bool notify=true)
	{
		if(new_repeat_count < 1)
		{
			new_repeat_count = 1;
		}
		
		if(repeat_count == new_repeat_count)
			return;
		
		repeat_count = new_repeat_count;
		recaclulate_props = true;
		
		toolbar.update_repeat_count();
		
		if(notify)
		{
			script.show_info('Repeat Count: ' + repeat_count);
		}
	}
	
	void update_repeat_spacing(float new_repeat_spacing, const bool notify=true)
	{
		if(new_repeat_spacing < 0)
		{
			new_repeat_spacing = 0;
		}
		
		if(repeat_spacing == new_repeat_spacing)
			return;
		
		repeat_spacing = new_repeat_spacing;
		recaclulate_props = true;
		
		toolbar.update_repeat_spacing();
		
		if(notify)
		{
			script.show_info('Repeat spacing: ' + string::nice_float(repeat_spacing, 1, true));
		}
	}
	
	void update_auto_spacing(const bool new_auto_spacing, const bool notify=true)
	{
		if(auto_spacing == new_auto_spacing)
			return;
		
		auto_spacing = new_auto_spacing;
		recaclulate_props = true;
		
		if(auto_spacing)
		{
			calculate_auto_spacing();
		}
		
		toolbar.update_auto_spacing();
		
		if(notify)
		{
			script.show_info('Auto spacing: ' + (auto_spacing ? 'On' : 'Off'));
		}
	}
	
	void update_scroll_mode(const bool new_scroll_mode, const bool notify=true)
	{
		if(scroll_spacing == new_scroll_mode)
			return;
		
		scroll_spacing = new_scroll_mode;
		toolbar.update_scroll_mode();
		
		if(notify)
		{
			script.show_info('Scroll mode: ' + (scroll_spacing ? 'Spacing' : 'Rotation'));
		}
	}
	
}
