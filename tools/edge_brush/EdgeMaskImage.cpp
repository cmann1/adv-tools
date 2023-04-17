#include '../../../../lib/tiles/TileEdge.cpp';

namespace EdgeMaskImage { const string TYPE_NAME = 'EdgeMaskImage'; }

class EdgeMaskImage : Image
{
	
	uint8 edge_mask;
	
	private array<Image@> edge_masks(4);
	
	EdgeMaskImage(UI@ ui)
	{
		super(ui, SPRITE_SET, 'edgebrush_edge_mask_off');
		
		@edge_masks[0] = Image(ui, SPRITE_SET, 'edgebrush_edge_mask_edge', Settings::IconSize, Settings::IconSize);
		@edge_masks[1] = Image(ui, SPRITE_SET, 'edgebrush_edge_mask_edge', Settings::IconSize, Settings::IconSize);
		@edge_masks[2] = Image(ui, SPRITE_SET, 'edgebrush_edge_mask_edge', Settings::IconSize, Settings::IconSize);
		@edge_masks[3] = Image(ui, SPRITE_SET, 'edgebrush_edge_mask_edge', Settings::IconSize, Settings::IconSize);
		
		edge_masks[0].rotation = -90;
		edge_masks[1].rotation =  90;
		edge_masks[2].rotation = 180;
		edge_masks[3].rotation =   0;
	}
	
	void _queue_children_for_layout(ElementStack@ stack) override
	{
		for(TileEdge edge = Top; edge <= Right; edge++)
		{
			if(edge_mask & (1 << edge) != 0)
			{
				stack.push(edge_masks[edge]);
			}
		}
	}
	
	
}
