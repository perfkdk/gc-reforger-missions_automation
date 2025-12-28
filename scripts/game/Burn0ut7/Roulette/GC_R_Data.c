[BaseContainerProps()]
class GC_R_BaseScenario : Managed
{
	void Intialize();
	void Reroll();
}

[BaseContainerProps(), BaseContainerCustomTitleField("Attack and Defend")]
class GC_R_AttackDefend: GC_R_BaseScenario
{
	[Attribute(defvalue: "", uiwidget: UIWidgets.Auto, desc: "Name of scenario refereced in the briefing")]
    protected string m_scenarioName;
	
	[Attribute(defvalue: "50", uiwidget: UIWidgets.Auto, desc: "Size of nearby building search", category: "Objective")]
    protected float m_buildingSearchSize;
	
	[Attribute(defvalue: "10", uiwidget: UIWidgets.Auto, desc: "Amount of buildings needed in Building Search Size", category: "Objective")]
    protected int m_minbuildingCount;
	
	[Attribute(defvalue: "", uiwidget: UIWidgets.ResourceNamePicker, desc: "Blacklist building prefab or kind", category: "Objective")]
    protected ref array<ResourceName> m_buildingBlackList;
	
	[Attribute(defvalue: "{FE585088FB849703}worlds/Burn0ut7/Roulette/Prefabs/RouletteCaptureArea.et", uiwidget: UIWidgets.Auto, desc: "Prefab to spawn for the objective", category: "Objective")]
    protected ResourceName m_objectivePrefab;
	
	[Attribute(defvalue: "", uiwidget: UIWidgets.Auto, desc: "Blacklist building of same name", category: "Objective")]
    protected ref array<string> m_buildingBlackListWords;
	
	[Attribute(defvalue: "1000", uiwidget: UIWidgets.Auto, desc: "AO width size in meters", category: "AO")]
    protected int m_width;
	
	[Attribute(defvalue: "2000", uiwidget: UIWidgets.Auto, desc: "Min size of AO in meters", category: "AO")]
    protected int m_minAOSize;
	
	[Attribute(defvalue: "5000", uiwidget: UIWidgets.Auto, desc: "Max size of AO in meters", category: "AO")]
    protected int m_maxAOSize;
	
	protected int m_searchAttempts;
	
	protected GC_R_Manager m_manager;
	protected ref GC_R_ObjAttackDefend m_objective;
	protected ref GC_R_SpawnAttacker m_attackerSpawn;
	protected ref GC_R_SpawnDefender m_defenderSpawn;
	
	ref array<ref GC_R_Team> m_teams = {};
	
	protected bool m_searching = false;
	
	override void Intialize()
	{
		m_manager = GC_R_Manager.GetInstance();

		FindObjectiveAsync();
	}
	
	int GetMinAO()
	{
		return m_minAOSize;
	}	
	
	int GetMaxAO()
	{
		return m_maxAOSize;
	}	
	
	void SetupOBJ()
	{
		vector position = m_objective.building.GetOrigin();
		
		TILW_FactionTriggerEntity objective = TILW_FactionTriggerEntity.Cast(m_manager.SpawnPrefab(m_objectivePrefab, position));
		vector min,max;
		m_objective.building.GetBounds(min,max);
		vector size = max - min;
		float width = Math.Max(size[0],size[2]) * 1.25;
		width = Math.Max(width, 15);
		
		objective.SetRadius(width);
		objective.SetOwnerFaction(m_teams[1].GetFaction());
	}
	
