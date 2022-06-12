// Neutron - Gwennaël Arbona

#pragma once

#include "CoreMinimal.h"
#include "Tickable.h"
#include "GameFramework/Actor.h"

#include "NeutronPostProcessManager.generated.h"

/*----------------------------------------------------
    Supporting types
----------------------------------------------------*/

// Base structure for post-process presets
struct FNeutronPostProcessSettingBase
{
	FNeutronPostProcessSettingBase() : TransitionDuration(0.5f)
	{}

	float TransitionDuration;
};

// Control function to determine the post-processing index
DECLARE_DELEGATE_RetVal(int32, FNeutronPostProcessControl);
DECLARE_DELEGATE_FiveParams(FNeutronPostProcessUpdate, class UPostProcessComponent*, TArray<class UMaterialInstanceDynamic*>,
	const TSharedPtr<FNeutronPostProcessSettingBase>&, const TSharedPtr<FNeutronPostProcessSettingBase>&, float);

/** Post-processing owner */
UCLASS(ClassGroup = (Neutron))
class NEUTRON_API ANeutronPostProcessActor : public AActor
{
	GENERATED_BODY()
};

/*----------------------------------------------------
    System
----------------------------------------------------*/

/** Post-processing manager */
UCLASS(ClassGroup = (Neutron))
class NEUTRON_API UNeutronPostProcessManager
	: public UObject
	, public FTickableGameObject
{
	GENERATED_BODY()

public:

	UNeutronPostProcessManager();

	/*----------------------------------------------------
	    System interface
	----------------------------------------------------*/

	/** Get the singleton instance */
	static UNeutronPostProcessManager* Get()
	{
		return Singleton;
	}

	/** Initialize this class */
	void Initialize(class UNeutronGameInstance* GameInstance);

	/** Start playing on a new level */
	void BeginPlay(class ANeutronPlayerController* PC, FNeutronPostProcessControl Control, FNeutronPostProcessUpdate Update);

	/*----------------------------------------------------
	    Public methods
	----------------------------------------------------*/

	/** Register a new preset - index 0 is considered neutral ! */
	template <typename T>
	void RegisterPreset(T Index, TSharedPtr<FNeutronPostProcessSettingBase> Preset)
	{
		PostProcessSettings.Add(static_cast<int32>(Index), Preset);
	}

	/*----------------------------------------------------
	    Tick
	----------------------------------------------------*/

	virtual void              Tick(float DeltaTime) override;
	virtual ETickableTickType GetTickableTickType() const override
	{
		return ETickableTickType::Always;
	}
	virtual TStatId GetStatId() const override
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(UNeutronSoundManager, STATGROUP_Tickables);
	}
	virtual bool IsTickableWhenPaused() const
	{
		return true;
	}
	virtual bool IsTickableInEditor() const
	{
		return false;
	}

protected:

	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

	// Singleton pointer
	static UNeutronPostProcessManager* Singleton;

	// Post-process materials that are dynamically controlled
	UPROPERTY()
	TArray<class UMaterialInstanceDynamic*> PostProcessMaterials;

	// Post-process component that's dynamically controlled
	UPROPERTY()
	class UPostProcessComponent* PostProcessVolume;

	// General state
	FNeutronPostProcessControl                              ControlFunction;
	FNeutronPostProcessUpdate                               UpdateFunction;
	int32                                                   CurrentPreset;
	int32                                                   TargetPreset;
	float                                                   CurrentPresetAlpha;
	TMap<int32, TSharedPtr<FNeutronPostProcessSettingBase>> PostProcessSettings;
};
