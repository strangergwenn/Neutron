// Neutron - Gwennaël Arbona

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "NeutronGameInstance.generated.h"

/** Game instance class */
UCLASS(ClassGroup = (Neutron))
class NEUTRON_API UNeutronGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:

	UNeutronGameInstance();

	/*----------------------------------------------------
	    Inherited & callbacks
	----------------------------------------------------*/

	virtual void Init() override;

	virtual void Shutdown() override;

	void PreLoadMap(const FString& InMapName);

	/*----------------------------------------------------
	    Game flow
	----------------------------------------------------*/

	/** Restart the game from the level in save data */
	void SetGameOnline(FString URL, bool Online = true, uint32 MaxPlayerCount = 1);

	/** Exit the session and go to the main menu */
	void GoToMainMenu();

	/** Change level on the server */
	void ServerTravel(FString URL);

private:

	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

	// Asset manager object
	UPROPERTY()
	class UNeutronAssetManager* AssetManager;

	// Contract manager object
	UPROPERTY()
	class UNeutronContractManager* ContractManager;

	// Menu manager object
	UPROPERTY()
	class UNeutronMenuManager* MenuManager;

	// Post-processing manager object
	UPROPERTY()
	class UNeutronPostProcessManager* PostProcessManager;

	// Sessions manager object
	UPROPERTY()
	class UNeutronSessionsManager* SessionsManager;

	// Save manager object
	UPROPERTY()
	class UNeutronSaveManager* SaveManager;

	// Sound manager object
	UPROPERTY()
	class UNeutronSoundManager* SoundManager;
};