	void SetupFW()
	{
		TILW_MissionFrameworkEntity framework = TILW_MissionFrameworkEntity.GetInstance();
		
		string defenderKey = m_teams[0].GetFaction();
		string attackerKey = m_teams[1].GetFaction();
		
		//Create flags
		TILW_FactionPlayersKilledFlag defenderFlag = TILW_FactionPlayersKilledFlag();
		defenderFlag.SetFlag("defender");
		defenderFlag.SetKey(defenderKey);
		defenderFlag.SetCasualtyRatio(0.95);

		TILW_FactionPlayersKilledFlag attackerFlag = TILW_FactionPlayersKilledFlag();
		attackerFlag.SetFlag("attacker");
		attackerFlag.SetKey(attackerKey);
		attackerFlag.SetCasualtyRatio(0.95);
		
		array<ref TILW_FactionPlayersKilledFlag> flags = {defenderFlag, attackerFlag};
		
		//Instructions
		TILW_EndGameInstruction defenderInstruction = TILW_EndGameInstruction();
		defenderInstruction.SetGameOverType(EGameOverTypes.EDITOR_FACTION_VICTORY);
		defenderInstruction.SetKey(attackerKey);		
		
		TILW_EndGameInstruction attackerInstruction = TILW_EndGameInstruction();
		attackerInstruction.SetGameOverType(EGameOverTypes.EDITOR_FACTION_VICTORY);
		attackerInstruction.SetKey(defenderKey);

		TILW_EndGameInstruction timelimitInstruction = TILW_EndGameInstruction();
		timelimitInstruction.SetGameOverType(EGameOverTypes.EDITOR_FACTION_VICTORY);
		timelimitInstruction.SetKey(defenderKey);
		timelimitInstruction.SetExecutionDelay(4200);
		
		TILW_EndGameInstruction objectiveInstruction = TILW_EndGameInstruction();
		attackerInstruction.SetGameOverType(EGameOverTypes.EDITOR_FACTION_VICTORY);
		attackerInstruction.SetKey(attackerKey);
		
		//Conditions
		TILW_LiteralTerm defenderTerm = TILW_LiteralTerm();
		defenderTerm.SetFlag("defender");
		
		TILW_LiteralTerm atttackerTerm = TILW_LiteralTerm();
		atttackerTerm.SetFlag("attacker");
		
		TILW_LiteralTerm timeLimitTerm = TILW_LiteralTerm();
		timeLimitTerm.SetFlag("timelimit");
		timeLimitTerm.SetInvert(true);

		TILW_LiteralTerm objectiveTerm = TILW_LiteralTerm();
		objectiveTerm.SetFlag("objective");
		
		//Create events
		TILW_MissionEvent defenderEvent = TILW_MissionEvent();
		defenderEvent.SetName("defender event");
		defenderEvent.SetInstructions({defenderInstruction});
		defenderEvent.SetCondition(defenderTerm);
		
		TILW_MissionEvent attackerEvent = TILW_MissionEvent();
		attackerEvent.SetName("attacker event");
		attackerEvent.SetInstructions({attackerInstruction});
		attackerEvent.SetCondition(atttackerTerm);
		
		TILW_MissionEvent timeLimitEvent = TILW_MissionEvent();
		timeLimitEvent.SetName("timeLimit event");
		timeLimitEvent.SetInstructions({timelimitInstruction});
		timeLimitEvent.SetCondition(timeLimitTerm);
		
		TILW_MissionEvent objectiveEvent = TILW_MissionEvent();
		objectiveEvent.SetName("objective event");
		objectiveEvent.SetInstructions({objectiveInstruction});
		objectiveEvent.SetCondition(objectiveTerm);
		
		array<ref TILW_MissionEvent> events = new array<ref TILW_MissionEvent>;
		events.Insert(defenderEvent);
		events.Insert(attackerEvent);
		events.Insert(timeLimitEvent);
		events.Insert(objectiveEvent);
		
		framework.SetMissionEvents(events);
		framework.SetPlayersKilledFlags(flags);
	}
	
	void SetupAO()
	{
		array<vector> points = new array<vector>;
		
		vector position1 = m_attackerSpawn.m_position;
		vector position2 = m_objective.building.GetOrigin();

		vector direction = vector.Direction(position1, position2).Normalized();

		// Calculate perpendicular directions at 45 degrees
	    vector perpendicular1 = Vector(direction[2], 0, -direction[0]); // Rotate 90 degrees clockwise
	    vector perpendicular2 = Vector(-direction[2], 0, direction[0]); // Rotate 90 degrees counterclockwise
	
	    // Scale perpendicular vectors
	    vector offset1 = (-direction + perpendicular1) * (m_width / 2); // 45 degrees clockwise
	    vector offset2 = (-direction + perpendicular2) * (m_width / 2); // 45 degrees counterclockwise
		vector offset3 = (direction + perpendicular2) * (m_width / 2); // Opposite direction + 45 degrees counterclockwise
	    vector offset4 = (direction + perpendicular1) * (m_width / 2); // Opposite direction + 45 degrees clockwise

	    // Calculate points
	    points.Insert(position1 + offset1);
	    points.Insert(position1 + offset2);
	    points.Insert(position2 + offset3);
	    points.Insert(position2 + offset4);
		
		// Spawn AO
		m_manager.SpawnAO(points);
	}
	
