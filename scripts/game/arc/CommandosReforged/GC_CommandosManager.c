[ComponentEditorProps(category: "GameScripted/GameMode/Components", description: "Commandos")]
class GC_CommandosManagerClass : SCR_BaseGameModeComponentClass
{
}

class GC_CommandosManager : SCR_BaseGameModeComponent
{
	[Attribute(defvalue: "5", UIWidgets.Auto)]
	protected int m_minObjectiveCount;
	
	[Attribute(defvalue: "", UIWidgets.Object)]
	protected ref array<ref GC_CommandosObj> m_objectives;

	int m_traitorPlayerId;

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
		if(playerId == m_traitorPlayerId)
			SetupTraitor(m_traitorPlayerId);
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
			if(!objective.IsPickable())
				continue;
			
			objective.Activate();
			objectivePool.RemoveItem(objective);
			objCount++;
		}
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
	
	void BriefingStart()
	{
		Print("GRAY.BriefingStart");
		
		foreach(GC_CommandosObj objective : m_objectives)
		{
			objective.OnBriefing();
		}
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
	
	void SetupTraitor(int playerId)
	{
		Print("GRAY.SetupTraitor = " + SCR_PlayerNamesFilterCache.GetInstance().GetPlayerDisplayName(playerId));
		m_traitorPlayerId = playerId;

		SCR_PlayerController pc = SCR_PlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
		pc.SetupTraitorLocal();
	}
	
	void GameStart()
	{
		//SCR_PlayerController pc = SCR_PlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(m_traitorPlayerId));
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
	
	//[RplProp]
	protected bool m_isActive;
	
	protected IEntity m_objective;
	protected PS_ManualMarker m_marker;

	void Setup()
	{
		m_isActive = false;
		
		if(m_objective)
		{
			EntitySpawnParams params = new EntitySpawnParams();
			params.Transform[3] = m_objective.GetOrigin();
			m_marker = PS_ManualMarker.Cast(GetGame().SpawnEntityPrefab(Resource.Load(m_markerPrefab), GetGame().GetWorld(), params));
			m_marker.SetDescription(m_name);
			m_marker.SetVisibleForEmptyFaction(true);
			
			SCR_FactionManager fm = SCR_FactionManager.Cast(GetGame().GetFactionManager());
			Faction faction = fm.GetFactionByKey("RHS_AFRF");
			m_marker.SetVisibleForFaction(faction, true);
		}
	}
	
	bool IsPickable()
	{
		if(m_isActive)
			return false;
		
		return true;
	}
	
	void Activate()
	{
		if(m_objective)
		{
			SCR_FactionManager fm = SCR_FactionManager.Cast(GetGame().GetFactionManager());
			Faction faction = fm.GetFactionByKey("RHS_USAF");
			m_marker.SetVisibleForFaction(faction, false);
			
			EntitySpawnParams params = new EntitySpawnParams();
			params.Transform[3] = m_objective.GetOrigin();
			
			PS_ManualMarker marker2 = PS_ManualMarker.Cast(GetGame().SpawnEntityPrefab(Resource.Load(m_markerPrefab), GetGame().GetWorld(), params));
			marker2.SetDescription(m_name);
			marker2.SetColor(Color.Red);
		}
		
		m_isActive = true;
		//Replication.BumpMe();
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
		//Replication.BumpMe();
	}

	string GetName()
	{
		return m_name;
	}

	bool IsActive()
	{
		return m_isActive;
	}
	
	void SetObjective(IEntity entity)
	{
		m_objective = entity;
	}
	
	void RegisterObjective(IEntity entity);
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
			if(cm.m_traitorPlayerId)
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
	
	[Attribute(uiwidget: UIWidgets.Auto, desc: "Where the objective marker will spawn")]
	protected bool m_setMainObjective;
	
	override protected void OnPostInit(IEntity owner)
	{
		GC_CommandosObj objective = GC_CommandosManager.GetInstance().GetObjective(m_objectiveName);
		if(!objective)
			return;
		
		if(m_setMainObjective)
			objective.SetObjective(owner);
		
		if(GC_CommandosDestroyed.Cast(objective))
			objective.RegisterObjective(owner);
	}
}