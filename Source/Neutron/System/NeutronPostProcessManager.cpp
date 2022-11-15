// Neutron - Gwennaël Arbona

#include "NeutronPostProcessManager.h"

#include "Neutron/Settings/NeutronGameUserSettings.h"
#include "Neutron/UI/NeutronUI.h"
#include "Neutron/Neutron.h"

#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/Material.h"
#include "Components/PostProcessComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "Engine.h"

// Statics
UNeutronPostProcessManager* UNeutronPostProcessManager::Singleton = nullptr;

/*----------------------------------------------------
    Constructor
----------------------------------------------------*/

UNeutronPostProcessManager::UNeutronPostProcessManager() : Super(), CurrentPreset(0), TargetPreset(0), CurrentPresetAlpha(0.0f)
{}

/*----------------------------------------------------
    Gameplay
----------------------------------------------------*/

void UNeutronPostProcessManager::Initialize(UNeutronGameInstance* GameInstance)
{
	Singleton = this;
}

void UNeutronPostProcessManager::BeginPlay(
	ANeutronPlayerController* PC, FNeutronPostProcessControl Control, FNeutronPostProcessUpdate Update)
{
	ControlFunction = Control;
	UpdateFunction  = Update;

	TArray<AActor*> PostProcessActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ANeutronPostProcessActor::StaticClass(), PostProcessActors);

	// Set the post process
	if (PostProcessActors.Num())
	{
		PostProcessVolume = Cast<UPostProcessComponent>(PostProcessActors[0]->GetComponentByClass(UPostProcessComponent::StaticClass()));

		// Replace the material by a dynamic variant
		if (PostProcessVolume)
		{
			TArray<FWeightedBlendable> Blendables = PostProcessVolume->Settings.WeightedBlendables.Array;
			PostProcessVolume->Settings.WeightedBlendables.Array.Empty();
			PostProcessMaterials.Empty();

			for (FWeightedBlendable Blendable : Blendables)
			{
				UMaterialInterface* BaseMaterial = Cast<UMaterialInterface>(Blendable.Object);
				NCHECK(BaseMaterial);

				UMaterialInstanceDynamic* MaterialInstance = UMaterialInstanceDynamic::Create(BaseMaterial, GetWorld());
				PostProcessMaterials.Add(MaterialInstance);

				PostProcessVolume->Settings.AddBlendable(MaterialInstance, 1.0f);
			}

			NLOG("UNeutronPostProcessManager::BeginPlay : post-process setup complete");
		}
	}
}

/*----------------------------------------------------
    Tick
----------------------------------------------------*/

void UNeutronPostProcessManager::Tick(float DeltaTime)
{
	if (IsValid(PostProcessVolume))
	{
		// Update desired settings
		if (ControlFunction.IsBound() && CurrentPreset == TargetPreset)
		{
			TargetPreset = ControlFunction.Execute();
		}

		// Update transition time
		float CurrentTransitionDuration = PostProcessSettings[TargetPreset]->TransitionDuration;
		if (CurrentPreset != TargetPreset)
		{
			CurrentPresetAlpha -= DeltaTime / CurrentTransitionDuration;
		}
		else if (CurrentPreset != 0)
		{
			CurrentPresetAlpha += DeltaTime / CurrentTransitionDuration;
		}
		CurrentPresetAlpha = FMath::Clamp(CurrentPresetAlpha, 0.0f, 1.0f);

		// Manage state transitions
		if (CurrentPresetAlpha <= 0)
		{
			CurrentPreset = TargetPreset;
		}

		// Get post process targets and apply the new settings
		float InterpolatedAlpha = FMath::InterpEaseInOut(0.0f, 1.0f, CurrentPresetAlpha, ENeutronUIConstants::EaseStandard);
		TSharedPtr<FNeutronPostProcessSettingBase>& CurrentPostProcess = PostProcessSettings[0];
		TSharedPtr<FNeutronPostProcessSettingBase>& TargetPostProcess  = PostProcessSettings[CurrentPreset];
		UpdateFunction.ExecuteIfBound(PostProcessVolume, PostProcessMaterials, CurrentPostProcess, TargetPostProcess, CurrentPresetAlpha);

		// Apply config-driven settings
		UNeutronGameUserSettings* GameUserSettings             = Cast<UNeutronGameUserSettings>(GEngine->GetGameUserSettings());
		PostProcessVolume->Settings.bOverride_MotionBlurAmount = true;
		PostProcessVolume->Settings.bOverride_BloomMethod      = true;
		PostProcessVolume->Settings.bOverride_DynamicGlobalIlluminationMethod = true;
		PostProcessVolume->Settings.bOverride_ReflectionMethod                = true;
		PostProcessVolume->Settings.MotionBlurAmount                          = GameUserSettings->MotionBlurAmount;
		PostProcessVolume->Settings.BloomMethod                               = GameUserSettings->EnableCinematicBloom ? BM_FFT : BM_SOG;
		PostProcessVolume->Settings.DynamicGlobalIlluminationMethod =
			GameUserSettings->EnableLumen ? EDynamicGlobalIlluminationMethod::Lumen : EDynamicGlobalIlluminationMethod::ScreenSpace;
		PostProcessVolume->Settings.ReflectionMethod =
			GameUserSettings->EnableLumen ? EReflectionMethod::Lumen : EReflectionMethod::ScreenSpace;
	}
}
