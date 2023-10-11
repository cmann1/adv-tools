namespace Settings
{
	
	const string ConfigEmbedKey = 'adv_tools_settings';
	const string ConfigFile = 'advtools.ini';
	
	const float IconSize = 22;
	
	const float ToolbarIconSize = 60;
	const float UIFadeAlpha = 0.35;
	const float UIFadeSpeed = 4;
	
	const float CursorLineWidth			= 2;
	const uint CursorLineColour			= 0xaaffffff;
	const uint CursorFillColour			= 0x10ffffff;
	
	const float SelectRectLineWidth			= 3;
	const uint SelectRectFillColour			= 0x1144ff44;
	const uint SelectRectLineColour			= 0x55aaffaa;
	
	const float DefaultLineWidth			= 2;
	const uint DefaultLineColour			= 0x55ffffff;
	const uint DefaultFillColour			= 0x10ffffff;
	
	const float HoveredLineWidth			= 3;
	const uint HoveredLineColour			= 0xff44eeff;
	const uint HoveredFillColour			= 0x5544eeff;
	
	const float SelectedLineWidth			= 2;
	const uint SelectedLineColour			= 0xcc44eeff;
	const uint SelectedFillColour			= 0x3344eeff;
	
	const float PendingAddLineWidth			= 2;
	const uint PendingAddFillColour			= 0x3344ff44;
	const uint PendingAddLineColour			= 0xaa44ff44;
	
	const float PendingRemoveLineWidth		= 2;
	const uint PendingRemoveFillColour		= 0x33ff4444;
	const uint PendingRemoveLineColour		= 0xaaff4444;
	
	const float BoundingBoxLineWidth		= 3;
	const uint BoundingBoxColour			= 0x55ffffff;
	
	const float	RotationHandleOffset		= 18;
	
	const float	RotateHandleSize			= 5;
	const uint RotateHandleColour			= 0xaaffffff;
	
	const float	ScaleHandleSize				= 4;
	const uint RotateHandleHoveredColour	= 0xaaea9c3f;
	
	const float	SelectDiamondSize			= 3;
	
	const array<float> ScaleHandleOffsets = {
		// Right
		 1,  0,
		 1,  0,
		// Bottom Right
		 1,  1,
		 1,  1,
		// Bottom
		 0,  1,
		 0,  1,
		// Bottom Left
		-1,  1,
		-1,  1,
		// Left
		-1,  0,
		-1,  0,
		// Top Left
		-1, -1,
		-1, -1,
		// Top
		 0, -1,
		 0, -1,
		// Top Right
		 1, -1,
		 1, -1,
	};
	
	const array<int> RepeatKeys = {
		GVB::LeftArrow,
		GVB::RightArrow,
		GVB::UpArrow,
		GVB::DownArrow,
	};
	
	/// How long (frames) to pause after the initial key press before the key starts repeating
	const int KeyPressDelay = 25;
	/// While a key is pressed, this specifies the speed (in frames) at which it will trigger
	const int KeyRepeatPeriod = 2;
	
	const int TileChunkSize = 24;
	
	const bool EdgeBrushDebugTiming = false;
	
	/// Both priority and collision on
	const uint EdgeOnColour    = 0xff00ffff;
	/// Both priority and collision off
	const uint EdgeOffColour   = 0x88ff00ff;
	/// Priority on, collision off
	const uint EdgeVisibleColour = 0xffffff00;
	/// Priority off, collision on - In game has same effect as both on
	const uint EdgeInvisibleColour = 0xff55ff22;
	
	const float EdgeMarkerLineWidth = 3;
	const float EdgeMarkerRadius = 9;
	const uint EdgeArrowMarkerColour = 0x55ffffff;
	
	const float TilePickerLineWidth = 2;
	const uint TilePickerLineColour = 0xcc44eeff;
	const uint TilePickerFillColour = 0x3344eeff;
	
}
