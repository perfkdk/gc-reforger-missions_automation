[BaseContainerProps()]
class GC_R_BaseScenario : Managed
{
	void Intialize();
	void Reroll();
}

[BaseContainerProps(), BaseContainerCustomTitleField("Attack and Defend")]
class GC_R_AttackDefend: GC_R_BaseScenario
{
	[Attribute(defvalue: "50", uiwidget: UIWidgets.CheckBox, desc: "Amount of buildings needed in the ", category: "Objective")]
    protected float m_buildingSearchSize;
	
	[Attribute(defvalue: "10", uiwidget: UIWidgets.CheckBox, desc: "Amount of buildings needed in Building Search Size", category: "Objective")]
    protected int m_minbuildingCount;
	
	[Attribute(defvalue: "", uiwidget: UIWidgets.ResourceNamePicker, desc: "Blacklist building prefab or kind", category: "Objective")]
    protected ref array<ResourceName> m_buildingBlackList;
	
	[Attribute(defvalue: "", uiwidget: UIWidgets.Auto, desc: "Blacklist building of same name", category: "Objective")]
    protected ref array<string> m_buildingBlackListWords;
	
	[Attribute(defvalue: "2000", uiwidget: UIWidgets.CheckBox, desc: "Min size of AO in meters", category: "AO")]
    protected int m_minAOSize;
	
	[Attribute(defvalue: "5000", uiwidget: UIWidgets.CheckBox, desc: "Max size of AO in meters", category: "AO")]
    protected int m_maxAOSize;
	
	[Attribute(defvalue: "100", uiwidget: UIWidgets.CheckBox, desc: "Max size of AO in meters", category: "Spawn")]
    protected int m_maxAttempts;
	
	protected int m_searchAttempts;
	
	protected GC_R_Manager m_manager;
	protected ref GC_R_ObjAttackDefend m_objective;
	protected ref GC_R_SpawnAttacker m_attackerSpawn;
	
	ref 	array<ref GC_R_Team> m_teams = {};
	
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
		Print("GC Roulette | Spawn pos found: " + m_attackerSpawn.m_position);
		
		IEntity player = GetGame().GetPlayerController().GetControlledEntity();

		player.SetOrigin(m_attackerSpawn.m_position);
		
		
		GetGame().GetCallqueue().CallLater(m_manager.Reroll, 3000);
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
			//m_manager.Reroll();
			return;
		}

		GetGame().GetCallqueue().CallLater(FindAttackerSpawn, 1, false);
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
}

[BaseContainerProps(), BaseContainerCustomTitleField("Company")]
class GC_R_Company : GC_R_ElementBase
{
	[Attribute(defvalue: "", UIWidgets.Object)]
    protected ref array<ref GC_R_Platoon> m_platoons;

	array<ref GC_R_Platoon> GetPlatoons()
	{
		return m_platoons;
	}
}

[BaseContainerProps(), BaseContainerCustomTitleField("Platoon")]
class GC_R_Platoon : GC_R_ElementBase
{
	[Attribute(defvalue: "", UIWidgets.Object)]
    protected ref array<ref GC_R_Squad> m_squads;
	
	array<ref GC_R_Squad> GetSquads()
	{
		return m_squads;
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

	ResourceName GetPrefab()
	{
		return m_prefab;
	}
	
	int GetPlayerCount()
	{
		array<ResourceName> slots;
		Resource.Load(m_prefab).GetResource().ToBaseContainer().Get("m_aUnitPrefabSlots", slots);

		return slots.Count();
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