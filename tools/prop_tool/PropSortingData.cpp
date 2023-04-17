class PropSortingData
{
	
	prop@ prop;
	int is_inside;
	int scene_index;
	int layer_position;
	bool selected;
	const array<array<float>>@ outline;
	
	int opCmp(const PropSortingData &in other)
	{
		if(selected != other.selected)
			return selected ? 1 : -1;
		
		// Props that the mouse is inside of take priority over props that the mouse is close to
		
		if(is_inside != other.is_inside)
			return is_inside - other.is_inside;
		
		// Compare layers
		
		if(layer_position != other.layer_position)
			return layer_position - other.layer_position;
		
		if(prop.sub_layer() != other.prop.sub_layer())
			return prop.sub_layer() - other.prop.sub_layer();
		
		// Compare segments
		// 16 tiles * 48 pixels = 768
		
		const int cx = int(floor(int(prop.x()) / 768));
		const int ocx = int(floor(int(other.prop.x()) / 768));
		
		if(cx != ocx)
			return cx - ocx;
		
		const int cy = int(floor(prop.y() / 768));
		const int ocy = int(floor(other.prop.y() / 768));
		
		if(cy != ocy)
			return cy - ocy;
		
		// Finally compare the index the props were returned in
		
		return scene_index - other.scene_index;
	}
	
}
