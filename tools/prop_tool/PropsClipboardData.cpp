class PropsClipboardData
{
	
	[text] float x;
	[text] float y;
	[text] float x1;
	[text] float y1;
	[text] float x2;
	[text] float y2;
	[text] int layer;
	[text] array<PropClipboardData> props;
	
}

class PropClipboardData
{
	
	[text] uint prop_set;
	[text] uint prop_group;
	[text] uint prop_index;
	[text] uint palette;
	[text] uint layer;
	[text] uint sub_layer;
	[text] float x;
	[text] float y;
	[text] float rotation;
	[text] float scale_x;
	[text] float scale_y;
	
}