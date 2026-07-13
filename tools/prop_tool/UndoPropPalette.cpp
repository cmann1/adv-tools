class UndoPropPalette : callback_base
{
	
	array<UndoPropPaletteData> props;
	
	UndoPropPalette(array<PropData@>@ selected_props, int selected_props_count)
	{
		props.resize(selected_props_count);
		for(int i = 0; i < selected_props_count; i++)
		{
			UndoPropPaletteData@ data = @props[i];
			@data.prop = selected_props[i].prop;
			data.start_palette = data.end_palette = data.prop.palette();
		}
	}
	
	void undo() {
		for(uint i = 0; i < props.length; i++)
		{
			UndoPropPaletteData@ data = @props[i];
			data.prop.palette(data.start_palette);
		}
	}
	
	void redo() {
		for(uint i = 0; i < props.length; i++)
		{
			UndoPropPaletteData@ data = @props[i];
			data.prop.palette(data.end_palette);
		}
	}
	
}

class UndoPropPaletteData
{
	prop@ prop;
	uint start_palette;
	uint end_palette;
}
