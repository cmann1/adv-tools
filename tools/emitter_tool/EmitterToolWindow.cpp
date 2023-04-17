#include '../../../../lib/ui3/elements/extra/SelectButton.cpp';
#include '../../../../lib/ui3/elements/Label.cpp';
#include '../../../../lib/ui3/elements/LayerButton.cpp';
#include '../../../../lib/ui3/elements/RotationWheel.cpp';
#include '../../../../lib/ui3/elements/Select.cpp';
#include '../../../../lib/ui3/elements/Window.cpp';

#include 'EmitterIdData.cpp';

class EmitterToolWindow
{
	
	private AdvToolScript@ script;
	private EmitterTool@ tool;
	
	private Window@ window;
	private Select@ emitter_id_select;
	private LayerButton@ layer_button;
	private RotationWheel@ rotation_wheel;
	private Label@ id_label;
	private Label@ emitter_id_label;
	
	private SelectButton@ other_ids_button;
	private ListView@ other_ids_list_view;
	private PopupOptions@ other_ids_tooltip;
	
	private array<EmitterIdData> other_emitter_names_sorted;
	private array<string> emitter_names;
	
	private const array<EmitterData@>@ selected_emitters;
	private int selected_emitters_count;
	
	private int selected_layer;
	
	private void create_ui()
	{
		UI@ ui = script.ui;
		Style@ style = ui.style;
		
		@window = Window(ui, 'None selected', false, true);
		window.name = 'EmitterToolWindow';
		window.x = 20;
		window.y = 20;
		window.drag_anywhere = true;
		window.title_bar_height = 32;
		window.title_label.set_padding(style.spacing, 2);
		
		// Emitter Id
		
		@emitter_id_select = Select(ui, '- Multiple Ids -');
		emitter_id_select.width = 260;
		emitter_id_select.allow_custom_value = true;
		@emitter_id_select.tooltip = PopupOptions(ui, 'Emitter Id', false, PopupPosition::Right);
		window.add_child(emitter_id_select);
		
		for(uint i = 0; i < Emitters::MainEmitterNames.length(); i++)
		{
			const string name = Emitters::MainEmitterNames[i];
			emitter_id_select.add_value(name, name);
			
			const int id = Emitters::MainEmitterIds[i];
			
			if(uint(id) >= emitter_names.length())
			{
				emitter_names.resize(id + 1);
			}
			
			emitter_names[id] = name;
		}
		
		for(uint i = 0; i < Emitters::OtherEmitterNames.length(); i++)
		{
			const string name = Emitters::OtherEmitterNames[i];
			const int id = Emitters::OtherEmitterIds[i];
			
			if(uint(id) >= emitter_names.length())
			{
				emitter_names.resize(id + 1);
			}
			
			emitter_names[id] = name;
		}
		
		emitter_id_select.selected_value = get_emitter_name(tool.emitter_id);
		emitter_id_select.change.on(EventCallback(on_emitter_id_change));
		
		// Other Emitters
		
		@other_ids_button = SelectButton(ui, 'editor', 'emittericon');
		other_ids_button.icon.width  = Settings::IconSize;
		other_ids_button.icon.height = Settings::IconSize;
		other_ids_button.fit_to_contents();
		other_ids_button.x = emitter_id_select.x + emitter_id_select.width - other_ids_button.width;
		other_ids_button.y = emitter_id_select.y + emitter_id_select.height + style.spacing;
		@other_ids_button.tooltip = PopupOptions(ui, 'Other Emitters', false, PopupPosition::Right);
		other_ids_button.open.on(EventCallback(on_other_ids_open));
		window.add_child(other_ids_button);
		
		// Layer
		
		@layer_button = LayerButton(ui);
		layer_button.y = emitter_id_select.y + emitter_id_select.height + style.spacing;
		@layer_button.tooltip = PopupOptions(ui, 'Layer');
		layer_button.layer_select.set_selected_layer(tool.layer);
		layer_button.layer_select.set_selected_sub_layer(tool.sublayer);
		layer_button.layer_select.layer_select.on(EventCallback(on_layer_change));
		layer_button.layer_select.sub_layer_select.on(EventCallback(on_sublayer_change));
		window.add_child(layer_button);
		
		// Rotation
		
		@rotation_wheel = RotationWheel(ui);
		rotation_wheel.indicator_offset = -HALF_PI;
		rotation_wheel.allow_range = false;
		rotation_wheel.drag_relative = true;
		rotation_wheel.auto_tooltip = true;
		rotation_wheel.tooltip_prefix = 'Rotation: ';
		rotation_wheel.x = layer_button.x + layer_button.width + style.spacing;
		rotation_wheel.y = layer_button.y;
		rotation_wheel.degrees = tool.rotation;
		rotation_wheel.change.on(EventCallback(on_rotation_change));
		window.add_child(rotation_wheel);
		
		layer_button.height = rotation_wheel.height;
		
		// Particle Id Label
		
		@id_label = Label(ui, '999', true, font::ENVY_BOLD, 20);
		id_label.colour = multiply_alpha(ui.style.text_clr, 0.5);
		id_label.fit_to_contents();
		id_label.x = other_ids_button.x - style.spacing - id_label.width;
		id_label.y = rotation_wheel.y;
		id_label.text_align_h = TextAlign::Right;
		id_label.align_h = TextAlign::Right;
		id_label.auto_size = false;
		window.add_child(id_label);
		
		Label@ id_lbl = Label(ui, 'Id:', true, font::ENVY_BOLD, 20);
		id_lbl.colour = id_label.colour;
		id_lbl.fit_to_contents();
		id_lbl.x = id_label.x - id_lbl.width - style.spacing;
		id_lbl.y = id_label.y;
		window.add_child(id_lbl);
		
		// Emitter Id Label
		
		@emitter_id_label = Label(ui, '[99999]', true, font::ENVY_BOLD, 20);
		emitter_id_label.colour = id_label.colour;
		emitter_id_label.fit_to_contents();
		emitter_id_label.x = other_ids_button.x - style.spacing - emitter_id_label.width;
		emitter_id_label.y = id_label.y + id_label.height + style.spacing;
		emitter_id_label.text_align_h = TextAlign::Right;
		emitter_id_label.align_h = TextAlign::Right;
		emitter_id_label.auto_size = false;
		window.add_child(emitter_id_label);
		
		// Finish
		
		window.fit_to_contents(true);
		script.window_manager.register_element(window);
		ui.add_child(window);
	}
	
