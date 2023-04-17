class ParticleEditorTool : Tool
{
	
	ParticleEditorTool(AdvToolScript@ script)
	{
		super(script, 'Particle');
		selectable = false;
	}
	
	// //////////////////////////////////////////////////////////
	// Tool Callbacks
	// //////////////////////////////////////////////////////////
	
	protected void on_select_impl()
	{
		// "Consume" the B key in while the particle editor is open
		// B is used to spawn particle bursts
		script.blocked_key.set(VK::B);
	}
	
	protected void on_deselect_impl()
	{
		script.blocked_key.clear();
	}
	
}
