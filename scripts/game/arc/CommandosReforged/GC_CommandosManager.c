[ComponentEditorProps(category: "GameScripted/GameMode/Components", description: "Commandos")]
class GC_CommandosManagerClass : SCR_BaseGameModeComponentClass
{
}

class GC_CommandosManager : SCR_BaseGameModeComponent
{
	[Attribute(defvalue: "5", UIWidgets.Auto)]
	protected int m_minObjectiveCount;
	
	[Attribute(defvalue: "7", UIWidgets.Auto)]
	protected int m_maxObjectiveCount;
	
	[Attribute(defvalue: "", UIWidgets.Object)]
	protected ref array<ref GC_CommandosObj> m_objectives;

	[Attribute(defvalue: "", UIWidgets.Object)]
	protected ref array<ref GC_CommandosObjWeight> m_weightedObjectives;
	
	int m_defectorPlayerId;
	bool m_isDefectorObjSelected = false;

	static GC_CommandosManager GetInstance()
	{
		return GC_CommandosManager.Cast(GetGame().GetGameMode().FindComponent(GC_CommandosManager));
	}
	
	GC_CommandosObj GetObjective(string name)
	{
		foreach(GC_CommandosObj objective : m_objectives)
		{
			if (objective.GetName() == name)
				return objective;
		}
		
		return null;
	}
	
	void CheckEndConditions()
	{
		int count = 0;
		foreach(GC_CommandosObj objective : m_objectives)
		{
			if(objective.IsActive())
				count++;
		}
		
		if(count <= 0)
			SendHintAll("US Victory!", "All objectives completed!")
	}
	
	void OnObjectiveCompleted(GC_CommandosObj objective)
	{
		CheckEndConditions();
	}
	
	override void OnPlayerConnected(int playerId)
	{
		if(playerId == m_defectorPlayerId)
			SetupTraitor(m_defectorPlayerId);
	}
	
	override protected void OnPostInit(IEntity owner)
	{
		SetEventMask(owner, EntityEvent.INIT);
	}
	
	override protected void EOnInit(IEntity owner)
	{
		if(!Replication.IsServer())
			return;
		
		if(!GetGame().InPlayMode())
			return;
		
		array<ref GC_CommandosObj> objectivePool = {};
		foreach(GC_CommandosObj objective : m_objectives)
		{
			objective.Setup();
			objectivePool.Insert(objective)
		}
		
		int objCount = 0;
		while(!objectivePool.IsEmpty() && objCount < m_minObjectiveCount)
		{
			ref GC_CommandosObj objective = objectivePool.GetRandomElement();
			objectivePool.RemoveItem(objective);
			if(!objective.IsPickable())
				continue;
			
			objective.Activate();
			objCount++;
		}
		
		//Weighted Objectives
		/*
		array<ref GC_CommandosObj> objPoolWeighted = {};
		foreach(GC_CommandosObj objective : m_weightedObjectives)
		{
			objective.Setup();
			objPoolWeighted.Insert(objective)
		}
		
		while(!objPoolWeighted.IsEmpty() && objCount <= m_maxObjectiveCount)
		{
			ref GC_CommandosObj objective = objPoolWeighted.GetRandomElement();
			objectivePool.RemoveItem(objective);
			if(!objective.IsPickable())
				continue;
			
			objective.Activate();
			objCount++;
		}
		*/
	}
	
	override void OnGameStateChanged(SCR_EGameModeState state)
	{
		if(!Replication.IsServer())
			return;
		
		switch(state)
		{
			case SCR_EGameModeState.BRIEFING:
				BriefingStart();
				break;
			
			case SCR_EGameModeState.GAME:
				GameStart();
				break;
		}
	}
	
	protected void BriefingStart()
	{
		Print("GRAY.BriefingStart");
		
		foreach(GC_CommandosObj objective : m_objectives)
		{
			objective.OnBriefing();
		}
		
		//FindDefector();
		/*
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
		*/
	}
	
	protected int FindDefector(array<string> blacklist)
	{
		Print("GRAY.FindDefector");
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
			
			Print("GRAY.FindDefector 2");
			RplId playableId = playableManager.GetPlayableByPlayer(playerId);
			Print("GRAY.FindDefector playableId = " + playableId);
	
			PS_PlayableContainer pc = playableManager.GetPlayableById(playableId);
			if(blacklist.Contains(pc.GetName()))
				continue;
			Print("GRAY.FindDefector = " + pc.GetName());
			return playerId;
		}
		
