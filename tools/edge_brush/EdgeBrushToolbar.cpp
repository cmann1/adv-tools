#include '../../../../lib/ui3/elements/extra/PopupButton.cpp';
#include '../../../../lib/ui3/elements/Checkbox.cpp';
#include '../../../../lib/ui3/elements/MultiButton.cpp';
#include '../../../../lib/ui3/elements/NumberSlider.cpp';
#include '../../../../lib/ui3/elements/Toolbar.cpp';
#include '../../../../lib/ui3/layouts/GridLayout.cpp';

#include 'EdgeMaskImage.cpp';

const string EMBED_spr_edgebrush_mode_brush				= EDGE_BRUSH_SPRITES_BASE + 'edgebrush_mode_brush.png';
const string EMBED_spr_edgebrush_mode_precision			= EDGE_BRUSH_SPRITES_BASE + 'edgebrush_mode_precision.png';
const string EMBED_spr_edgebrush_edge_mask_off			= EDGE_BRUSH_SPRITES_BASE + 'edgebrush_edge_mask_off.png';
const string EMBED_spr_edgebrush_edge_mask_edge			= EDGE_BRUSH_SPRITES_BASE + 'edgebrush_edge_mask_edge.png';
const string EMBED_spr_edgebrush_edge_corner			= EDGE_BRUSH_SPRITES_BASE + 'edgebrush_edge_corner.png';
const string EMBED_spr_edgebrush_inside_tile			= EDGE_BRUSH_SPRITES_BASE + 'edgebrush_inside_tile.png';
const string EMBED_spr_edgebrush_update_collision		= EDGE_BRUSH_SPRITES_BASE + 'edgebrush_update_collision.png';
const string EMBED_spr_edgebrush_update_priority		= EDGE_BRUSH_SPRITES_BASE + 'edgebrush_update_priority.png';
const string EMBED_spr_edgebrush_update_custom			= EDGE_BRUSH_SPRITES_BASE + 'edgebrush_update_custom.png';
const string EMBED_spr_edgebrush_edge_facing_external	= EDGE_BRUSH_SPRITES_BASE + 'edgebrush_edge_facing_external.png';
const string EMBED_spr_edgebrush_edge_facing_internal	= EDGE_BRUSH_SPRITES_BASE + 'edgebrush_edge_facing_internal.png';
const string EMBED_spr_edgebrush_edge_facing_both		= EDGE_BRUSH_SPRITES_BASE + 'edgebrush_edge_facing_both.png';
const string EMBED_spr_edgebrush_update_neighbour		= EDGE_BRUSH_SPRITES_BASE + 'edgebrush_update_neighbour.png';
const string EMBED_spr_edgebrush_render_mode_draw		= EDGE_BRUSH_SPRITES_BASE + 'edgebrush_render_mode_draw.png';

class EdgeBrushToolbar
{
	
	private AdvToolScript@ script;
	private EdgeBrushTool@ tool;
	
	private Toolbar@ toolbar;
	private MultiButton@ mode_btn;
	private PopupButton@ edge_mask_btn;
	private EdgeMaskImage@ edge_mask_img;
	private array<Checkbox@> edge_checkboxes(4);
	private NumberSlider@ radius_slider;
	private Button@ collision_btn;
	private Button@ priority_btn;
	private PopupButton@ custom_bits_btn;
	private MultiButton@ facing_btn;
	private Checkbox@ internal_sprites_checkbox;
	private Button@ update_neightbour_btn;
	private Button@ inside_only_btn;
	private MultiButton@ render_mode_btn;
	
	private EventCallback@ on_edge_mask_change_delegate = EventCallback(this.on_edge_mask_change);
	private EventCallback@ on_edge_mask_click_delegate = EventCallback(this.on_edge_mask_click);
	private EventCallback@ on_custom_bit_change_delegate = EventCallback(this.on_custom_bit_change);
	
