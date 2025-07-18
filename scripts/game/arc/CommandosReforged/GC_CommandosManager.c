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

	int m_defectorPlayerId;
	
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
		//if(playerId == m_defectorPlayerId)
		//	SetupTraitor(m_defectorPlayerId);
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
			if(GC_CommandosObjWeight.Cast(objective))
				continue;
			
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
		
		//Weighted objectives
		objectivePool.Clear();
		foreach(GC_CommandosObj objective : m_objectives)
		{
			if(GC_CommandosObjWeight.Cast(objective))
				objectivePool.Insert(objective);
		}
		
		while(!objectivePool.IsEmpty() && objCount < m_maxObjectiveCount)
		{
			ref GC_CommandosObj objective = objectivePool.GetRandomElement();
			objectivePool.RemoveItem(objective);
			
			if(!objective.IsPickable())
				continue;
			
			objective.Activate();
			objCount++;
		}
	}
	
	override void OnGameStateChanged(SCR_EGameModeState state)
	{
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
	}

	protected void SetupTraitor(int playerId)
	{
		//Print("GRAY.SetupTraitor = " + SCR_PlayerNamesFilterCache.GetInstance().GetPlayerDisplayName(playerId));
		//m_defectorPlayerId = playerId;

		//SCR_PlayerController pc = SCR_PlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
		//pc.SetupTraitorLocal();
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

	void Setup();
	
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
			ActivateMarker(marker);
		}
		
		m_isActive = true;
	}
	
	void ActivateMarker(PS_ManualMarker marker)
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
		Print("Gray.Complete = " + this);
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
		
		if(m_isActive)
			ActivateMarker(marker);
	}
	
	void RegisterObjective(IEntity entity);
}

[BaseContainerProps()]
class GC_CommandosObjWeight : GC_CommandosObj
{
	[Attribute(defvalue: "1", UIWidgets.Auto, desc: "0-1 Chance of being selected. 1 = 100%, 0 = 0%")]
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
	protected ref array<string> m_blacklist;
	
	static int m_defectorId;
	static GC_CommandosDefector m_defectorObj;
	
	override bool IsPickable()
	{
		if(m_defectorObj)
			return false;
		
		return super.IsPickable();
	}
	
	override void Activate()
	{
		super.Activate();
		
		m_defectorObj = this;
	}
	
	override void OnBriefing()
	{
		if(!Replication.IsServer())
			return;
		
		if(!m_isActive)
			return;
		
		int id = FindDefector();
		if(!id)
			return;
		
		m_defectorId = id;
		
		GC_CommandosManager cm = GC_CommandosManager.GetInstance();
		cm.m_defectorPlayerId = m_defectorId;
		
		GC_CommandosPlayerController pc = GC_CommandosPlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(m_defectorId));
		if(!pc)
			return;
		
		pc.ActivateObjective(m_name);
	}
	
	int FindDefector()
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
			
			RplId playableId = playableManager.GetPlayableByPlayer(playerId);
			if(!playableId || !playableId.IsValid())
				continue;
			
			PS_PlayableContainer pc = playableManager.GetPlayableById(playableId);
			if(!pc)
				continue;
			
			Print("GRAY.FindDefector pc = " + pc);
			Print("GRAY.FindDefector name = " + pc.GetName());
			Print("GRAY.FindDefector m_blacklist = " + m_blacklist);
			if(m_blacklist.Contains(pc.GetName()))
				continue;
			
			Print("GRAY.FindDefector2");
			return playerId;
		}
		
		return null;
	}
	
	void ActivateLocal();
}

[BaseContainerProps()]
class GC_CommandosShootHeli : GC_CommandosDefector
{
	protected ref array<IEntity> m_objs = {};
	IEntity m_mi8;
	
	override void RegisterObjective(IEntity entity)
	{
		if(!Vehicle.Cast(entity))
			return;
		
		m_mi8 = entity;
	}
	
