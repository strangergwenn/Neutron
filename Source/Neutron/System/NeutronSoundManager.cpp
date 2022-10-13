// Neutron - Gwennaël Arbona

#include "NeutronSoundManager.h"

#include "NeutronAssetManager.h"
#include "NeutronMenuManager.h"

#include "Neutron/Player/NeutronPlayerController.h"
#include "Neutron/Settings/NeutronGameUserSettings.h"
#include "Neutron/UI/NeutronUI.h"
#include "Neutron/Neutron.h"

#include "Components/AudioComponent.h"
#include "AudioDevice.h"

// Statics
UNeutronSoundManager* UNeutronSoundManager::Singleton = nullptr;

/*----------------------------------------------------
    Audio player instance
----------------------------------------------------*/

FNeutronSoundInstance::FNeutronSoundInstance(
	UObject* Owner, FNeutronSoundInstanceCallback Callback, USoundBase* Sound, bool ChangePitchWithFade, float FadeSpeed)
	: StateCallback(Callback), SoundPitchFade(ChangePitchWithFade), SoundFadeSpeed(FadeSpeed), CurrentVolume(0.0f)
{
	// Create the sound component
	NCHECK(Owner);
	SoundComponent = NewObject<UAudioComponent>(Owner, UAudioComponent::StaticClass());
	NCHECK(SoundComponent);
	SoundComponent->RegisterComponent();

	// Set up the sound component
	SoundComponent->Sound         = Sound;
	SoundComponent->bAutoActivate = false;
	SoundComponent->bAutoDestroy  = false;
}

void FNeutronSoundInstance::Update(float DeltaTime)
{
	if (IsValid())
	{
		// Check the state
		bool ShouldPlay = false;
		if (StateCallback.IsBound())
		{
			ShouldPlay = StateCallback.Execute();
		}

		// Determine new volume
		float VolumeDelta = (ShouldPlay ? 1.0f : -1.0f) * DeltaTime;
		float NewVolume   = FMath::Clamp(CurrentVolume + VolumeDelta * SoundFadeSpeed, 0.0f, 1.0f);

		// Update playback
		if (NewVolume != CurrentVolume)
		{
			if (CurrentVolume == 0 && NewVolume != 0)
			{
				SoundComponent->Play();
			}
			else
			{
				SoundComponent->SetVolumeMultiplier(NewVolume);
				if (SoundPitchFade)
				{
					SoundComponent->SetPitchMultiplier(0.5f + 0.5f * NewVolume);
				}
			}
			CurrentVolume = NewVolume;
		}
		else if (CurrentVolume == 0)
		{
			SoundComponent->Stop();
		}
		else if (!SoundComponent->IsPlaying())
		{
			SoundComponent->Play();
		}
	}
}

bool FNeutronSoundInstance::IsValid()
{
	return ::IsValid(SoundComponent);
}

bool FNeutronSoundInstance::IsIdle()
{
	return !IsValid() || !SoundComponent->IsPlaying();
}

/*----------------------------------------------------
    Constructor
----------------------------------------------------*/

UNeutronSoundManager::UNeutronSoundManager()
	: Super()

	, PlayerController(nullptr)

	, AudioDevice()
	, CurrentMusicTrack(NAME_None)
	, DesiredMusicTrack(NAME_None)

	, MasterVolume(1.0f)
	, UIVolume(1.0f)
	, EffectsVolume(1.0f)
	, EffectsVolumeMultiplier(1.0f)
	, MusicVolume(1.0f)
{}

/*----------------------------------------------------
    Public methods
----------------------------------------------------*/

void UNeutronSoundManager::BeginPlay(ANeutronPlayerController* PC, FNeutronMusicCallback Callback)
{
	NLOG("UNeutronSoundManager::BeginPlay");

	// Get references
	PlayerController = PC;
	MusicCallback    = Callback;

	// Get basic game pointers
	const UNeutronGameUserSettings* GameUserSettings = Cast<UNeutronGameUserSettings>(GEngine->GetGameUserSettings());
	NCHECK(GameUserSettings);
	SoundSetup = UNeutronAssetManager::Get()->GetDefaultAsset<UNeutronSoundSetup>();

	// Be safe
	NCHECK(SoundSetup->MasterSoundMix);
	NCHECK(SoundSetup->MasterSoundClass);
	NCHECK(SoundSetup->UISoundClass);
	NCHECK(SoundSetup->EffectsSoundClass);
	NCHECK(SoundSetup->MusicSoundClass);

	// Fetch and map the musical tracks
	MusicCatalog.Empty();
	if (SoundSetup)
	{
		for (const FNeutronMusicCatalogEntry& Entry : SoundSetup->Tracks)
		{
			MusicCatalog.Add(Entry.Name, Entry.Tracks);
		}
	}

	// Initialize the sound device and master mix
	AudioDevice = PC->GetWorld()->GetAudioDeviceRaw();
	if (AudioDevice)
	{
		AudioDevice->SetBaseSoundMix(SoundSetup->MasterSoundMix);
	}

	// Setup sound settings
	SetMasterVolume(GameUserSettings->MasterVolume);
	SetUIVolume(GameUserSettings->UIVolume);
	SetEffectsVolume(GameUserSettings->EffectsVolume);
	SetMusicVolume(GameUserSettings->MusicVolume);

	// Initialize the music instance
	EnvironmentSoundInstances.Empty();
	MusicSoundInstance = FNeutronSoundInstance(PlayerController,    //
		FNeutronSoundInstanceCallback::CreateLambda(
			[this]()
			{
				return CurrentMusicTrack == DesiredMusicTrack;
			}),
		nullptr, false, SoundSetup->MusicFadeSpeed);
}

