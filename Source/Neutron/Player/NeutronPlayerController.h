// Neutron - Gwennaël Arbona

#pragma once

#include "Neutron/System/NeutronGameInstance.h"
#include "Neutron/System/NeutronPostProcessManager.h"
#include "Neutron/UI/NeutronUI.h"

#include "CoreMinimal.h"
#include "Online.h"
#include "GameFramework/PlayerController.h"

#include "NeutronPlayerController.generated.h"

enum class ENeutronNetworkError : uint8;

/*----------------------------------------------------
    Player class
----------------------------------------------------*/

/** Default player controller class */
UCLASS(ClassGroup = (Neutron))
class NEUTRON_API ANeutronPlayerController : public APlayerController
{
	GENERATED_BODY()

public:

	ANeutronPlayerController();

	/*----------------------------------------------------
	    Inherited
	----------------------------------------------------*/

	virtual void PlayerTick(float DeltaTime) override;

	/*----------------------------------------------------
	    Game flow
	----------------------------------------------------*/

public:

	/** Re-start the current level, keeping the save data */
	void SetGameOnline(bool Online = true, uint32 MaxPlayers = 1);

	/** Exit the session and go to the main menu */
	void GoToMainMenu(bool ShouldSaveGame);

	/** Exit the game */
	void ExitGame();

	/** Save the game */
	virtual void SaveGame()
	{
		NCHECK(false);
	}

	/** Invite a friend to join the game */
	void InviteFriend(TSharedRef<FOnlineFriend> Friend);

	/** Join a friend's game from the menu */
	void JoinFriend(TSharedRef<FOnlineFriend> Friend);

	/** Join a friend's game from an invitation */
	void AcceptInvitation(const FOnlineSessionSearchResult& InviteResult);

	/*----------------------------------------------------
	    Menus
	----------------------------------------------------*/

public:

	/** Is the player on the main menu */
	virtual bool IsOnMainMenu() const;

	/** Is the player restricted to menus */
	virtual bool IsMenuOnly() const;

	/** Check if the player is ready to go */
	virtual bool IsReady() const
	{
		return true;
	}

	/** Show a text notification on the screen */
	virtual void Notify(const FText& Text, const FText& Subtext = FText(), ENeutronNotificationType Type = ENeutronNotificationType::Info)
	{
		NDIS("%s > %s / %s", *Text.ToString(), *Subtext.ToString(), *GetEnumString(Type));
	}

	/*----------------------------------------------------
	    Input
	----------------------------------------------------*/

public:

	virtual void SetupInputComponent() override;

	/** Any key pressed */
	void AnyKey(FKey Key);

	/** Toggle the main menu */
	void ToggleMenuOrQuit();

#if WITH_EDITOR

	// Test
	void OnJoinRandomFriend(TArray<TSharedRef<FOnlineFriend>> FriendList);
	void OnJoinRandomSession(TArray<FOnlineSessionSearchResult> SessionList);
	void TestJoin();

#endif

	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

private:

	// General state
	ENeutronNetworkError LastNetworkError;
};
