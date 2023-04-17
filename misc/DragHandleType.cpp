enum DragHandleType
{
	
	None,
	Right,
	BottomRight,
	Bottom,
	BottomLeft,
	Left,
	TopLeft,
	Top,
	TopRight,
	Centre,
	Rotate,
	
	Start,
	End,
	Segment,
	
}
namespace DragHandle
{
	
	DragHandleType opposite(const DragHandleType handle)
	{
		switch(handle)
		{
			case DragHandleType::Right:			return DragHandleType::Left;
			case DragHandleType::BottomRight:	return DragHandleType::TopLeft;
			case DragHandleType::Bottom:		return DragHandleType::Top;
			case DragHandleType::BottomLeft:	return DragHandleType::TopRight;
			case DragHandleType::Left:			return DragHandleType::Right;
			case DragHandleType::TopLeft:		return DragHandleType::BottomRight;
			case DragHandleType::Top:			return DragHandleType::Bottom;
			case DragHandleType::TopRight:		return DragHandleType::BottomLeft;
		}
		
		return DragHandleType::None;
	}
	
}