	void show(AdvToolScript@ script, EmitterTool@ tool)
	{
		if(@this.script == null)
		{
			@this.script = script;
			@this.tool = tool;
			
			create_ui();
		}
		
		selected_layer = script.layer;
		layer_button.layer_select.set_selected_layer(selected_layer);
		update_selection(null, 0);
		window.show();
	}
	
	void hide()
	{
		window.hide();
		
		selected_emitters_count = 0;
	}
	
	void update_selection(const array<EmitterData@>@ selected_emitters, const int selected_emitters_count)
	{
		@this.selected_emitters = selected_emitters;
		this.selected_emitters_count = selected_emitters_count;
		
		set_ui_events_enabled(false);
		
		if(selected_emitters_count == 0)
		{
			window.title = 'None selected';
			
			emitter_id_select.selected_value = get_emitter_name(tool.emitter_id);
			layer_button.layer_select.set_selected_layer(tool.layer);
			layer_button.layer_select.set_selected_sub_layer(tool.sublayer);
			rotation_wheel.degrees = tool.rotation;
			id_label.text = tool.emitter_id < 0 ? '-' : tool.emitter_id + '';
			emitter_id_label.text = '-';
		}
		else if(selected_emitters_count == 1)
		{
			EmitterData@ data = @selected_emitters[0];
			
			window.title = 'Emitter Properties';
			
			tool.emitter_id	= data.emitter_id;
			tool.layer			= data.layer;
			tool.sublayer		= data.sublayer;
			tool.rotation		= data.rotation;
			
			emitter_id_select.selected_value = get_emitter_name(tool.emitter_id);
			layer_button.layer_select.set_selected_layer(tool.layer);
			layer_button.layer_select.set_selected_sub_layer(tool.sublayer);
			rotation_wheel.degrees = tool.rotation;
			id_label.text = tool.emitter_id < 0 ? '-' : tool.emitter_id + '';
			emitter_id_label.text = '[' + selected_emitters[0].emitter.id() + ']';
			
			selected_layer = data.layer;
			script.select_layer(selected_layer);
		}
		else
		{
			window.title = 'Multiple Emtters';
			
			EmitterData@ data = @selected_emitters[0];
			
			int emitter_id = data.emitter_id;
			int layer = data.layer;
			int sublayer = data.sublayer;
			float rotation = data.rotation;
			bool same_rotation = true;
			
			for(int i = 1; i < selected_emitters_count; i++)
			{
				@data = @selected_emitters[i];
				
				if(emitter_id != data.emitter_id)
				{
					emitter_id = -1;
				}
				
				if(layer != data.layer)
				{
					layer = -1;
				}
				
				if(sublayer != data.sublayer)
				{
					sublayer = -1;
				}
				
				if(!approximately(rotation, data.rotation))
				{
					same_rotation = false;
				}
			}
			
			if(emitter_id != -1)
			{
				emitter_id_select.selected_value = get_emitter_name(emitter_id);
				id_label.text = emitter_id + '';
			}
			else
			{
				emitter_id_select.selected_index = -1;
				id_label.text = '-';
			}
			
			emitter_id_label.text = '-';
			
			if(layer != -1)
				layer_button.layer_select.set_selected_layer(layer);
			else
				layer_button.layer_select.select_layers_none(false, true);
			
			if(sublayer != -1)
				layer_button.layer_select.set_selected_sub_layer(sublayer);
			else
				layer_button.layer_select.select_sub_layers_none(false, true);
			
			rotation_wheel.degrees = same_rotation ? rotation : 0;
		}
		
		set_ui_events_enabled(true);
	}
	