		return null;
	}
	
	protected void SetupTraitor(int playerId)
	{
		Print("GRAY.SetupTraitor = " + SCR_PlayerNamesFilterCache.GetInstance().GetPlayerDisplayName(playerId));
		m_defectorPlayerId = playerId;

		SCR_PlayerController pc = SCR_PlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
		pc.SetupTraitorLocal();
	}
	
	protected void GameStart()
	{
		//SCR_PlayerController pc = SCR_PlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(m_defectorPlayerId));
		//pc.TILW_SendHintToPlayer("You are a Traitor!", "Sabotage everything! No spawn killing.", 120);
		
		foreach(GC_CommandosObj objective : m_objectives)
		{
			objective.OnGameStart();
		}
	}
	
	void SendHintAll(string title, string description)
	{
		array<int> players = {};
		GetGame().GetPlayerManager().GetAllPlayers(players);
		foreach(int player : players)
		{
			SCR_PlayerController pc = SCR_PlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(player));
			if(!pc)
				continue;
			
			pc.TILW_SendHintToPlayer(title, description, 15);
		}
	}
	
	void DoBark(RplId rplId)
	{
		Rpc(RPC_DoBark, rplId);
		
		if(RplSession.Mode() != RplMode.Dedicated)
			RPC_DoBark(rplId);
	}
	
	[RplRpc(RplChannel.Unreliable, RplRcver.Broadcast)]
	protected void RPC_DoBark(RplId rplId)
	{
		IEntity player = IEntity.Cast(Replication.FindItem(rplId));
		if(!player)
			return;
		
		SoundComponent soundComp = SoundComponent.Cast(player.FindComponent(SoundComponent));
		if (!soundComp)
			return;

		soundComp.SoundEvent("SOUND_DOGBARK");
	}
}

[BaseContainerProps()]
class GC_CommandosObj
{
	[Attribute(defvalue: "", UIWidgets.Auto)]
	protected string m_name;

	[Attribute(defvalue: "The %1 has been neutralized!", UIWidgets.EditBoxMultiline)]
	protected string m_objectiveCompletedMsg;
	
	[Attribute(defvalue: "Destroy the objective using C4", UIWidgets.EditBoxMultiline)]
	protected string m_briefingDescription;
	
	[Attribute(defvalue: "{2E08E7B7914EB585}worlds/arc/CommandosReforged/Prefabs/Objective_Marker.et", UIWidgets.ResourceNamePicker)]
	protected ResourceName m_markerPrefab;
	
	protected bool m_isActive = false;
	
	protected ref array<PS_ManualMarker> m_markers = {};

	void Setup()
	{
		
	}
	
	bool IsPickable()
	{
		if(m_isActive)
			return false;
		
		return true;
	}
	
	void Activate()
	{
		foreach(PS_ManualMarker marker : m_markers)
		{
			SCR_FactionManager fm = SCR_FactionManager.Cast(GetGame().GetFactionManager());
			Faction faction = fm.GetFactionByKey("RHS_USAF");
			marker.SetVisibleForFaction(faction, false);
			
			EntitySpawnParams params = new EntitySpawnParams();
			params.Transform[3] = marker.GetOrigin();
			
			PS_ManualMarker marker2 = PS_ManualMarker.Cast(GetGame().SpawnEntityPrefab(Resource.Load(m_markerPrefab), GetGame().GetWorld(), params));
			marker2.SetDescription(m_name);
			marker2.SetColor(Color.Red);
		}
		
		m_isActive = true;
	}
	
	void OnBriefing();
	
	void OnGameStart();

	void Complete()
	{
		if(!Replication.IsServer())
			return;
		
		if(!m_isActive)
			return;

		GC_CommandosManager cm = GC_CommandosManager.GetInstance();
		
		string description = string.Format(m_objectiveCompletedMsg, m_name);
		GetGame().GetCallqueue().CallLater(cm.SendHintAll,100,false,"Objective Completed", description);
		
		cm.OnObjectiveCompleted(this);
		
		m_isActive = false;
	}

	string GetName()
	{
		return m_name;
	}

	bool IsActive()
	{
		return m_isActive;
	}
	
	void RegisterMarker(IEntity entity)
	{
		EntitySpawnParams params = new EntitySpawnParams();
		params.Transform[3] = entity.GetOrigin();
		PS_ManualMarker marker = PS_ManualMarker.Cast(GetGame().SpawnEntityPrefab(Resource.Load(m_markerPrefab), GetGame().GetWorld(), params));
		marker.SetDescription(m_name);
		marker.SetVisibleForEmptyFaction(true);
		
		SCR_FactionManager fm = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		Faction faction = fm.GetFactionByKey("RHS_AFRF");
		marker.SetVisibleForFaction(faction, true);
		
		m_markers.Insert(marker);
	}
	
	void RegisterObjective(IEntity entity);
}

[BaseContainerProps()]
class GC_CommandosObjWeight : GC_CommandosObj
{
	[Attribute(defvalue: ".5", UIWidgets.Auto, desc: "0-1 Chance of being selected. 1 = 100%, 0 = 0%")]
	protected float m_chance;
	
	override bool IsPickable()
	{
		float random = Math.RandomFloat01();
		if(random > m_chance)
			return false;
		
		return super.IsPickable();
	}
}

