modded class SCR_PlayerController
{
	void SetupTraitorLocal()
	{
		Rpc(Rpc_SetupTraitorLocal);
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void Rpc_SetupTraitorLocal()
	{
		ResourceName briefing = "{230422472078D379}worlds/arc/CommandosReforged/Prefabs/Traitor_Briefing.et";
		SCR_HintUIInfo customHint = SCR_HintUIInfo.CreateInfo("Sabotage everything without getting caught", "You are a Traitor!", 300, EHint.UNDEFINED, EFieldManualEntryId.NONE, false);
		SCR_HintManagerComponent hintManager = SCR_HintManagerComponent.GetInstance();
		if (hintManager)
			hintManager.Show(customHint, false, true);
		
		GetGame().SpawnEntityPrefabLocal(Resource.Load(briefing));
		GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.BriefingMapMenu);
		
		GC_CommandosManager.GetInstance().m_defectorPlayerId = GetGame().GetPlayerController().GetPlayerId();
	}
}