	void SetupMarkers()
	{
		ResourceName prefab = "{DA7538C821CFD1FE}worlds/Burn0ut7/Roulette/Prefabs/RouletteMarker.et";
		
		// Objective
		vector position = m_objective.building.GetOrigin();
		PS_ManualMarker marker = PS_ManualMarker.Cast(m_manager.SpawnPrefab(prefab, position));
		marker.SetImageSet("{E23427CAC80DA8B7}UI/Textures/Icons/icons_mapMarkersUI.imageset");
		marker.SetQuadName("circle-2");
		marker.SetColor(Color.Red);
		marker.SetSize(100);

		// Spawns
		FactionManager fm = GetGame().GetFactionManager();
		
		// Defenders
		SCR_Faction defenders = SCR_Faction.Cast(fm.GetFactionByKey(m_teams[0].GetFaction()));
		marker = PS_ManualMarker.Cast(m_manager.SpawnPrefab(prefab, m_defenderSpawn.m_position));
		marker.SetUseWorldScale(false);
		marker.SetImageSet(defenders.GetFactionFlag());
		marker.SetAngles({0, 90, 0});
		marker.SetSize(40);

		// Attackers
		SCR_Faction attackers = SCR_Faction.Cast(fm.GetFactionByKey(m_teams[1].GetFaction()));
		marker = PS_ManualMarker.Cast(m_manager.SpawnPrefab(prefab, m_attackerSpawn.m_position));
		marker.SetUseWorldScale(false);
		marker.SetImageSet(attackers.GetFactionFlag());
		marker.SetAngles({0, 90, 0});
		marker.SetSize(40);
	}

	GC_R_ObjAttackDefend GetObjective()
	{
		return m_objective;
	}
	
	protected void GetTeams()
	{
		m_teams = m_manager.SelectTeams(2, m_manager.m_teams);
		
		foreach(GC_R_Team team : m_teams)
			Print("GC Roulette | Team: " + team.GetName());
	}
	
	protected void ObjectiveFound()
	{
		Print("GC Roulette | Objective found: " + m_objective.building);
		Print("GC Roulette | Nearby buildings: " + m_objective.buildings.Count());
		
		GetTeams();
		FindAttackerSpawnAsync();
	}
	
	protected void FindObjectiveAsync()
	{
		if (m_searching)
			return;

		m_searching = true;
		GetGame().GetCallqueue().CallLater(FindObjective, 1, false);
	}
	
	protected void FindObjective()
	{
		GC_R_BuidlingQuery query = new GC_R_BuidlingQuery();
		query.BlackListPrefabs = m_buildingBlackList;
		query.BlackListWords = m_buildingBlackListWords;

		for (int i = 0; i < 5; i++) // do a few iterations per frame
		{
			query.Clear();
			IEntity building = m_manager.FindBuildingRandom(query);
			if (!building)
				continue;

			query.Clear();
			array<IEntity> buildings = m_manager.FindBuildings(building.GetOrigin(), m_buildingSearchSize, query);
			if (buildings.Count() < m_minbuildingCount)
				continue;

			m_objective = new GC_R_ObjAttackDefend();
			m_objective.building = building;
			m_objective.buildings = buildings;

			m_searching = false;
			ObjectiveFound();
			
			return;
		}

		GetGame().GetCallqueue().CallLater(FindObjective, 1, false);
	}
	
	protected void AttackerSpawnFound()
	{
		Print("GC Roulette | Attacker Spawn pos found: " + m_attackerSpawn.m_position);
		m_searching = false;

		FindDefenderSpawnAsync();
	}
	
