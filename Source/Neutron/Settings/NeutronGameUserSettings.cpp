// Neutron - Gwennaël Arbona

#include "NeutronGameUserSettings.h"
#include "Neutron/Neutron.h"

#include "Modules/ModuleManager.h"
#include "RenderUtils.h"
#include "RenderCore.h"

#define HAS_DLSS PLATFORM_WINDOWS && 1

#if HAS_DLSS
#include "DLSS.h"
#endif    // HAS_DLSS

/*----------------------------------------------------
    Constructor
----------------------------------------------------*/

UNeutronGameUserSettings::UNeutronGameUserSettings()
{}

/*----------------------------------------------------
    Settings
----------------------------------------------------*/

void UNeutronGameUserSettings::ApplyCustomGraphicsSettings()
{
	// Toggle TSR
	IConsoleVariable* TSRVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.AntiAliasingMethod"));
	if (TSRVar)
	{
		TSRVar->Set(EnableTSR ? 4 : 2, ECVF_SetByConsole);
	}

	EnableDLSS = EnableDLSS && IsDLSSSupported();

#if HAS_DLSS

	// Toggle VRS
	IConsoleVariable* VRSVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.VRS.Enable"));
	if (VRSVar)
	{
		VRSVar->Set(EnableDLSS ? 0 : 1, ECVF_SetByConsole);
	}

	// Toggle DLSS
	IConsoleVariable* DLSSVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.NGX.DLSS.Enable"));
	if (DLSSVar)
	{
		DLSSVar->Set(EnableDLSS ? 1 : 0, ECVF_SetByConsole);
	}
#endif    // HAS_DLSS

	// Toggle Nanite
	IConsoleVariable* NaniteVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.Nanite"));
	if (NaniteVar)
	{
		NaniteVar->Set(EnableNanite ? 1 : 0, ECVF_SetByConsole);
	}

	// Toggle Lumen HWRT
	IConsoleVariable* LumenHWRTVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.Lumen.HardwareRayTracing"));
	if (LumenHWRTVar)
	{
		LumenHWRTVar->Set(EnableLumenHWRT ? 1 : 0, ECVF_SetByConsole);
	}

	// Toggle virtual shadow maps
	IConsoleVariable* VirtualShadowsVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.Shadow.Virtual.Enable"));
	if (VirtualShadowsVar)
	{
		VirtualShadowsVar->Set(EnableVirtualShadows ? 1 : 0, ECVF_SetByConsole);
	}

	// Set screen percentage
	IConsoleVariable* ScreenPercentageVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.ScreenPercentage"));
	if (ScreenPercentageVar)
	{
		ScreenPercentageVar->Set(ScreenPercentage, ECVF_SetByConsole);
	}
}

bool UNeutronGameUserSettings::IsHDRSupported() const
{
	return GetFullscreenMode() == EWindowMode::Fullscreen && SupportsHDRDisplayOutput() && IsHDRAllowed();
}

bool UNeutronGameUserSettings::IsDLSSSupported() const
{
#if HAS_DLSS
	IDLSSModuleInterface* DLSSModule = &FModuleManager::LoadModuleChecked<IDLSSModuleInterface>(TEXT("DLSS"));
	if (DLSSModule)
	{
		return DLSSModule->QueryDLSSSupport() == EDLSSSupport::Supported;
	}
#endif    // HAS_DLSS

	return false;
}

bool UNeutronGameUserSettings::IsNaniteSupported() const
{
	return DoesRuntimeSupportNanite(GShaderPlatformForFeatureLevel[GMaxRHIFeatureLevel], true, false);
}

bool UNeutronGameUserSettings::IsLumenSupported() const
{
	EShaderPlatform Platform = GShaderPlatformForFeatureLevel[GMaxRHIFeatureLevel];

	return FDataDrivenShaderPlatformInfo::GetSupportsLumenGI(Platform) && !IsForwardShadingEnabled(Platform);
}

bool UNeutronGameUserSettings::IsLumenHWRTSupported() const
{
	EShaderPlatform Platform = GShaderPlatformForFeatureLevel[GMaxRHIFeatureLevel];

	return IsLumenSupported() && FDataDrivenShaderPlatformInfo::GetSupportsRayTracing(Platform);
}

bool UNeutronGameUserSettings::IsVirtualShadowSupported() const
{
	return IsNaniteSupported();
}

/*----------------------------------------------------
    Inherited
----------------------------------------------------*/

void UNeutronGameUserSettings::SetToDefaults()
{
	Super::SetToDefaults();

	// System
	EnableCrashReports = true;

	// Gameplay / visuals
	MouseSensitivity   = 1.0f;
	GamepadSensitivity = 0.5f;
	FOV                = 90.0f;
	MotionBlurAmount   = 0.5f;

	// Sound
	MasterVolume  = 10;
	UIVolume      = 10;
	EffectsVolume = 10;
	MusicVolume   = 10;

	// Graphics
	EnableTSR            = true;
	EnableDLSS           = false;
	EnableNanite         = true;
	EnableLumen          = true;
	EnableLumenHWRT      = false;
	EnableVirtualShadows = false;
	EnableCinematicBloom = false;
	ScreenPercentage     = 100.0f;
}

void UNeutronGameUserSettings::ApplySettings(bool bCheckForCommandLineOverrides)
{
	Super::ApplySettings(bCheckForCommandLineOverrides);

	ApplyCustomGraphicsSettings();
}
