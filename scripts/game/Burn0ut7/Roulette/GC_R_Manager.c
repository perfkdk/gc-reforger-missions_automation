[ComponentEditorProps()]
class GC_R_ManagerClass : GameEntityClass
{
}

class GC_R_Manager : GameEntity
{
	[Attribute(defvalue: "500", UIWidgets.Auto)]
	float m_mapEdgeBuffer;
	
	[Attribute(defvalue: "", UIWidgets.Object)]
    ref array<ref GC_R_BaseScenario> m_scenarios;	
	
	[Attribute(defvalue: "", UIWidgets.Object)]
    ref array<ref GC_R_Team> m_teams;	
	
	protected const float m_searchDistance = 250;
	
	protected static GC_R_Manager m_instance;
	protected BaseWorld m_world;
	protected float m_worldSize;
	protected ref RandomGenerator m_random = new RandomGenerator();
	protected ref GC_R_BaseScenario m_currentScenario;
	
	void GC_R_Manager(IEntitySource src, IEntity parent)
	{
		if(!GetGame().InPlayMode())
			return;
		
		SetEventMask(EntityEvent.INIT);
	}

	override protected void EOnInit(IEntity owner)
	{
		Intialize();
		
		GetGame().GetInputManager().AddActionListener("CharacterMelee", EActionTrigger.DOWN, TestM);
	}
	
	void TestM()
	{
		IEntity player = GetGame().GetPlayerController().GetControlledEntity();
		vector pos = player.GetOrigin();

		bool empty = IsPositionEmpty(pos, 10);
		
		Print("GC Roulette | empty = " + empty);
		
		//player.SetOrigin(pos);
	}
	
	void NewScenario()
	{
		GC_R_BaseScenario selected = GC_R_Helper<GC_R_BaseScenario>.RandomElementR(m_scenarios, m_random);
		m_currentScenario = GC_R_BaseScenario.Cast(selected.Clone());

		Print("GC Roulette | NewScenario = " + m_currentScenario);
	
		m_currentScenario.Intialize();
	}
	
	protected void Intialize()
	{
		Print("GC Roulette | Intialize");
		
		SetSeed();
		
		m_instance = this;
		m_world = GetGame().GetWorld();
		m_worldSize = GetMapSize();
		
		NewScenario();
		
		//GetGame().GetCallqueue().CallLater(Reroll, 3000, true);
	}
	
	void Reroll()
	{
		SetSeed();
		NewScenario();
	}
	
	array<ref GC_R_Team> SelectTeams(int count, array<ref GC_R_Team> teams)
	{
		if (teams.IsEmpty() || teams.Count() < count)
			return null;
		
		array<ref GC_R_Team> pool = {};
		foreach (GC_R_Team team : teams)
			pool.Insert(team);
		
		GC_R_Helper<GC_R_Team>.ShuffleR(pool, m_random);
		
		array<ref GC_R_Team> selected = {};
		
		foreach (GC_R_Team team : pool)
		{
			array<ref GC_R_Team> opponents = FindOpponents(team, count - 1, pool);
			if (!opponents.IsEmpty())
			{
				selected.Insert(team);
				foreach (GC_R_Team opp : opponents)
					selected.Insert(opp);
				
				break;
			}
		}
		
		if (selected.Count() < count)
			Print("GC Roulette | Failed to find enough valid teams (" + selected.Count() + "/" + count + ")", LogLevel.ERROR);
		
		GC_R_Helper<GC_R_Team>.ShuffleR(selected, m_random);
		
		return selected;
	}

	array<ref GC_R_Team> FindOpponents(GC_R_Team baseTeam, int needed, array<ref GC_R_Team> pool)
	{
		array<ref GC_R_Team> opponents = {};
		array<ref GC_R_Team> shuffled = {};
		
		foreach (GC_R_Team team : pool)
			if(baseTeam != team)
				shuffled.Insert(team);

		GC_R_Helper<GC_R_Team>.ShuffleR(shuffled, m_random);
		
		foreach (GC_R_Team other : shuffled)
		{
			if (baseTeam.IsBlacklisted(other) || other.IsBlacklisted(baseTeam))
				continue;
			
			bool conflict = false;
			foreach (GC_R_Team opp : opponents)
			{
				if (other.IsBlacklisted(opp) || opp.IsBlacklisted(other))
				{
					conflict = true;
					break;
				}
			}
	
			if (conflict)
				continue;
			
			opponents.Insert(other);
			
			if (opponents.Count() >= needed)
				break;
		}
		
		if (opponents.Count() < needed)
				return {};
		
		return opponents;
	}