	protected void FindAttackerSpawnAsync()
	{
		Print("GC Roulette | FindAttackerSpawnAsync = " + m_searching);
		if (m_searching)
			return;
		
		m_searching = true;
		
		m_attackerSpawn = new GC_R_SpawnAttacker();
		m_attackerSpawn.m_scenario = this;
		m_attackerSpawn.m_searchCount = 100;

		GetGame().GetCallqueue().CallLater(FindAttackerSpawn, 1, false);
	}
	
	protected void FindAttackerSpawn()
	{
		bool found = m_attackerSpawn.GetSpawn();
		if(found)
			return AttackerSpawnFound();
		
		if(m_attackerSpawn.m_searchCount <= 0)
		{
			Print("GC Roulette | No attacker spawn found");
			m_manager.Reroll();
			return;
		}

		GetGame().GetCallqueue().CallLater(FindAttackerSpawn, 1, false);
	}
	
	protected void DefenderSpawnFound()
	{
		Print("GC Roulette | Defender Spawn pos found: " + m_defenderSpawn.m_position);
		m_searching = false;
		
		GC_R_Team defender = m_teams[0];
		array<GC_R_ElementBase> elements = defender.GetRatioElements(50);
		vector dir = vector.Direction(m_defenderSpawn.m_position, m_attackerSpawn.m_position);
		float yaw = dir.ToYaw();
		
		bool isSucessful = m_manager.SpawnTeam(m_defenderSpawn.m_position, yaw, elements);
		if(!isSucessful)
			return m_manager.Reroll();
		
		GC_R_Team attacker = m_teams[1];
		elements = attacker.GetRatioElements(50);
		dir = vector.Direction(m_attackerSpawn.m_position, m_defenderSpawn.m_position);
		yaw = dir.ToYaw();
		isSucessful = m_manager.SpawnTeam(m_attackerSpawn.m_position, yaw, elements);
		if(!isSucessful)
			return m_manager.Reroll();
		
		SetupAO();
		SetupMarkers();
		m_manager.ResetMapMenu();
	}
	
	protected void FindDefenderSpawnAsync()
	{
		Print("GC Roulette | FindDefenderSpawnAsync = " + m_searching);
		if (m_searching)
			return;
		
		m_searching = true;
		
		m_defenderSpawn = new GC_R_SpawnDefender();
		m_defenderSpawn.m_scenario = this;
		m_defenderSpawn.m_searchCount = 100;

		GetGame().GetCallqueue().CallLater(FindDefenderSpawn, 1, false);
	}
	
	protected void FindDefenderSpawn()
	{
		bool found = m_defenderSpawn.GetSpawn();
		if(found)
			return DefenderSpawnFound();
		
		if(m_defenderSpawn.m_searchCount <= 0)
		{
			Print("GC Roulette | No defender spawn found");
			m_manager.Reroll();
			return;
		}

		GetGame().GetCallqueue().CallLater(FindDefenderSpawn, 1, false);
	}
}

[BaseContainerProps(), BaseContainerCustomTitleField("Team")]
class GC_R_Team
{
	[Attribute("", UIWidgets.EditBox)]
	protected string m_name;

	[Attribute("", UIWidgets.EditBox, desc: "What is this teams faction key in the faction manager?")]
	protected FactionKey m_factionKey;
	
	[Attribute("1", UIWidgets.Auto, desc: "Relative strength. 1 = 1980s baseline. Less = weaker, More = stronger")]
	protected float m_strength;
	
	[Attribute("0", UIWidgets.EditBox, desc: "Use only squad callsigns from the faction manager")]
	protected bool m_squadCallsignsOnly;
	
	[Attribute("", UIWidgets.Auto, desc: "This team cannot versus these faction keys.")]
    protected ref array<FactionKey> m_versusBlacklist;
	
	[Attribute(defvalue: "", UIWidgets.Object)]
    protected ref array<ref GC_R_Company> m_companies;
	
	int m_totalPlayers = 0;
	
	string GetName() { return m_name; }
	
	FactionKey GetFaction(){ return m_factionKey; }
	
	float GetStrength(){ return m_strength; }
	
