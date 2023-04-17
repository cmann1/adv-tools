class EmitterIdData
{
	
	int id;
	string name;
	
	int opCmp(const EmitterIdData &in other)
	{
		if(name < other.name)
			return -1;
        
		if(name > other.name)
			return 1;
        
		return 0;
	}
	
	bool opEquals(const EmitterIdData &in other)
	{
		return name == other.name;
	}
	
}