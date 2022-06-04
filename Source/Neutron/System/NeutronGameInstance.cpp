// Neutron - Gwennaël Arbona

#include "NeutronGameInstance.h"

#include "NeutronAssetManager.h"
#include "NeutronContractManager.h"
#include "NeutronMenuManager.h"
#include "NeutronPostProcessManager.h"
#include "NeutronSoundManager.h"
#include "NeutronSaveManager.h"
#include "NeutronSessionsManager.h"

#include "Neutron/Settings/NeutronGameUserSettings.h"
#include "Neutron/Settings/NeutronWorldSettings.h"
#include "Neutron/Player/NeutronGameViewportClient.h"

#include "Neutron/UI/NeutronUI.h"
#include "Neutron/Neutron.h"

#include "Kismet/GameplayStatics.h"
#include "GameMapsSettings.h"
#include "Engine/NetDriver.h"
#include "Engine/World.h"
#include "Engine.h"

#define LOCTEXT_NAMESPACE "UNeutronGameInstance"

/*----------------------------------------------------
    Constructor
----------------------------------------------------*/

UNeutronGameInstance::UNeutronGameInstance() : Super()
{}

/*----------------------------------------------------
    Inherited
----------------------------------------------------*/

void UNeutronGameInstance::Init()
{
	Super::Init();

	// Apply user settings
	UNeutronGameUserSettings* GameUserSettings = Cast<UNeutronGameUserSettings>(GEngine->GetGameUserSettings());
	GameUserSettings->ApplyCustomGraphicsSettings();

	// Setup connection screen
	FCoreUObjectDelegates::PreLoadMap.AddUObject(this, &UNeutronGameInstance::PreLoadMap);

	// Create asset manager
	AssetManager = NewObject<UNeutronAssetManager>(this, UNeutronAssetManager::StaticClass(), TEXT("AssetManager"));
	NCHECK(AssetManager);
	AssetManager->Initialize(this);

	// Create the contract manager
	ContractManager = NewObject<UNeutronContractManager>(this, UNeutronContractManager::StaticClass(), TEXT("ContractManager"));
	NCHECK(ContractManager);
	ContractManager->Initialize(this);

	// Create the menu manager
	MenuManager = NewObject<UNeutronMenuManager>(this, UNeutronMenuManager::StaticClass(), TEXT("MenuManager"));
	NCHECK(MenuManager);
	MenuManager->Initialize(this);

	// Create the post-process manager
	PostProcessManager = NewObject<UNeutronPostProcessManager>(this, UNeutronPostProcessManager::StaticClass(), TEXT("PostProcessManager"));
	NCHECK(PostProcessManager);
	PostProcessManager->Initialize(this);

	// Create save manager
	SaveManager = NewObject<UNeutronSaveManager>(this, UNeutronSaveManager::StaticClass(), TEXT("SaveManager"));
	NCHECK(SaveManager);
	SaveManager->Initialize(this);

	// Create the sessions  manager
	SessionsManager = NewObject<UNeutronSessionsManager>(this, UNeutronSessionsManager::StaticClass(), TEXT("SessionsManager"));
	NCHECK(SessionsManager);
	SessionsManager->Initialize(this);

	// Create the sound manager
	SoundManager = NewObject<UNeutronSoundManager>(this, UNeutronSoundManager::StaticClass(), TEXT("SoundManager"));
	NCHECK(SoundManager);
	SoundManager->Initialize(this);
}

void UNeutronGameInstance::Shutdown()
{
	SessionsManager->Finalize();
	Super::Shutdown();
}

void UNeutronGameInstance::PreLoadMap(const FString& InMapName)
{
	UNeutronGameViewportClient* Viewport = Cast<UNeutronGameViewportClient>(GetWorld()->GetGameViewport());
	if (Viewport)
	{
		Viewport->ShowLoadingScreen();
	}
}

/*----------------------------------------------------
    Game flow
----------------------------------------------------*/

void UNeutronGameInstance::SetGameOnline(FString URL, bool Online, uint32 MaxPlayerCount)
{
	NLOG("UNeutronGameInstance::SetGameOnline : '%s', online = %d, players = %d", *URL, Online, MaxPlayerCount);

	SessionsManager->ClearErrors();

	// We want to be online and presumably aren't, start a new online session and then travel to the level
	if (Online)
	{
		SessionsManager->StartSession(URL, MaxPlayerCount, true);
	}

	// We are on the main menu and are staying offline, simply travel there
	else if (Cast<ANeutronWorldSettings>(GetWorld()->GetWorldSettings())->IsMainMenuMap())
	{
		GetFirstLocalPlayerController()->ClientTravel(URL, ETravelType::TRAVEL_Absolute, false);
	}

	// We want to be offline and presumably aren't, kill the session and then travel to the level
	else
	{
		SessionsManager->EndSession(URL);
	}
}

void UNeutronGameInstance::GoToMainMenu()
{
	NLOG("UNeutronGameInstance::GoToMainMenu");

	UNeutronSaveManager::Get()->ReleaseCurrentSaveData();

	UGameplayStatics::OpenLevel(GetWorld(), FName(*UGameMapsSettings::GetGameDefaultMap()), true);
}

void UNeutronGameInstance::ServerTravel(FString URL)
{
	GetWorld()->ServerTravel(URL + TEXT("?listen"), true);
}

#undef LOCTEXT_NAMESPACE