	bool GetCallsign(){ return m_squadCallsignsOnly; }
	
	array<FactionKey> GetBlacklist(){ return m_versusBlacklist; }
	
	array<ref GC_R_Company> GetCompanies(){ return m_companies; }
	
	bool IsBlacklisted(GC_R_Team team)
	{
		FactionKey otherFaction = team.GetFaction();
		
		if(otherFaction == m_factionKey)
			return true;
		
		if(m_versusBlacklist.Contains(otherFaction))
			return true;
	
		return false;
	}
	
	array<GC_R_ElementBase> GetRatioElements(int targetPlayers)
	{
		array<GC_R_ElementBase> outElements = {};
		m_totalPlayers = 0;
		
		foreach (GC_R_Company company : m_companies)
		{
			array<GC_R_ElementBase> elements = company.Fill(targetPlayers, this);
			foreach(GC_R_ElementBase element : elements)
				outElements.Insert(element);
		}
		
		Print("GC Roulette | GetRatio " + m_name + " - Target: " + targetPlayers + " Selected: " + m_totalPlayers);

		return Order(outElements);
	}
	
    array<GC_R_ElementBase> Order(array<GC_R_ElementBase> pickedElements)
    {
        array<GC_R_ElementBase> ordered = {};

        foreach (GC_R_Company company : m_companies)
        {
            if (pickedElements.Contains(company)) 
                ordered.Insert(company);

            foreach (GC_R_Platoon platoon : company.GetPlatoons())
            {
                if (pickedElements.Contains(platoon))
                    ordered.Insert(platoon);

                foreach (GC_R_Squad squad : platoon.GetSquads())
                {
                    if (pickedElements.Contains(squad))
                        ordered.Insert(squad);
                }
            }
        }
	
		Print("GC Roulette | GetRatio - Elements: " + ordered);
		
        return ordered;
    }
}

[BaseContainerProps(), BaseContainerCustomTitleField("Company")]
class GC_R_Company : GC_R_ElementBase
{
	[Attribute(defvalue: "", UIWidgets.Object)]
    protected ref array<ref GC_R_Platoon> m_platoons;

	void SetParents()
	{
		foreach(GC_R_Platoon platoon : m_platoons)
		{
			platoon.SetParent(this);
		}
	}
	
	array<ref GC_R_Platoon> GetPlatoons()
	{
		return m_platoons;
	}
	
	array<GC_R_ElementBase> Fill(int target, GC_R_Team team)
	{
		array<GC_R_ElementBase> elements = {};
		int platoons = 0;
		bool picked = false;
		
		SetParents();
		
		foreach(GC_R_Platoon platoon : m_platoons)
		{
			array<GC_R_ElementBase> outElements = platoon.Fill(target, team);

			if(outElements.Count() == 0)
				continue;

			platoons++;
			foreach(GC_R_ElementBase element : outElements)
				elements.Insert(element);

			if(platoons >= 2 && !picked)
			{
				int finalCount = Count() + team.m_totalPlayers;
				if(!IsCloser(finalCount, team.m_totalPlayers, target))
					break;	
				
				elements.Insert(this);
				team.m_totalPlayers = finalCount;
				picked = true;
			}
		}
		
		return elements;
	}
}

[BaseContainerProps(), BaseContainerCustomTitleField("Platoon")]
class GC_R_Platoon : GC_R_ElementBase
{
	[Attribute(defvalue: "", UIWidgets.Object)]
    protected ref array<ref GC_R_Squad> m_squads;
	
	void SetParents()
	{
		foreach(GC_R_Squad squad : m_squads)
		{
			squad.SetParent(this);
		}
	}
	
	array<ref GC_R_Squad> GetSquads()
	{
		return m_squads;
	}
	
	array<GC_R_ElementBase> Fill(int target, GC_R_Team team)
	{
		array<GC_R_ElementBase> elements = {};
		int squads = 0;
		bool picked = false;
		
		SetParents();
		
		foreach(GC_R_Squad squad : m_squads)
		{
			int finalCount = squad.Count() + team.m_totalPlayers;
			
			if(!IsCloser(finalCount, team.m_totalPlayers, target))
				break;
			
			squads++;
			elements.Insert(squad);
			team.m_totalPlayers = finalCount;
			
			if(squads >= 2 && !picked)
			{
				finalCount = Count() + team.m_totalPlayers;
				if(!IsCloser(finalCount, team.m_totalPlayers, target))
					break;	

				elements.Insert(this);
				team.m_totalPlayers = finalCount;
				picked = true;
			}
		}
		
		return elements;
	}
}

