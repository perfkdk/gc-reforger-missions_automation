[EntityEditorProps(category: "GameScripted/ScriptWizard", description: "Deletes entity in certain radius.")]
class BLU_PrefabDeleterEntityClass : GenericEntityClass
{
}



//------------------------------------------------------------------------------------------------
class BLU_PrefabDeleterEntity : GenericEntity
{
	[Attribute(defvalue: "20", uiwidget: UIWidgets.Slider, desc: "Radius in which entities are deleted", "0 1000 1")]
	protected float m_fRadius;
	
	[Attribute(uiwidget: UIWidgets.ResourcePickerThumbnail, params: "et")]
	ResourceName m_sResourceName;

	private ref array<IEntity> QueryEntitiesToRemove;
	
	//------------------------------------------------------------------------------------------------
	private bool QueryEntities(IEntity e)
	{
		
		
		if(e.FindComponent(BLU_ProtectDeleteComponent)==null)
			QueryEntitiesToRemove.Insert(e);
		return true;
		
	}
	


	//------------------------------------------------------------------------------------------------
	override void EOnInit(IEntity owner)
	{
		QueryEntitiesToRemove = {};
		// server only
		RplComponent rplComponent = RplComponent.Cast(owner.FindComponent(RplComponent));
		if (rplComponent && !rplComponent.IsMaster())
			return;

		BaseWorld world = GetWorld();
	
		
		world.QueryEntitiesBySphere(owner.GetOrigin(), m_fRadius, QueryEntities);

		foreach(IEntity e : QueryEntitiesToRemove)
		{
			SCR_EntityHelper.DeleteEntityAndChildren(e);
		}
		
			
		// destroy self
		delete owner;
		
	}
	
	

	//------------------------------------------------------------------------------------------------
	// constructor
	//! \param[in] src
	//! \param[in] parent
	void BLU_PrefabDeleterEntity(IEntitySource src, IEntity parent)
	{
		if (!GetGame().InPlayMode())
			return;
		
		SetEventMask(EntityEvent.INIT);
	}

#ifdef WORKBENCH	

	
	//------------------------------------------------------------------------------------------------
	override void _WB_AfterWorldUpdate(float timeSlice)
	{

		auto origin = GetOrigin();
		auto radiusShape = Shape.CreateSphere(COLOR_YELLOW, ShapeFlags.WIREFRAME | ShapeFlags.ONCE, origin, m_fRadius);

		super._WB_AfterWorldUpdate(timeSlice);
		
	}
	
	
	
#endif

}