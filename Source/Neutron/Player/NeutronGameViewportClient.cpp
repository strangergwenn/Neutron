// Neutron - Gwennaël Arbona

#include "NeutronGameViewportClient.h"

#include "Neutron/System/NeutronMenuManager.h"
#include "Neutron/UI/Widgets/NeutronLoadingScreen.h"
#include "Neutron/Neutron.h"

#include "MoviePlayer.h"

/*----------------------------------------------------
    Constructor
----------------------------------------------------*/

UNeutronGameViewportClient::UNeutronGameViewportClient() : Super()
{}

/*----------------------------------------------------
    Public methods
----------------------------------------------------*/

void UNeutronGameViewportClient::SetViewport(FViewport* InViewport)
{
	NLOG("UNeutronGameViewportClient::SetViewport");

	Super::SetViewport(InViewport);

	Initialize();

	SetLoadingScreen(ENeutronLoadingScreen::None);
}

void UNeutronGameViewportClient::BeginDestroy()
{
	NLOG("UNeutronGameViewportClient::BeginDestroy");

	AnimatedMaterialInstance = nullptr;
	LoadingScreenWidget.Reset();

	Super::BeginDestroy();
}

void UNeutronGameViewportClient::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Get the current loading screen alpha from the menu manager if valid
	float                Alpha       = 1;
	UNeutronMenuManager* MenuManager = UNeutronMenuManager::Get();
	if (MenuManager)
	{
		Alpha = MenuManager->GetLoadingScreenAlpha();
	}

	// Apply alpha
	if (LoadingScreenWidget.IsValid())
	{
		LoadingScreenWidget->SetFadeAlpha(Alpha);
	}
}

void UNeutronGameViewportClient::SetLoadingScreen(ENeutronLoadingScreen LoadingScreen)
{
	Initialize();

	// Disallow changes during animation
	if (CurrentLoadingScreen != ENeutronLoadingScreen::None && LoadingScreen != CurrentLoadingScreen && LoadingScreenWidget.IsValid() &&
		LoadingScreenWidget->GetFadeAlpha() != 0)
	{
		NERR("UNeutronGameViewportClient::SetLoadingScreen : can't switch loading screen here");
		return;
	}

	// Default to previous screen if none was passed
	if (LoadingScreen == ENeutronLoadingScreen::None)
	{
		if (CurrentLoadingScreen == ENeutronLoadingScreen::None)
		{
			LoadingScreen = ENeutronLoadingScreen::Black;
		}
		else
		{
			LoadingScreen = CurrentLoadingScreen;
		}
	}
	else
	{
		CurrentLoadingScreen = LoadingScreen;
	}

	// NLOG("UNeutronGameViewportClient::SetLoadingScreen : setting %d", static_cast<int32>(LoadingScreen));

	// Setup the loading screen widget with the desired resource
	UTexture2D* LoadingScreenTexture =
		CurrentLoadingScreen == ENeutronLoadingScreen::Launch ? LoadingScreenSetup->LaunchScreen : LoadingScreenSetup->BlackScreen;

	// Set the texture
	NCHECK(AnimatedMaterialInstance);
	NCHECK(LoadingScreenTexture);
	AnimatedMaterialInstance->SetTextureParameterValue("LoadingScreenTexture", LoadingScreenTexture);
}

void UNeutronGameViewportClient::ShowLoadingScreen()
{
	NLOG("UNeutronGameViewportClient::ShowLoadingScreen");

	// Start the loading screen
	FLoadingScreenAttributes LoadingScreen;
	LoadingScreen.bAutoCompleteWhenLoadingCompletes = false;
	LoadingScreen.WidgetLoadingScreen               = LoadingScreenWidget;
	GetMoviePlayer()->SetupLoadingScreen(LoadingScreen);
}

/*----------------------------------------------------
    Internals
----------------------------------------------------*/

void UNeutronGameViewportClient::Initialize()
{
	if (LoadingScreenSetup == nullptr || AnimatedMaterialInstance == nullptr || !LoadingScreenWidget.IsValid())
	{
		NLOG("UNeutronGameViewportClient::Initialize");

		// Find the loading screen setup
		TArray<const UNeutronLoadingScreenSetup*> LoadingScreenSetupList =
			UNeutronAssetManager::Get()->GetAssets<UNeutronLoadingScreenSetup>();
		if (LoadingScreenSetupList.Num())
		{
			LoadingScreenSetup = LoadingScreenSetupList.Last();
			NCHECK(LoadingScreenSetup);

			// Create the material
			if (AnimatedMaterialInstance == nullptr && LoadingScreenSetup && LoadingScreenSetup->AnimatedMaterial)
			{
				AnimatedMaterialInstance = UMaterialInstanceDynamic::Create(LoadingScreenSetup->AnimatedMaterial, this);
				NCHECK(AnimatedMaterialInstance);
			}

			// Build the loading widget using the material instance
			LoadingScreenWidget = SNew(SNeutronLoadingScreen).Settings(LoadingScreenSetup).Material(AnimatedMaterialInstance);
			AddViewportWidgetContent(SNew(SWeakWidget).PossiblyNullContent(LoadingScreenWidget), 1000);
		}
	}
}