[BaseContainerProps()]
class GC_CommandosDefector : GC_CommandosObjWeight
{
	[Attribute(defvalue: "", UIWidgets.Auto, desc: "Name of slots to not select")]
	protected array<string> m_blacklist;
	
	override bool IsPickable()
	{
		GC_CommandosManager cm = GC_CommandosManager.GetInstance();
		if(cm.m_isDefectorObjSelected)
			return false;
		
		return super.IsPickable();
	}
	
	override void Activate()
	{
		super.Activate();
		
		GC_CommandosManager cm = GC_CommandosManager.GetInstance();
		cm.m_isDefectorObjSelected = true;
	}
}

[BaseContainerProps()]
class GC_CommandosShootHeli : GC_CommandosDefector
{

}

[BaseContainerProps()]
class GC_CommandosGatherIntel : GC_CommandosDefector
{

}

[BaseContainerProps()]
class GC_CommandosDestroyed : GC_CommandosObj
{
	[Attribute(defvalue: "", UIWidgets.Auto)]
	protected bool m_destroyAll;
	
	protected int objCount = 0;
	protected ref array<IEntity> entities = {};
	
	override void Activate()
	{
		super.Activate();

		foreach(IEntity entity : entities)
		{
			SCR_DamageManagerComponent dmc = SCR_DamageManagerComponent.Cast(entity.FindComponent(SCR_DamageManagerComponent));
			if(!dmc)
				continue;
			
			SCR_HitZone hz = SCR_HitZone.Cast(dmc.GetDefaultHitZone());
			hz.GetOnDamageStateChanged().Insert(OnDamageStateChangedHZ);
			objCount++;
		}
	}
	
	void OnDamageStateChangedHZ(SCR_HitZone hitZone)
	{
		EDamageState state = hitZone.GetDamageState();
		if (state == EDamageState.DESTROYED || state == EDamageState.INTERMEDIARY)
		{
			hitZone.GetOnDamageStateChanged().Remove(OnDamageStateChangedHZ);
			objCount--;
		}
		
		if(m_destroyAll)
		{
			if(objCount <= 0)
				Complete();
		}
		else
		{
			Complete();
		}
	}
	
	override void RegisterObjective(IEntity entity)
	{
		entities.Insert(entity);
	}
}

class GC_CommandosInteraction : ScriptedUserAction
{
	[Attribute(uiwidget: UIWidgets.Auto, desc: "The name of the objective class to complete.")]
	protected string m_objectiveName;
	
	[Attribute(uiwidget: UIWidgets.Auto, desc: "Allowed user faction keys.")]
	protected ref array<string> m_factionKeys;
	
	[Attribute(uiwidget: UIWidgets.Auto, desc: "Allow the defector to complete.")]
	protected bool m_defectorObjective;
	
	protected GC_CommandosManager cm;
	
	override void Init(IEntity pOwnerEntity, GenericComponent pManagerComponent)
	{
		cm = GC_CommandosManager.GetInstance();
	}
	
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		GC_CommandosObj objective = cm.GetObjective(m_objectiveName);
		if(objective)
			objective.Complete();
	}
	
	override bool CanBePerformedScript(IEntity user)
	{
		return IsAvailable(user);
	}
	
	override bool CanBeShownScript(IEntity user)
	{
		return IsAvailable(user);
	}
	
	bool IsAvailable(IEntity user)
	{
		if(!cm.GetObjective(m_objectiveName).IsActive())
			return false;
		
		if(m_defectorObjective)
		{
			if(cm.m_defectorPlayerId)
				return true;
		}
		
		SCR_ChimeraCharacter cc = SCR_ChimeraCharacter.Cast(user);
		if (!cc || !m_factionKeys.Contains(cc.GetFactionKey()))
			return false;
		
		return true;
	}
	
	override bool HasLocalEffectOnlyScript()
	{
		return false;
	}
	
	override bool CanBroadcastScript()
	{
		return true;
	}
}

[ComponentEditorProps(category: "GameScripted/GameMode/Components", description: "Commandos")]
class GC_CommandosObjRegisterClass : ScriptComponentClass
{
}

class GC_CommandosObjRegister : ScriptComponent
{
	[Attribute(uiwidget: UIWidgets.Auto, desc: "The name of the objective class to affect")]
	protected string m_objectiveName;
	
	[Attribute(uiwidget: UIWidgets.Auto, desc: "Spawn marker here")]
	protected bool m_registerMarker;
	
	override protected void OnPostInit(IEntity owner)
	{
		GC_CommandosObj objective = GC_CommandosManager.GetInstance().GetObjective(m_objectiveName);
		if(!objective)
			return;
		
		if(m_registerMarker)
			objective.RegisterMarker(owner);
		
		if(GC_CommandosDestroyed.Cast(objective))
			objective.RegisterObjective(owner);
	}
}