	void update_selection()
	{
		update_selection(@selected_emitters, selected_emitters_count);
	}
	
	void update_rotation(const float rotation)
	{
		if(selected_emitters_count != 1)
			return;
		
		set_ui_events_enabled(false);
		rotation_wheel.degrees = rotation;
		set_ui_events_enabled(true);
	}
	
	void update_layer()
	{
		set_ui_events_enabled(false);
		
		if(selected_emitters_count == 0)
			return;
		
		if(selected_emitters_count == 1)
		{
			EmitterData@ data = @selected_emitters[0];
			tool.layer		= data.layer;
			tool.sublayer	= data.sublayer;
			layer_button.layer_select.set_selected_layer(tool.layer);
			layer_button.layer_select.set_selected_sub_layer(tool.sublayer);
		}
		else
		{
			EmitterData@ data = @selected_emitters[0];
			int layer = data.layer;
			int sublayer = data.sublayer;
			
			for(int i = 1; i < selected_emitters_count; i++)
			{
				@data = @selected_emitters[i];
				if(layer != data.layer)
				{
					layer = -1;
				}
				
				if(sublayer != data.sublayer)
				{
					sublayer = -1;
				}
			}
			
			if(layer != -1)
				layer_button.layer_select.set_selected_layer(layer);
			else
				layer_button.layer_select.select_layers_none(false, true);
			
			if(sublayer != -1)
				layer_button.layer_select.set_selected_sub_layer(sublayer);
			else
				layer_button.layer_select.select_sub_layers_none(false, true);
		}
		
		set_ui_events_enabled(true);
	}
	
	void update_selected_layer()
	{
		const int new_layer = script.layer;
		
		if(new_layer != selected_layer)
		{
			selected_layer = new_layer;
			layer_button.layer_select.set_selected_layer(selected_layer);
		}
	}
	
	private void set_ui_events_enabled(const bool enabled=true)
	{
		emitter_id_select.change.enabled = enabled;
		rotation_wheel.change.enabled = enabled;
		layer_button.layer_select.layer_select.enabled = enabled;
		layer_button.layer_select.sub_layer_select.enabled = enabled;
	}
	
