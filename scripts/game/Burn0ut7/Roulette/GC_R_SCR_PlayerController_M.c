
//Roulette Commands

modded class SCR_PlayerController : PlayerController
{
	void SendCommandServer(string command, string data)
	{
		Rpc(RPC_SendCommandServer, command, data);
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RPC_SendCommandServer(string command, string data)
	{
		GC_R_Manager manager = GC_R_Manager.GetInstance();
		if(!manager)
			return;

		manager.InvokeCommand(this, command, data);
	}
	
	void SendChatMsg(string message)
	{
		Rpc(RPC_SendChatMsg, message);
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RPC_SendChatMsg(string message)
	{
		SCR_ChatComponent cc = SCR_ChatComponent.Cast(FindComponent(SCR_ChatComponent));
	    if (!cc)
	        return;
	    cc.ShowMessage(message);
	}
}
