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
	
	protected ref array<ref GC_CommandosObj> m_activeObjectives = {};

	int m_traitorPlayerId;

	static GC_CommandosManager GetInstance()
	{
		return GC_CommandosManager.Cast(GetGame().GetGameMode().FindComponent(GC_CommandosManager));
	}
	
	GC_CommandosObj GetObjective(typename type)
	{
		foreach(GC_CommandosObj objective : m_activeObjectives)
		{
			if (objective.IsInherited(type))
				return objective;
		}

		foreach(GC_CommandosObj objective : m_objectives)
		{
			if (objective.IsInherited(type))
				return objective;
		}
		
		return null;
	}
	
	void OnObjectiveCompleted(GC_CommandosObj objective)
	{
		m_activeObjectives.RemoveItem(objective);
		
		if(m_activeObjectives.Count() <= 0)
			SendHintAll("US Victory!", "All objectives completed!")
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
		
		foreach(GC_CommandosObj objective : m_objectives)
		{
			objective.Setup();
		}
		
		while(!m_objectives.IsEmpty() && m_activeObjectives.Count() < m_minObjectiveCount)
		{
			ref GC_CommandosObj objective = m_objectives.GetRandomElement();
			if(!objective.IsPickable())
				continue;
			
			m_objectives.RemoveItem(objective);
			m_activeObjectives.Insert(objective);

			objective.Activate();
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
		
		foreach(GC_CommandosObj objective : m_activeObjectives)
		{
			objective.OnBriefing();
		}

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
		pc.TILW_SendHintToPlayer("You are a Traitor!", "Sabotage everything! No spawn killing.", 120);
		
		foreach(GC_CommandosObj objective : m_activeObjectives)
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
	protected string name;
	protected string completedString = "The %1 has been neutralized!";
	protected IEntity objective;
	protected PS_ManualMarker marker;
	protected bool isActive = false;
	
	protected string markerPrefab = "{2E08E7B7914EB585}worlds/arc/CommandosReforged/Prefabs/Objective_Marker.et";

	void Setup()
	{
		if(objective)
		{
			EntitySpawnParams params = new EntitySpawnParams();
			params.Transform[3] = objective.GetOrigin();
			marker = PS_ManualMarker.Cast(GetGame().SpawnEntityPrefab(Resource.Load(markerPrefab), GetGame().GetWorld(), params));
			marker.SetDescription(name);
			marker.SetVisibleForEmptyFaction(true);
			
			SCR_FactionManager fm = SCR_FactionManager.Cast(GetGame().GetFactionManager());
			Faction faction = fm.GetFactionByKey("RHS_AFRF");
			marker.SetVisibleForFaction(faction, true);
		}
	}
	
	bool IsPickable(){return true;}
	
	void Activate()
	{
		IEntity child = objective.GetChildren();

		if(objective)
		{
			SCR_FactionManager fm = SCR_FactionManager.Cast(GetGame().GetFactionManager());
			Faction faction = fm.GetFactionByKey("RHS_USAF");
			marker.SetVisibleForFaction(faction, false);
			
			EntitySpawnParams params = new EntitySpawnParams();
			params.Transform[3] = objective.GetOrigin();
			
			PS_ManualMarker marker2 = PS_ManualMarker.Cast(GetGame().SpawnEntityPrefab(Resource.Load(markerPrefab), GetGame().GetWorld(), params));
			marker2.SetDescription(name);
			marker2.SetColor(Color.Red);
		}
		
		isActive = true;
	}
	
	void OnBriefing();
	
	void OnGameStart();

	void Complete()
	{
		if(!Replication.IsServer())
			return;
		
		if(!isActive)
			return;

		GC_CommandosManager cm = GC_CommandosManager.GetInstance();
		
		string description = string.Format(completedString, name);
		GetGame().GetCallqueue().CallLater(cm.SendHintAll,100,false,"Objective Completed", description);
		
		cm.OnObjectiveCompleted(this);
		isActive = false;
	}

	string GetName()
	{
		return name;
	}

	bool IsActive()
	{
		return isActive;
	}
	
	void SetObjective(IEntity entity)
	{
		objective = entity;
	}
	
	void RegisterObjective(IEntity entity);
}

[BaseContainerProps()]
class GC_CommandosPS : GC_CommandosObj
{
	override void Setup()
	{
		name = "Power Station";
		super.Setup();
	}
}

[BaseContainerProps()]
class GC_CommandosEV : GC_CommandosDestroyed
{
	override void Setup()
	{
		name = "Experimental Vehicle";
		super.Setup();
	}
}

[BaseContainerProps()]
class GC_CommandosBH : GC_CommandosDestroyed
{
	override void Setup()
	{
		name = "Black Hawk";
		super.Setup();
	}
}

[BaseContainerProps()]
class GC_CommandosMi24 : GC_CommandosDestroyed
{
	override void Setup()
	{
		name = "Mi-24s";
		completedString = "The %1 have been neutralized!";
		super.Setup();
	}
}

[BaseContainerProps()]
class GC_CommandosDestroyed : GC_CommandosObj
{
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
		
		if(objCount <= 0)
			Complete();
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
	
	[Attribute(uiwidget: UIWidgets.Auto, desc: "Allowed user faction keys. If empty, any.")]
	protected ref array<string> m_factionKeys;
	
	[Attribute(uiwidget: UIWidgets.Auto, desc: "Allow the defector to complete.")]
	protected bool m_defectorObjective;
	
	protected bool m_completed = false;
	protected GC_CommandosManager cm;
	
	override void Init(IEntity pOwnerEntity, GenericComponent pManagerComponent)
	{
		cm = GC_CommandosManager.GetInstance();
	}
	
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		GC_CommandosObj objective = GC_CommandosManager.GetInstance().GetObjective(m_objectiveName.ToType());
		if(objective)
			objective.Complete();
		
		m_completed = true;
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
		if(m_completed)
			return false;
		
		if(m_defectorObjective)
		{
			if(cm.m_traitorPlayerId)
				return true;
			
			return false;
		}
		
		if (!m_factionKeys.IsEmpty())
		{
			SCR_ChimeraCharacter cc = SCR_ChimeraCharacter.Cast(user);
			if (!cc || !m_factionKeys.Contains(cc.GetFactionKey()))
				return false;
		}
		
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
		GC_CommandosObj objective = GC_CommandosManager.GetInstance().GetObjective(m_objectiveName.ToType());
		if(!objective)
			return;
		
		if(m_setMainObjective)
			objective.SetObjective(owner);
		
		if(GC_CommandosDestroyed.Cast(objective))
			objective.RegisterObjective(owner);
	}
}