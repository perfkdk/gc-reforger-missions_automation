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
		
		IEntity player = GetGame().GetPlayerController().GetControlledEntity();
		player.SetOrigin(m_defenderSpawn.m_position);
		
		
		m_teams[0].GetRatioElements(50);
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
			Print("GC Roulette | GetRatio elements3" + elements);
			foreach(GC_R_ElementBase element : elements)
				outElements.Insert(element);
		}
		
		Print("GC Roulette | GetRatio elements2" + outElements);
		outElements = Order(outElements);
		
		Print("GC Roulette | GetRatio " + m_name + " - Target: " + targetPlayers + " Selected: " + m_totalPlayers);
		Print("GC Roulette | GetRatio elements3" + outElements);
		
		return outElements;
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

        foreach (GC_R_ElementBase e : pickedElements)
        {
            GC_R_Squad squad = GC_R_Squad.Cast(e);
            if (!squad) 
                continue;

            GC_R_ElementBase parent = squad.GetParent();
            if (!parent || !pickedElements.Contains(parent))
                ordered.Insert(squad);
        }

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
				
				Print("GC Roulette | CompanyFill - Count: " + Count() + " Total: " + team.m_totalPlayers);
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
			
			Print("GC Roulette | SquadFill - Count: " + squad.Count() + " Total: " + team.m_totalPlayers);
			squads++;
			elements.Insert(squad);
			team.m_totalPlayers = finalCount;
			
			if(squads >= 2 && !picked)
			{
				finalCount = Count() + team.m_totalPlayers;
				if(!IsCloser(finalCount, team.m_totalPlayers, target))
					break;	
				
				Print("GC Roulette | PlatoonFill - Count: " + Count() + " Total: " + team.m_totalPlayers);
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