void UNeutronSoundManager::Mute()
{
	NLOG("UNeutronSoundManager::Mute");

	if (AudioDevice)
	{
		AudioDevice->SetSoundMixClassOverride(
			SoundSetup->MasterSoundMix, SoundSetup->MasterSoundClass, 0.0f, 1.0f, ENeutronUIConstants::FadeDurationShort, true);
	}
}

void UNeutronSoundManager::UnMute()
{
	NLOG("UNeutronSoundManager::UnMute");

	if (AudioDevice)
	{
		AudioDevice->SetSoundMixClassOverride(
			SoundSetup->MasterSoundMix, SoundSetup->MasterSoundClass, MasterVolume, 1.0f, ENeutronUIConstants::FadeDurationShort, true);
	}
}

void UNeutronSoundManager::AddEnvironmentSound(
	FName SoundName, FNeutronSoundInstanceCallback Callback, bool ChangePitchWithFade, float FadeSpeed)
{
	NCHECK(IsValid(PlayerController));

	const FNeutronEnvironmentSoundEntry* EnvironmentSound = SoundSetup->Sounds.Find(SoundName);
	if (EnvironmentSound)
	{
		EnvironmentSoundInstances.Add(FNeutronSoundInstance(
			PlayerController, Callback, EnvironmentSound->Sound, EnvironmentSound->ChangePitchWithFade, EnvironmentSound->SoundFadeSpeed));
	}
}

void UNeutronSoundManager::SetMasterVolume(int32 Volume)
{
	NLOG("UNeutronSoundManager::SetMasterVolume %d", Volume);

	MasterVolume = FMath::Clamp(Volume / 10.0f, 0.0f, 1.0f);

	if (AudioDevice)
	{
		AudioDevice->SetSoundMixClassOverride(
			SoundSetup->MasterSoundMix, SoundSetup->MasterSoundClass, MasterVolume, 1.0f, ENeutronUIConstants::FadeDurationLong, true);
	}
}

void UNeutronSoundManager::SetUIVolume(int32 Volume)
{
	NLOG("UNeutronSoundManager::SetUIVolume %d", Volume);

	UIVolume = FMath::Clamp(Volume / 10.0f, 0.0f, 1.0f);

	if (AudioDevice)
	{
		AudioDevice->SetSoundMixClassOverride(
			SoundSetup->MasterSoundMix, SoundSetup->UISoundClass, UIVolume, 1.0f, ENeutronUIConstants::FadeDurationLong, true);
	}
}

void UNeutronSoundManager::SetEffectsVolume(int32 Volume)
{
	NLOG("UNeutronSoundManager::SetEffectsVolume %d", Volume);

	EffectsVolume = FMath::Clamp(Volume / 10.0f, 0.0f, 1.0f);

	// Do not set the volume since it's done on tick
}

void UNeutronSoundManager::SetMusicVolume(int32 Volume)
{
	NLOG("UNeutronSoundManager::SetMusicVolume %d", Volume);

	MusicVolume = FMath::Clamp(Volume / 10.0f, 0.0f, 1.0f);

	if (AudioDevice)
	{
		AudioDevice->SetSoundMixClassOverride(
			SoundSetup->MasterSoundMix, SoundSetup->MusicSoundClass, MusicVolume, 1.0f, ENeutronUIConstants::FadeDurationLong, true);
	}
}

/*----------------------------------------------------
    Tick
----------------------------------------------------*/

void UNeutronSoundManager::Tick(float DeltaTime)
{
	// Control the music track
	if (MusicSoundInstance.IsValid() && MasterVolume > 0)
	{
		DesiredMusicTrack = MusicCallback.IsBound() ? MusicCallback.Execute() : NAME_None;

		if (CurrentMusicTrack != DesiredMusicTrack || MusicSoundInstance.IsIdle())
		{
			int32 TrackIndex = FMath::RandHelper(MusicCatalog[DesiredMusicTrack].Num());

			NLOG("UNeutronSoundManager::Tick : switching track from '%s' to '%s' %d", *CurrentMusicTrack.ToString(),
				*DesiredMusicTrack.ToString(), TrackIndex);

			MusicSoundInstance.SoundComponent->SetSound(MusicCatalog[DesiredMusicTrack][TrackIndex]);
			CurrentMusicTrack = DesiredMusicTrack;
		}

		MusicSoundInstance.Update(DeltaTime);
	}

	// Update all sound instances
	for (FNeutronSoundInstance& Sound : EnvironmentSoundInstances)
	{
		Sound.Update(DeltaTime);
	}

	// Check if we should fade out audio effects
	if (AudioDevice)
	{
		UNeutronMenuManager* MenuManager = UNeutronMenuManager::Get();

		if (MenuManager->IsMenuOpening() && SoundSetup->FadeEffectsInMenus)
		{
			EffectsVolumeMultiplier -= DeltaTime / ENeutronUIConstants::FadeDurationShort;
		}
		else
		{
			EffectsVolumeMultiplier += DeltaTime / ENeutronUIConstants::FadeDurationShort;
		}
		EffectsVolumeMultiplier = FMath::Clamp(EffectsVolumeMultiplier, 0.01f, 1.0f);

		AudioDevice->SetSoundMixClassOverride(
			SoundSetup->MasterSoundMix, SoundSetup->EffectsSoundClass, EffectsVolumeMultiplier * EffectsVolume, 1.0f, 0.0f, true);
	}
}
