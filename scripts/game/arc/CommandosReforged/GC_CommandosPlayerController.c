class GC_CommandosPlayerControllerClass : SCR_PlayerControllerClass
{
}

class GC_CommandosPlayerController : SCR_PlayerController
{
	
	void ActivateObjective(string name)
	{
		Rpc(RPC_ActivateObjective, name);
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RPC_ActivateObjective(string name)
	{
		GC_CommandosManager cm = GC_CommandosManager.GetInstance();
		GC_CommandosDefector obj = GC_CommandosDefector.Cast(cm.GetObjective(name));
		if(!obj)
			return;
		
		obj.ActivateLocal();
	}
}