	void build_sprites(message@ msg)
	{
		build_sprite(msg, 'edgebrush_mode_brush');
		build_sprite(msg, 'edgebrush_mode_precision');
		build_sprite(msg, 'edgebrush_edge_mask_off');
		build_sprite(msg, 'edgebrush_edge_mask_edge');
		build_sprite(msg, 'edgebrush_edge_corner');
		build_sprite(msg, 'edgebrush_inside_tile');
		build_sprite(msg, 'edgebrush_update_collision');
		build_sprite(msg, 'edgebrush_update_priority');
		build_sprite(msg, 'edgebrush_update_custom');
		build_sprite(msg, 'edgebrush_edge_facing_external');
		build_sprite(msg, 'edgebrush_edge_facing_internal');
		build_sprite(msg, 'edgebrush_edge_facing_both');
		build_sprite(msg, 'edgebrush_update_neighbour');
		build_sprite(msg, 'edgebrush_render_mode_draw');
		
	}
	
	void show(AdvToolScript@ script, EdgeBrushTool@ tool)
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
		
		EventCallback@ button_select = EventCallback(on_toolbar_button_select);
		EventCallback@ on_checkbox_change = EventCallback(this.on_checkbox_change);
		
		// Mode button
		//{
			@mode_btn = MultiButton(ui);
			mode_btn.add('brush', SPRITE_SET, 'edgebrush_mode_brush');
			mode_btn.set_tooltip('Mode: Brush');
			mode_btn.add('precision', SPRITE_SET, 'edgebrush_mode_precision');
			mode_btn.set_tooltip('Mode: Precision');
			mode_btn.name = 'mode';
			mode_btn.fit_to_contents();
			mode_btn.select.on(button_select);
			toolbar.add_child(mode_btn);
			script.init_icon(mode_btn);
		//}
		
		// Edge mask button
		//{
			@edge_mask_img = EdgeMaskImage(ui);
			edge_mask_img.edge_mask = tool.edge_mask;
			@edge_mask_btn = PopupButton(ui, edge_mask_img);
			edge_mask_btn.name = 'edge_mask';
			@edge_mask_btn.tooltip = PopupOptions(ui, 'Edge mask');
			script.init_icon(edge_mask_btn);
			toolbar.add(edge_mask_btn);
			
			Container@ edge_mask_contents = Container(ui);
			GridLayout@ grid = GridLayout(ui, 3, 0, 0, Row, Stretch, Stretch);
			grid.column_spacing = 1;
			grid.row_spacing = 1;
			@edge_mask_contents.layout = grid;
		
			// TL
			make_edge_corner(edge_mask_contents, 0);
			// T
			make_edge_checkbox(edge_mask_contents, 'top', Top);
			// TR
			make_edge_corner(edge_mask_contents, 90);
			// L
			make_edge_checkbox(edge_mask_contents, 'left', Left);
			// M filler
			Container@ middle = Container(ui);
			middle.width = 0;
			middle.height = 0;
			edge_mask_contents.add_child(middle);
			// R
			make_edge_checkbox(edge_mask_contents, 'right', Right);
			// BL
			make_edge_corner(edge_mask_contents, 270);
			// B
			make_edge_checkbox(edge_mask_contents, 'bottom', Bottom);
			// BR
			make_edge_corner(edge_mask_contents, 180);
			
			edge_mask_contents.fit_to_contents(true);
			@edge_mask_btn.popup.content_element = edge_mask_contents;
			edge_mask_btn.popup.padding = style.spacing;
		//}
		
		// Inside only
		//{
			@inside_only_btn = toolbar.create_button(SPRITE_SET, 'edgebrush_inside_tile');
			inside_only_btn.selectable = true;
			inside_only_btn.name = 'inside_tile';
			@inside_only_btn.tooltip = PopupOptions(ui, 'Inside tiles only');
			inside_only_btn.select.on(button_select);
			toolbar.add(inside_only_btn);
			script.init_icon(inside_only_btn);
		//}
		