	IEntity FindBuildingRandom(notnull GC_R_BaseQuery query)
	{
		IEntity building;
		
		while(building == null)
		{
			vector position = Vector(
				m_random.RandFloatXY(m_mapEdgeBuffer, m_worldSize),
				0,
				m_random.RandFloatXY(m_mapEdgeBuffer, m_worldSize)
			);
			position[1] = m_world.GetSurfaceY(position[0], position[2]);
			
			array<IEntity> buildings = FindBuildings(position, m_searchDistance, query);
			if(buildings.IsEmpty())
				continue;
			
			//Why doesn't helper work with IEntity?
			//int index = GC_R_Helper<IEntity>.RandomIndex(buildings, m_random);
			int index = m_random.RandIntInclusive(0, buildings.Count()-1);
			building = buildings[index];
		}
		
		return building;
	}
		
	array<IEntity> FindBuildings(vector position, float searchDistance, notnull GC_R_BaseQuery query)
	{
		m_world.QueryEntitiesBySphere(position, searchDistance, query.QueryCallback, null, EQueryEntitiesFlags.STATIC);
		
		array<IEntity> buildings = query.GetResults();
		
		return buildings;
	}
	
	bool FindEmptyPosition(inout vector position, float radius, float maxDistance)
	{
		if (radius <= 0 || maxDistance <= 0)
			return false;
		
		float cellWidth = radius * Math.Sqrt(3);
		float cellHeight = radius * 2;
		int maxRadiusSteps = Math.Ceil(maxDistance / radius);
		
		for (int ring = 0; ring < maxRadiusSteps; ring++)
		{
			for (int x = -ring; x <= ring; x++)
			{
				float posX = cellWidth * x;
				float posY = cellHeight * (x - SCR_Math.fmod(x, 1)) * 0.5;
	
				int yMin = Math.Max(-ring -x, -ring);
				int yMax = Math.Min(ring -x, ring);
				int yStep;
				if (Math.AbsInt(x) == ring)
					yStep = 1;
				else 
					yStep = yMax - yMin;
	
				for (int y = yMin; y <= yMax; y += yStep)
				{
					vector searchPos = position + Vector(posX, 0, posY + cellHeight * y);
					if (vector.DistanceXZ(searchPos, position) > maxDistance - radius)
						continue;
	
					searchPos[1] = m_world.GetSurfaceY(searchPos[0], searchPos[2]);
		
					if (IsPositionEmpty(searchPos, radius))
					{
						position = searchPos;
						return true;
					}
				}
			}
		}
		
		return false;
	}
	
	bool IsPositionEmpty(vector position, float radius)
	{
		GC_R_PhysQuery query = new GC_R_PhysQuery();

		m_world.QueryEntitiesBySphere(position, radius, query.QueryCallback, null, EQueryEntitiesFlags.ALL);
		if(!query.m_found)
			return true;

		return false;
	}
	
	IEntity SpawnPrefab(ResourceName prefab, vector transform[4] = { "0 0 0", "0 0 0", "0 0 0", "0 0 0" })
	{
		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.Transform = transform;
		
		return GetGame().SpawnEntityPrefab(Resource.Load(prefab), null, spawnParams);
	}
	
	/*
	bool IsSpawnSafe(vector position, notnull IEntity entity, float maxNormalDiff)
	{
		vector min, max;
		entity.GetWorldBounds(min, max);
	
		vector points[5];
		points[0] = position;
		points[1] = Vector(min[0], 0, min[2]);
		points[2] = Vector(max[0], 0, min[2]);
		points[3] = Vector(max[0], 0, max[2]);
		points[4] = Vector(min[0], 0, max[2]);
	
		vector normals[5];
		for (int i = 0; i < points.Count(); i++)
		{
			vector tracePos = points[i];
			tracePos[1] = position[1]; // keep Y height
			normals[i] = GetTerrainNormal(tracePos, m_world);
		}
	
		// --- Average normal
		vector avgNormal = "0 0 0";
		foreach (vector n : normals)
			avgNormal += n;
		avgNormal.Normalize();
	
		// --- Compare all normals to the average
		foreach (vector n2 : normals)
		{
			float dot = vector.Dot(avgNormal, n2);
			if (dot < maxNormalDiff) // dot lower → larger angle difference
				return false;
		}
	
		return true;
	}
	*/
	