[BaseContainerProps()]
class GC_R_Squad : GC_R_ElementBase {}

[BaseContainerProps()]
class GC_R_ElementBase
{
	[Attribute("", UIWidgets.ResourceNamePicker, desc: "Player prefab to spawn in")]
	protected ResourceName m_prefab;
	
	[Attribute("", UIWidgets.ResourceNamePicker, desc: "Vehicle prefab")]
	protected ResourceName m_vehiclePrefab;

	protected GC_R_ElementBase m_parent;
	
	void SetParent(GC_R_ElementBase parent)
	{
		m_parent = parent;
	}

	GC_R_ElementBase GetParent()
	{
		return m_parent;
	}
	
	ResourceName GetPrefab()
	{
		return m_prefab;
	}
	
	ResourceName GetVehicle()
	{
		return m_vehiclePrefab;
	}
	
	int Count()
	{
		array<ResourceName> slots;
		Resource.Load(m_prefab).GetResource().ToBaseContainer().Get("m_aUnitPrefabSlots", slots);

		return slots.Count();
	}
	
	protected bool IsCloser(int newTotal, int currentTotal, int target)
	{
		return Math.AbsInt(target - newTotal) <= Math.AbsInt(target - currentTotal);
	}
}

enum GC_R_EAssetFlags
{
	Ground = 1 << 0,
	Air = 1 << 1,
	Water = 1 << 2
}

class GC_R_ObjBase{}

class GC_R_ObjAttackDefend : GC_R_ObjBase
{
	IEntity building;
	ref array<IEntity> buildings = {};
}

class GC_R_SpawnBase
{
	bool m_found = false;
	vector m_position;
	GC_R_Team m_team;
	int m_searchCount;
}

class GC_R_SpawnAttacker : GC_R_SpawnBase
{
	ref GC_R_AttackDefend m_scenario;
	
	bool GetSpawn()
	{
		for (int i = 0; i < 5; i++)
		{
			if(SearchLine())
			{
				m_found = true;
				return true;
			}
			
			m_searchCount--;
		}
		
		return false;
	}
	
	bool SearchLine()
	{
		GC_R_Manager manager = GC_R_Manager.GetInstance();
		
		float radius = 15;
		float searchDistance = 500;
		
		float angle = manager.GetRandom().RandFloatXY(0, Math.PI2);
		
		float stepDistance = searchDistance * 2;
		
		float distance = m_scenario.GetMinAO();
		float maxDistance = m_scenario.GetMaxAO();
		
		vector worldSize = Vector(manager.GetMapSize(), 0, manager.GetMapSize());
		vector edgeBuffer = Vector(manager.EdgeBuffer(), 0, manager.EdgeBuffer());
		vector objPos = m_scenario.GetObjective().building.GetOrigin();
		
		while(distance < maxDistance)
		{
			float directionX = Math.Cos(angle);
	    		float directionZ = Math.Sin(angle);
			
			vector position = Vector(directionX, 0, directionZ) * distance + objPos;
			position[1] = GetGame().GetWorld().GetSurfaceY(position[0], position[2]);
			
			if(!manager.InXZBounds(position, edgeBuffer, worldSize))
				break;

			bool found = manager.FindEmptyPosition(position, radius, searchDistance);
			if(found)
			{
				if(manager.SurfaceIsWater(position))
					return false;
				
				m_position = position;
				return true;
			}

			distance += stepDistance;
		}

		return false;
	}
}

class GC_R_SpawnDefender : GC_R_SpawnBase
{
	ref GC_R_AttackDefend m_scenario;
	
	bool GetSpawn()
	{
		for (int i = 0; i < 5; i++)
		{
			if(Search())
			{
				m_found = true;
				return true;
			}
			
			m_searchCount--;
		}
		
		return false;
	}
	