		// Radius slider
		//{
			@radius_slider = NumberSlider(ui);
			@radius_slider.tooltip = PopupOptions(ui, 'Radius');
			radius_slider.change.on(EventCallback(on_radius_slider_change));
			toolbar.add_child(radius_slider);
		//}
		
		// Collision button
		//{
			@collision_btn = toolbar.create_button(SPRITE_SET, 'edgebrush_update_collision');
			collision_btn.selectable = true;
			collision_btn.name = 'collision';
			@collision_btn.tooltip = PopupOptions(ui, 'Collision');
			collision_btn.select.on(button_select);
			toolbar.add(collision_btn);
			script.init_icon(collision_btn);
		//}
		
		// Priority button
		//{
			@priority_btn = toolbar.create_button(SPRITE_SET, 'edgebrush_update_priority');
			priority_btn.selectable = true;
			priority_btn.name = 'priority';
			@priority_btn.tooltip = PopupOptions(ui, 'Priority');
			priority_btn.select.on(button_select);
			toolbar.add(priority_btn);
			script.init_icon(priority_btn);
		//}
		
		// Custom button
		//{
			@custom_bits_btn = PopupButton(ui, SPRITE_SET, 'edgebrush_update_custom');
			custom_bits_btn.name = 'custom_bits';
			@custom_bits_btn.tooltip = PopupOptions(ui, 'Corner Bits');
			toolbar.add(custom_bits_btn);
			script.init_icon(custom_bits_btn);
			
			Container@ custom_bits_contents = Container(ui);
			FlowLayout@ layout = FlowLayout(ui, FlowDirection::Row, FlowAlign::Start, FlowAlign::Start, FlowWrap::NoWrap, FlowFit::None);
			@custom_bits_contents.layout = layout;
			
			Divider@ divider = Divider(ui);
			
			make_bit_checkbox(custom_bits_contents, '1');
			make_bit_checkbox(custom_bits_contents, '2');
			//custom_bits_contents.add_child(divider);
			//make_bit_checkbox(custom_bits_contents, '4');
			//make_bit_checkbox(custom_bits_contents, '5');
			//make_bit_checkbox(custom_bits_contents, '6');
			//make_bit_checkbox(custom_bits_contents, '7');
			
			divider.height = custom_bits_contents.children[0].height;
			
			custom_bits_contents.fit_to_contents(true);
			@custom_bits_btn.popup.content_element = custom_bits_contents;
			custom_bits_btn.popup.padding = style.spacing;
		//}
		
		// Facing button
		//{
			@facing_btn = MultiButton(ui);
			facing_btn.add('external', SPRITE_SET, 'edgebrush_edge_facing_external');
			facing_btn.set_tooltip('Edges: External');
			facing_btn.add('internal', SPRITE_SET, 'edgebrush_edge_facing_internal');
			facing_btn.set_tooltip('Edges: Internal');
			facing_btn.add('both', SPRITE_SET, 'edgebrush_edge_facing_both');
			facing_btn.set_tooltip('Edges: Both');
			facing_btn.name = 'edge_facing';
			facing_btn.fit_to_contents();
			facing_btn.select.on(button_select);
			toolbar.add_child(facing_btn);
			script.init_icon(facing_btn);
		//}
		
		// Internal sprites checkbox
		//{
			@internal_sprites_checkbox = Checkbox(ui);
			internal_sprites_checkbox.name = 'internal_sprites';
			internal_sprites_checkbox.height = facing_btn.height;
			@internal_sprites_checkbox.tooltip = PopupOptions(ui, 'Check internal sprites');
			internal_sprites_checkbox.change.on(on_checkbox_change);
			toolbar.add_child(internal_sprites_checkbox);
		//}
		
		// Update neighbours
		//{
			@update_neightbour_btn = toolbar.create_button(SPRITE_SET, 'edgebrush_update_neighbour');
			update_neightbour_btn.selectable = true;
			update_neightbour_btn.name = 'update_neighbour';
			@update_neightbour_btn.tooltip = PopupOptions(ui, 'Update neighbours');
			update_neightbour_btn.select.on(button_select);
			toolbar.add(update_neightbour_btn);
			script.init_icon(update_neightbour_btn);
		//}
		
