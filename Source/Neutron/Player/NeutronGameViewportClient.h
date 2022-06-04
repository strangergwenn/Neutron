// Neutron - Gwennaël Arbona

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameViewportClient.h"
#include "SlateBasics.h"

#include "Neutron/UI/NeutronUI.h"
#include "Neutron/System/NeutronAssetManager.h"

#include "NeutronGameViewportClient.generated.h"

/** Loading screen list */
UCLASS()
class UNeutronLoadingScreenSetup : public UNeutronAssetDescription
{
	GENERATED_BODY()

public:

	// Width of the loading screen in pixels
	UPROPERTY(Category = LoadingScreensSetup, EditDefaultsOnly)
	int32 Width;

	// Height of the loading screen in pixels
	UPROPERTY(Category = LoadingScreensSetup, EditDefaultsOnly)
	int32 Height;

	// Loading screen material
	UPROPERTY(Category = LoadingScreensSetup, EditDefaultsOnly)
	class UMaterial* AnimatedMaterial;

	// Black loading screen
	UPROPERTY(Category = LoadingScreensSetup, EditDefaultsOnly)
	UTexture2D* BlackScreen;

	// Animated loading screen
	UPROPERTY(Category = LoadingScreensSetup, EditDefaultsOnly)
	UTexture2D* LaunchScreen;
};

/** Game viewport class */
UCLASS(ClassGroup = (Neutron))
class NEUTRON_API UNeutronGameViewportClient : public UGameViewportClient
{
	GENERATED_BODY()

public:

	UNeutronGameViewportClient();

	/*----------------------------------------------------
	    Public methods
	----------------------------------------------------*/

	virtual void SetViewport(FViewport* InViewport) override;

	virtual void BeginDestroy();

	virtual void Tick(float DeltaTime) override;

	/** Set the name of the level to load */
	void SetLoadingScreen(ENeutronLoadingScreen LoadingScreen);

	/** Start playing the loading screen through movie player */
	void ShowLoadingScreen();

protected:

	/*----------------------------------------------------
	    Internals
	----------------------------------------------------*/

	/** Set up the material and resources */
	void Initialize();

protected:

	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

	// Loading screen setup
	UPROPERTY()
	const UNeutronLoadingScreenSetup* LoadingScreenSetup;

	// Dynamic material
	UPROPERTY()
	class UMaterialInstanceDynamic* AnimatedMaterialInstance;

	// Widget
	TSharedPtr<class SNeutronLoadingScreen> LoadingScreenWidget;

	// Local state
	ENeutronLoadingScreen CurrentLoadingScreen;
};
