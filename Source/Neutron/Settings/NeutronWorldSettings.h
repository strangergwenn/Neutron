// Neutron - Gwennaël Arbona

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/WorldSettings.h"
#include "NeutronWorldSettings.generated.h"

/** Default game mode class */
UCLASS(ClassGroup = (Neutron))
class NEUTRON_API ANeutronWorldSettings : public AWorldSettings
{
	GENERATED_BODY()

public:

	ANeutronWorldSettings();

	/*----------------------------------------------------
	    Gameplay
	----------------------------------------------------*/

	/** Is the player restricted to menus */
	UFUNCTION(Category = Neutron, BlueprintCallable)
	bool IsMenuMap()
	{
		return MenuOnly;
	}

	/** Is the player on the main menu */
	UFUNCTION(Category = Neutron, BlueprintCallable)
	bool IsMainMenuMap()
	{
		return MainMenu;
	}

	/*----------------------------------------------------
	    Properties
	----------------------------------------------------*/

	/** Menu mode */
	UPROPERTY(EditDefaultsOnly, NoClear, BlueprintReadOnly, Category = Neutron)
	bool MenuOnly;

	/** Main menu mode */
	UPROPERTY(EditDefaultsOnly, NoClear, BlueprintReadOnly, Category = Neutron)
	bool MainMenu;
};
