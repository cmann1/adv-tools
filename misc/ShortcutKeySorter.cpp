class ShortcutKeySorter
{
	
	Tool@ tool;
	int index;
	
	ShortcutKeySorter()
	{
		
	}
	
	/// Sort by priority then index
	int opCmp(const ShortcutKeySorter &in other)
	{
		if(tool.key.priority != other.tool.key.priority)
			return tool.key.priority - other.tool.key.priority;
		
		return index - other.index;
	}
	
}
