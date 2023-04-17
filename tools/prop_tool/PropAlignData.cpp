class PropAlignData
{
	
	PropData@ data;
	float x;
	
	int opCmp(const PropAlignData &in other)
	{
		return x == other.x ? 0 : x < other.x ? -1 : 1;
	}
	
}