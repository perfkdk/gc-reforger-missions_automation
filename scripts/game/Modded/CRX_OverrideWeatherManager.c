modded class SCR_TimeAndWeatherHandlerComponent : SCR_BaseGameModeComponent
{
	
	protected TimeAndWeatherManagerEntity m_TimeAndWeatherManager;
	
	[Attribute("49.019001", uiwidget:UIWidgets.Slider, params: "-90 90 1", category: "Environment")]
	protected float m_fLatitude;
	
	[Attribute("37.921001", uiwidget:UIWidgets.Slider, params: "-180 180 1", category: "Environment")]
	protected float m_fLongitude;
	
	[Attribute("2", uiwidget:UIWidgets.Slider, params: "-12 12 0.25", category: "Environment")]
	protected float m_fUTCTimeZone;
	
	[Attribute(defvalue: "false", uiwidget:UIWidgets.CheckBox, category: "Environment")]
	protected bool m_fDSTEnabled;
	[Attribute("1", uiwidget:UIWidgets.Slider, params: "0 5 0.25", category: "Environment")]
	protected float m_fDSTOffsetHours;
	
	[Attribute(defvalue: "false", uiwidget:UIWidgets.CheckBox, category: "Environment")]
	protected bool m_fFogDensityOveride;
	[Attribute("1", uiwidget:UIWidgets.Slider, params: "0 1 0.01", category: "Environment")]
	protected float m_fFogDensity;
	
	[Attribute(defvalue: "false", uiwidget:UIWidgets.CheckBox, category: "Environment")]
	protected bool m_fFogHeightOveride;
	[Attribute("1", uiwidget:UIWidgets.Slider, params: "0 150 0.1", category: "Environment")]
	protected float m_fFogHeight;
	
	[Attribute(defvalue: "false", uiwidget:UIWidgets.CheckBox, category: "Environment")]
	protected bool m_fRainOveride;
	[Attribute("1", uiwidget:UIWidgets.Slider, params: "0 1 0.01", category: "Environment")]
	protected float m_fRain;
	
	[Attribute(defvalue: "false", uiwidget:UIWidgets.CheckBox, category: "Environment")]
	protected bool m_fWindOveride;
	[Attribute("1", uiwidget:UIWidgets.Slider, params: "0 50 1", category: "Environment", desc:"Wind speed in m/s")]
	protected float m_fWind;
	
	//------------------------------------------------------------------------------------------------
	override void OnWorldPostProcess(World world)
	{
		super.OnWorldPostProcess(world);

		if (!Replication.IsServer() || !GetGame().InPlayMode())
			return;

		if (s_Instance != this)
		{
			Print("Multiple instances of SCR_TimeAndWeatherHandlerComponent detected.", LogLevel.WARNING);
			return;
		}
			SetupTimeZoningAndWeather(m_fLatitude, m_fLongitude, m_fUTCTimeZone, m_fDSTOffsetHours, m_fDSTEnabled, m_fFogDensity, m_fFogDensityOveride, m_fFogHeight, m_fFogHeightOveride, m_fRain, m_fRainOveride, m_fWind , m_fWindOveride);
	}
	
	protected void SetupTimeZoningAndWeather(float lat, float long, float timezone,float offset, bool DST, float fogdensity, bool fogdensityoverride, float fogheight, bool fogheightoverride, float rain, bool rainoverride, float wind, bool windoverride)
	{
		ChimeraWorld world = ChimeraWorld.CastFrom(GetOwner().GetWorld());
		if (!world)
			return;
		
		TimeAndWeatherManagerEntity manager = world.GetTimeAndWeatherManager();
		manager.SetTimeZoneOffset(timezone);
		manager.SetDSTOffset(offset);
		manager.SetCurrentLatitude(lat);
		manager.SetCurrentLongitude(long);
		manager.SetFogAmountOverride(true, fogdensity);
		manager.SetFogHeightDensityOverride(true, fogheight);
		manager.SetRainIntensityOverride(true, rain);
		manager.SetWindSpeedOverride(true, wind);
		manager.SetDSTEnabled(DST)
		
	}
}