	bool Search()
	{
		float radius = 15;
		float searchDistance = 500;
		
		GC_R_Manager manager = GC_R_Manager.GetInstance();
		vector position = m_scenario.GetObjective().building.GetOrigin();
		
		bool found = manager.FindEmptyPosition(position, radius, searchDistance);
		if(found)
		{
			if(manager.SurfaceIsWater(position))
				return false;
			
			m_position = position;
			
			return true;
		}
		
		return false;
	}
}

class GC_R_BuidlingQuery : GC_R_BaseQuery
{
	ref array<ResourceName> BlackListPrefabs = {};
	ref array<string> BlackListWords = {};

	override bool QueryCallback(IEntity entity)
	{
		if(entity.GetRootParent() != entity)
			return true;
		
		if(!Building.Cast(entity))
			return true;
		
		if(IsEntityBL(entity))
			return true;
		
		if(!BlackListPrefabs.IsEmpty())
			if(ArrayContainsKind(entity, BlackListPrefabs))
				return true;
		
		m_entities.Insert(entity);

		return true;
	}
	
	bool IsEntityBL(IEntity entity)
	{
		string prefabName = entity.GetPrefabData().GetPrefabName();
		if(prefabName.IsEmpty())
			return true;
		
		foreach(string word: BlackListWords)
			if(prefabName.Contains(word))
				return true;
		
		return false;
	}
}

class GC_R_PhysQuery : GC_R_BaseQuery
{
	ref array<IEntity> m_ignoreEntities;
	bool m_found;
	
	override bool QueryCallback(IEntity entity)
	{
		if(entity != entity.GetRootParent())
			return true;
		
		if(m_ignoreEntities && m_ignoreEntities.Contains(entity))
			return true;
		
		if(!entity.GetPhysics())
			return true;
		
		m_found = true;
		
		return false;
	}
}

class GC_R_BaseQuery
{
	protected ref array<IEntity> m_entities = {};
	
	bool QueryCallback(IEntity entity)
	{
		m_entities.Insert(entity);
		
		return true;
	}

	void Clear()
	{
		m_entities.Clear();
	}
	
	array<IEntity> GetResults()
	{
		return m_entities;
	}
	
	bool ArrayContainsKind(IEntity entity, array<ResourceName> list)
	{
		EntityPrefabData epd = entity.GetPrefabData();
		if (!epd)
			return false;

		BaseContainer bc = epd.GetPrefab();
		if (!bc)
			return false;
		
		foreach (ResourceName rn : list)
			if (SCR_BaseContainerTools.IsKindOf(bc, rn))
				return true;

		return false;
	}
}

class GC_R_Helper<Class T>
{
	static void Shuffle(inout array<T> items, RandomGenerator random)
	{
		for (int i = items.Count() - 1; i > 0; i--)
		{
			int j = random.RandIntInclusive(0, i);
			T temp = items[i];
			items[i] = items[j];
			items[j] = temp;
		}
	}
	
	static void ShuffleR(inout array<ref T> items, RandomGenerator random)
	{
		for (int i = items.Count() - 1; i > 0; i--)
		{
			int j = random.RandIntInclusive(0, i);
			T temp = items[i];
			items[i] = items[j];
			items[j] = temp;
		}
	}
	
	static T RandomElement(array<T> items, RandomGenerator random)
	{
		if(items.IsEmpty())
			return null;
		
		int index = random.RandIntInclusive(0, items.Count()-1);
		return items[index];
	}
	
	static T RandomElementR(array<ref T> items, RandomGenerator random)
	{
		if(items.IsEmpty())
			return null;
		
		int index = random.RandIntInclusive(0, items.Count()-1);
		return items[index];
	}
	
	static int RandomIndex(array<T> items, RandomGenerator random)
	{
		if(items.IsEmpty())
			return null;
		
		return random.RandIntInclusive(0, items.Count()-1);
	}
	
	static int RandomIndexR(array<ref T> items, RandomGenerator random)
	{
		if(items.IsEmpty())
			return null;
		
		return random.RandIntInclusive(0, items.Count()-1);
	}
}