	override void Activate()
	{
		super.Activate();
		
		SCR_DamageManagerComponent dmc = SCR_DamageManagerComponent.Cast(m_mi8.FindComponent(SCR_DamageManagerComponent));
		if(!dmc)
			return;

		SCR_HitZone hz = SCR_HitZone.Cast(dmc.GetDefaultHitZone());
		hz.GetOnDamageStateChanged().Insert(OnDamageStateChangedHZ);
	}
	
	override void RegisterMarker(IEntity entity)
	{
		m_objs.Insert(entity);
	}
	
	override void ActivateLocal()
	{
		foreach(IEntity entity : m_objs)
		{
			EntitySpawnParams params = new EntitySpawnParams();
			params.Transform[3] = entity.GetOrigin();
			PS_ManualMarker marker = PS_ManualMarker.Cast(GetGame().SpawnEntityPrefab(Resource.Load(m_markerPrefab), GetGame().GetWorld(), params));
			marker.SetDescription("Igla Cache");
			marker.SetVisibleForEmptyFaction(true);
			marker.SetColor(Color.Red);
			
			SCR_FactionManager fm = SCR_FactionManager.Cast(GetGame().GetFactionManager());
			Faction faction = fm.GetFactionByKey("RHS_AFRF");
			marker.SetVisibleForFaction(faction, true);
		}
		
		GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.BriefingMapMenu);
	}

	void OnDamageStateChangedHZ(SCR_HitZone hitZone)
	{
		EDamageState state = hitZone.GetDamageState();
		if (state == EDamageState.DESTROYED || state == EDamageState.INTERMEDIARY)
		{
			hitZone.GetOnDamageStateChanged().Remove(OnDamageStateChangedHZ);
			Complete();
		}
	}
}

[BaseContainerProps()]
class GC_CommandosTracker : GC_CommandosDestroyed
{
	protected ref array<ref GC_CommandosTrackerData> m_trackers = {};
	
	override void Activate()
	{
		super.Activate();
		
		foreach(IEntity entity : entities)
		{
			RegisterTracker(entity);
		}
		
		GetGame().GetCallqueue().CallLater(UpdateTracker, 120000, true)
	}
	
	override void RegisterObjective(IEntity entity)
	{
		super.RegisterObjective(entity);
		
		if(m_isActive)
			RegisterTracker(entity);
	}
	
	void UpdateTracker()
	{
		foreach(GC_CommandosTrackerData tracker : m_trackers)
		{
			tracker.Update();
		}
	}
	
	void RegisterTracker(IEntity entity)
	{
		EntitySpawnParams params = new EntitySpawnParams();
		params.Transform[3] = entity.GetOrigin();
		PS_ManualMarker marker = PS_ManualMarker.Cast(GetGame().SpawnEntityPrefab(Resource.Load(m_markerPrefab), GetGame().GetWorld(), params));
		
		marker.SetDescription(m_name);
		marker.SetColor(Color.Violet);

		GC_CommandosTrackerData tracker = GC_CommandosTrackerData(entity, marker);
		m_trackers.Insert(tracker);
	}
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
			ActivateObjective(entity);
		}
	}
	
	void ActivateObjective(IEntity entity)
	{
		Print("GRAY.ActivateObjective = " + this);
		
		SCR_DamageManagerComponent dmc = SCR_DamageManagerComponent.Cast(entity.FindComponent(SCR_DamageManagerComponent));
		if(!dmc)
			return;
		
		SCR_HitZone hz = SCR_HitZone.Cast(dmc.GetDefaultHitZone());
		if(!hz)
			return;
		
		hz.GetOnDamageStateChanged().Insert(OnDamageStateChangedHZ);
		objCount++;
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
		
		if(m_isActive)
			ActivateObjective(entity);
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
			if(cm.m_defectorPlayerId == GetGame().GetPlayerController().GetPlayerId())
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
		
		objective.RegisterObjective(owner);
	}
}

class GC_CommandosTrackerData
{
	IEntity m_entity;
	PS_ManualMarker m_marker;
	
	void GC_CommandosTrackerData(IEntity entity, PS_ManualMarker marker)
	{
		m_entity = entity;
		m_marker = marker;
	}
	
	void Update()
	{
		m_marker.SetOrigin(m_entity.GetOrigin());
	}
}
