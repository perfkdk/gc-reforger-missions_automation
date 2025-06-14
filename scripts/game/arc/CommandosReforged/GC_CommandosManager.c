[ComponentEditorProps(category: "GameScripted/GameMode/Components", description: "Commandos")]
class GC_CommandosManagerClass : SCR_BaseGameModeComponentClass
{
}

class GC_CommandosManager : SCR_BaseGameModeComponent
{
	protected int m_traitorPlayerId;
	
	override void OnPlayerConnected(int playerId)
	{
		if(playerId == m_traitorPlayerId)
			SetupTraitor(m_traitorPlayerId);
	}
	
	override void OnGameStateChanged(SCR_EGameModeState state)
	{
		if(!Replication.IsServer())
			return;
		
		switch(state)
		{
			case SCR_EGameModeState.BRIEFING:
				BriefingStart(); break;
			
			case SCR_EGameModeState.GAME:
				GameStart(); break;
		}
	}
	
	void BriefingStart()
	{
		Print("GRAY.BriefingStart");
		PlayerManager pm = GetGame().GetPlayerManager();
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		
		array<int> players = {};
		pm.GetAllPlayers(players);
		while(!players.IsEmpty())
		{
			int playerId = players.GetRandomElement();
			players.RemoveItem(playerId);
			
			playableManager.GetPlayerFactionKey(playerId);
			string faction = playableManager.GetPlayerFactionKey(playerId);
			if(faction != "RHS_AFRF")
				continue;

			SetupTraitor(playerId);
			break;
		}
	}
	
	void SetupTraitor(int playerId)
	{
		Print("GRAY.SetupTraitor = " + SCR_PlayerNamesFilterCache.GetInstance().GetPlayerDisplayName(playerId));
		m_traitorPlayerId = playerId;
		
		SCR_PlayerController pc = SCR_PlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
		pc.SetupTraitorLocal();
	}
	
	void GameStart()
	{
		SCR_PlayerController pc = SCR_PlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(m_traitorPlayerId));
		pc.TILW_SendHintToPlayer("You are a Traitor!", "Sabotage everything! No spawn killing.", 120)
	}
}