	vector GetTerrainNormal(inout vector position)
	{
		float surfaceY = m_world.GetSurfaceY(position[0], position[2]);
		position[1] = surfaceY + 0.01;
		
		TraceParam param = new TraceParam();
		param.Flags = TraceFlags.ANY_CONTACT | TraceFlags.WORLD | TraceFlags.OCEAN | TraceFlags.ENTS;
		param.Start = position + vector.Up;
		param.End = position - vector.Up;
		
		float percent = m_world.TraceMove(param);

		position = vector.Lerp(param.Start, param.End, percent);
		
		return param.TraceNorm;
	}
	
	bool SurfaceIsWater(vector pos)
	{
	    pos[1] = GetGame().GetWorld().GetSurfaceY(pos[0], pos[2]);
	    vector outWaterSurfacePoint;
	    EWaterSurfaceType outType;
	    vector transformWS[4];
	    vector obbExtents;
		
	    ChimeraWorldUtils.TryGetWaterSurface(m_world, pos, outWaterSurfacePoint, outType, transformWS, obbExtents);
		
	    return outType != EWaterSurfaceType.WST_NONE;
	}
	
	static GC_R_Manager GetInstance()
	{
		return m_instance;
	}
	
	RandomGenerator GetRandom()
	{
		return m_random;
	}
	
	void SetSeed(int seed = -1)
	{
		if (seed == -1)
		{
			int t = System.GetUnixTime();
			int tc = System.GetTickCount();
			int r = Math.RandomInt(0, 999999);
	
			// Mix bits to avoid predictable correlation
			seed = t ^ (tc << 16) ^ (r << 1);
			seed ^= (seed << 13);
			seed ^= (seed >> 17);
			seed ^= (seed << 5);
		}
		
		m_random.SetSeed(seed);
		Print("GC Roulette | Seed = " + seed);
	}
	
	float EdgeBuffer()
	{
		return m_mapEdgeBuffer;
	}
	
	float GetMapSize()
	{
		if(m_worldSize)
			return m_worldSize;
		
		vector mins, max;
		GetGame().GetWorldEntity().GetWorldBounds(mins, max);
		float size = Math.Round(Math.Min(max[0] - mins[0], max[2] - mins[2]));

		while(size > 0)
		{
			size--;
			
			float y = m_world.GetSurfaceY(size,size);
			if(y != -256)
				break;
		}
		
		size = Math.Max(0,size - m_mapEdgeBuffer);
		Print("GC Roulette | Map size = " + size);
		
		return size;
	}
	
	static bool InXZBounds(vector position, vector min, vector max)
    {
		if(position[0] < min[0] || position[0] > max[0])
			return false;
		
		if(position[2] < min[2] || position[2] > max[2])
			return false;
		
		return true;
    }
		
	void SetTime(float hours24 = -1, bool dayOnly = true)
	{
		TimeAndWeatherManagerEntity manager = ChimeraWorld.CastFrom(m_world).GetTimeAndWeatherManager();
		if (!manager)
			return;
	
		float sunrise, sunset;
		if (!manager.GetSunriseHour(sunrise) || !manager.GetSunsetHour(sunset))
		{
			sunrise = 8;
			sunset = 18;
		}
	
		float startTime = 0;
		float endTime = 24;
	
		if (dayOnly)
		{
			startTime = sunrise + 2;
			endTime = sunset - 2;
		}
	
		if (hours24 == -1)
			hours24 = m_random.RandFloatXY(startTime, endTime);
	
		hours24 = Math.Clamp(hours24, 0.0, 24.0);
		manager.SetTimeOfTheDay(hours24, true);
	}
}