	private void populate_other_ids()
	{
		if(@other_ids_list_view != null)
			return;
		
		other_ids_button.popup.position = PopupPosition::Right;
		
		@other_ids_list_view = other_ids_button.list_view;
		other_ids_list_view.scroll_amount = 75;
		other_ids_list_view.select.on(EventCallback(on_other_ids_select));
		
		other_emitter_names_sorted.resize(Emitters::OtherEmitterNames.length());
		
		for(uint i = 0; i < Emitters::OtherEmitterNames.length(); i++)
		{
			other_emitter_names_sorted[i].id = Emitters::OtherEmitterIds[i];
			other_emitter_names_sorted[i].name = Emitters::OtherEmitterNames[i];
		}
		
		other_emitter_names_sorted.sortAsc();
		
		for(uint i = 0; i < other_emitter_names_sorted.length(); i++)
		{
			ListViewItem@ item = other_ids_list_view.add_item(other_emitter_names_sorted[i].name, other_emitter_names_sorted[i].name);
		}
	}
	
	private string get_emitter_name(const int id)
	{
		const string name = id >= 0 && id < int(emitter_names.length())
			? emitter_names[id] : '';
		
		return name != ''
			? name
			: Emitters::MainEmitterNames[Emitters::MainEmitterIds[EmitterId::DustGround]];
	}
	
	private void update_emitter_id(const int id)
	{
		tool.emitter_id = id;
		
		for(int i = 0; i < selected_emitters_count; i++)
		{
			selected_emitters[i].update_emitter_id(id);
		}
		
		update_selection();
	}
	
	// //////////////////////////////////////////////////////////
	// Events
	// //////////////////////////////////////////////////////////
	
	private void on_emitter_id_change(EventInfo@ event)
	{
		if(emitter_id_select.selected_index == -1)
			return;
		
		update_emitter_id(Emitters::MainEmitterIds[emitter_id_select.selected_index]);
	}
	
	private void on_layer_change(EventInfo@ event)
	{
		const int value = layer_button.layer_select.get_selected_layer();
		
		if(value == -1)
			return;
		
		tool.layer = value;
		
		for(int i = 0; i < selected_emitters_count; i++)
		{
			selected_emitters[i].update_layer(tool.layer);
		}
		
		script.select_layer(value);
		update_selection();
	}
	
	private void on_sublayer_change(EventInfo@ event)
	{
		const int value = layer_button.layer_select.get_selected_sub_layer();
		
		if(value == -1)
			return;
		
		tool.sublayer = value;
		
		for(int i = 0; i < selected_emitters_count; i++)
		{
			selected_emitters[i].update_sublayer(tool.sublayer);
		}
		
		update_selection();
	}
	
	private void on_rotation_change(EventInfo@ event)
	{
		tool.rotation = rotation_wheel.degrees;
		
		for(int i = 0; i < selected_emitters_count; i++)
		{
			selected_emitters[i].update_rotation(tool.rotation);
		}
		
		update_selection();
	}
	
	private void on_other_ids_open(EventInfo@ event)
	{
		if(event.type != EventType::BEFORE_OPEN)
			return;
		
		populate_other_ids();
		
		other_ids_list_view.select.enabled = false;
		other_ids_list_view.select_none();
		
		const int other_id_index = Emitters::OtherEmitterIds.find(tool.emitter_id);
		
		if(other_id_index != -1)
		{
			other_ids_list_view.select_item(Emitters::OtherEmitterNames[other_id_index]);
		}
		
		other_ids_list_view.select.enabled = true;
	}
	
	private void on_other_ids_select(EventInfo@ event)
	{
		if(other_ids_list_view.selected_index == -1)
			return;
		
		other_ids_list_view.select.enabled = false;
		
		update_emitter_id(other_emitter_names_sorted[other_ids_list_view.selected_index].id);
		
		other_ids_list_view.select.enabled = true;
	}
	
}