		toolbar.create_divider();
		
		// Render mode button
		//{
			@render_mode_btn = MultiButton(ui);
			render_mode_btn.add('external', SPRITE_SET, 'icon_visible', Settings::IconSize, Settings::IconSize);
			render_mode_btn.set_tooltip('Render Mode: Always');
			render_mode_btn.add('internal', SPRITE_SET, 'edgebrush_render_mode_draw', Settings::IconSize, Settings::IconSize);
			render_mode_btn.set_tooltip('Render Mode: Active');
			render_mode_btn.add('both', SPRITE_SET, 'icon_invisible', Settings::IconSize, Settings::IconSize);
			render_mode_btn.set_tooltip('Render Mode: Never');
			render_mode_btn.name = 'render_mode';
			render_mode_btn.fit_to_contents();
			render_mode_btn.select.on(button_select);
			toolbar.add_child(render_mode_btn);
			script.init_icon(render_mode_btn);
		//}
		
		// Finish up
		update_mode();
		update_brush_radius();
		update_collision();
		update_priority();
		update_facing();
		update_internal_sprites();
		update_update_neighbour();
		update_render_mode();
		
		ui.add_child(toolbar);
		script.window_manager.register_element(toolbar);
	}
	
	void update_mode()
	{
		mode_btn.selected_index = tool.mode;
		
		const bool brush_mode = tool.mode == Brush;
		const bool precision_mode = tool.mode == Precision;
		
		radius_slider.disabled = tool.mode != Brush;
		update_neightbour_btn.disabled = tool.mode != Precision;
		inside_only_btn.disabled = tool.mode != Precision;
	}
	
	void update_edge_mask()
	{
		if(edge_mask_img.edge_mask == tool.edge_mask)
			return;
		
		edge_mask_img.edge_mask = tool.edge_mask;
		
		for(TileEdge edge = TileEdge::Top; edge <= TileEdge::Right; edge++)
		{
			edge_checkboxes[edge].checked = tool.edge_mask & (1 << edge) != 0;
		}
	}
	
	void update_brush_radius()
	{
		radius_slider.value = tool.brush_radius;
	}
	
	void update_collision()
	{
		collision_btn.selected = tool.update_collision;
	}
	
	void update_priority()
	{
		priority_btn.selected = tool.update_priority;
	}
	
	void update_facing()
	{
		facing_btn.selected_index = tool.edge_facing;
		internal_sprites_checkbox.disabled = tool.edge_facing == Both;
	}
	
	void update_internal_sprites()
	{
		internal_sprites_checkbox.checked = tool.check_internal_sprites;
	}
	
	void update_update_neighbour()
	{
		update_neightbour_btn.selected = tool.precision_update_neighbour;
	}
	
	void update_render_mode()
	{
		render_mode_btn.selected_index = tool.render_mode;
	}
	
	void show_edge_info(TileEdgeData@ data)
	{
		string edge_name = '';
		switch(data.selected_edge)
		{
			case TileEdge::Top: edge_name = 'Top'; break;
			case TileEdge::Bottom: edge_name = 'Bottom'; break;
			case TileEdge::Left: edge_name = 'Left'; break;
			case TileEdge::Right: edge_name = 'Right'; break;
		}
		
		string text =
			edge_name + '\n' +
			string::reversed(bin(data.edge, 4));
		script.show_info_popup(text, toolbar);
	}
	
	private void make_edge_checkbox(Container@ container , const string &in name, const TileEdge edge)
	{
		Checkbox@ cbx = Checkbox(script.ui);
		cbx.name = name;
		cbx.checked = tool.edge_mask & (1 << edge) != 0;
		cbx.fit(0);
		cbx.change.on(on_edge_mask_change_delegate);
		cbx.mouse_button_click.on(on_edge_mask_click_delegate);
		container.add_child(cbx);
		
		@edge_checkboxes[edge] = cbx;
	}
	
	private void make_edge_corner(Container@ container, const int rotation)
	{
		Image@ img = Image(script.ui, SPRITE_SET, 'edgebrush_edge_corner');
		img.rotation = rotation;
		img.padding = 1;
		
		switch(rotation)
		{
			case 0: // TL
				img.align_h = 1;
				img.align_v = 1;
				break;
			case 90: // TR
				img.align_h = 0;
				img.align_v = 1;
				break;
			case 180: // BR
				img.align_h = 0;
				img.align_v = 0;
				break;
			case 270: // BL
				img.align_h = 1;
				img.align_v = 0;
				break;
		}
		
		container.add_child(img);
		
		script.init_icon(img);
	}
	
	private void make_bit_checkbox(Container@ container, const string &in name)
	{
		Checkbox@ cbx = Checkbox(script.ui);
		cbx.name = name;
		cbx.fit(0);
		cbx.change.on(on_custom_bit_change_delegate);
		container.add_child(cbx);
	}
	
	private TileEdge get_edge(const string & in name)
	{
		if(name == 'bottom')
			return Bottom;
		else if(name == 'left')
			return Left;
		else if(name == 'right')
			return Right;
		
		return Top;
	}
	
	// //////////////////////////////////////////////////////////
	// Events
	// //////////////////////////////////////////////////////////
	
	private void on_toolbar_button_select(EventInfo@ event)
	{
		const string name = event.target.name;
		
		if(name == 'mode')
		{
			tool.update_mode(EdgeBrushMode(mode_btn.selected_index), false);
		}
		else if(name == 'edge_facing')
		{
			tool.update_edge_facing(EdgeFacing(facing_btn.selected_index), false);
		}
		else if(name == 'collision')
		{
			tool.do_update_collision(collision_btn.selected);
		}
		else if(name == 'priority')
		{
			tool.do_update_priority(priority_btn.selected);
		}
		else if(name == 'update_neighbour')
		{
			tool.update_update_neighbour(update_neightbour_btn.selected);
		}
		else if(name == 'inside_tile')
		{
			tool.update_inside_tile_only(inside_only_btn.selected);
		}
		else if(name == 'render_mode')
		{
			tool.update_render_mode(EdgeBrushRenderMode(render_mode_btn.selected_index));
		}
	}
	
	private void on_checkbox_change(EventInfo@ event)
	{
		Checkbox@ cbx = cast<Checkbox@>(event.target);
		const string name = cbx.name;
		
		if(name == 'internal_sprites')
		{
			tool.update_internal_sprites(cbx.checked);
		}
	}
	
	private void on_edge_mask_change(EventInfo@ event)
	{
		Checkbox@ cbx = cast<Checkbox@>(event.target);
		tool.update_edge_mask(get_edge(cbx.name), cbx.checked);
	}
	
	private void on_edge_mask_click(EventInfo@ event)
	{
		if(event.button != Right)
			return;
		
		uint8 edge_mask = tool.edge_mask;
		Checkbox@ cbx = cast<Checkbox@>(event.target);
		const TileEdge edge = get_edge(cbx.name);
		// Turn this edge on and others off if this edge is off, or this edge and any of the other edges are one
		const bool turn_on = (edge_mask & (1 << edge) == 0) || ((edge_mask & ~(1 << edge)) != 0);
		edge_mask = 1 << edge;
		tool.update_edge_mask(turn_on ? edge_mask : ~edge_mask);
	}
	
	private void on_custom_bit_change(EventInfo@ event)
	{
		Checkbox@ cbx = cast<Checkbox@>(event.target);
		const uint bit_index = parseInt(cbx.name);
		uint8 mask = tool.update_custom;
		mask &= ~(1 << bit_index);
		mask |= uint(cbx.checked ? 1 : 0) << bit_index;
		tool.update_custom = mask;
	}
	
	private void on_radius_slider_change(EventInfo@ event)
	{
		tool.update_brush_radius(radius_slider.value, 0);
	}
	
}
