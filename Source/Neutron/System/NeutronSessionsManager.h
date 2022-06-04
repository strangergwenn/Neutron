// Neutron - Gwennaël Arbona

#pragma once

#include "CoreMinimal.h"

#include "GameFramework/OnlineReplStructs.h"
#include "Net/UnrealNetwork.h"
#include "Online.h"

#include "NeutronSessionsManager.generated.h"

/** Network states */
UENUM(BlueprintType)
enum class ENeutronNetworkState : uint8
{
	Offline,
	Starting,
	OnlineHost,
	OnlineClient,
	Searching,
	Joining,
	JoiningDestroying,
	Ending
};

/** Network errors */
UENUM(BlueprintType)
enum class ENeutronNetworkError : uint8
{
	// Session errors
	Success,
	CreateFailed,
	StartFailed,
	DestroyFailed,
	ReadFriendsFailed,
	JoinFriendFailed,
	JoinSessionIsFull,
	JoinSessionDoesNotExist,
	JoinSessionConnectionError,

	// Network errors
	NetDriverError,
	ConnectionLost,
	ConnectionTimeout,
	ConnectionFailed,
	VersionMismatch,

	UnknownError,
};

/** Action to process when a session has been destroyed */
struct FNeutronSessionAction
{
	FNeutronSessionAction()
	{}

	// Join a new session
	FNeutronSessionAction(FOnlineSessionSearchResult NewSessionToJoin) : SessionToJoin(NewSessionToJoin)
	{}

	// Create a new offline game
	FNeutronSessionAction(FString NewURL) : URL(NewURL), Online(false)
	{}

	// Create a new online game
	FNeutronSessionAction(FString NewURL, int32 NewMaxNumPlayers, bool NewPublic)
		: URL(NewURL), MaxNumPlayers(NewMaxNumPlayers), Online(true), Public(NewPublic)
	{}

	FOnlineSessionSearchResult SessionToJoin;
	FString                    URL;
	int32                      MaxNumPlayers;
	bool                       Online;
	bool                       Public;
};

// Session delegate
DECLARE_DELEGATE_OneParam(FNeutronOnSessionSearchComplete, TArray<FOnlineSessionSearchResult>);

// Friend delegate
DECLARE_DELEGATE_OneParam(FNeutronOnFriendSearchComplete, TArray<TSharedRef<FOnlineFriend>>);

// Friend delegate
DECLARE_DELEGATE_OneParam(FNeutronOnFriendInviteAccepted, const FOnlineSessionSearchResult&);

/** Game instance class */
UCLASS(ClassGroup = (Neutron))
class NEUTRON_API UNeutronSessionsManager : public UObject
{
	GENERATED_BODY()

public:

	UNeutronSessionsManager();

	/*----------------------------------------------------
	    Inherited & callbacks
	----------------------------------------------------*/

	/** Get the singleton instance */
	static UNeutronSessionsManager* Get()
	{
		return Singleton;
	}

	/** Initialize this class */
	void Initialize(class UNeutronGameInstance* Instance);

	/** Finalize the sessions manager */
	void Finalize();

	/*----------------------------------------------------
	    Session API
	----------------------------------------------------*/

	/** Get the current online subsystem */
	FString GetOnlineSubsystemName() const;

	/** Are we in an online session ? */
	bool IsOnline() const;

	/** Are we currently in a busy state ? */
	bool IsBusy() const;

	/** Start a multiplayer session and open URL as a listen server */
	bool StartSession(FString URL, int32 MaxNumPlayers, bool Public = true);

	/** Finish the online session and open URL as standalone */
	bool EndSession(FString URL);

	/** Mark the session as public or private */
	void SetSessionAdvertised(bool Public);

	/** Search for sessions */
	bool SearchSessions(bool OnLan, FNeutronOnSessionSearchComplete Callback);

	/** Join a session */
	bool JoinSearchResult(const FOnlineSessionSearchResult& SearchResult);

	/** Reset the session errors */
	void ClearErrors();

	/*----------------------------------------------------
	    Friends API
	----------------------------------------------------*/

	/** Set the callback for accepted friend invitations */
	void SetAcceptedInvitationCallback(FNeutronOnFriendInviteAccepted Callback);

	/** Read the list of online friends */
	bool SearchFriends(FNeutronOnFriendSearchComplete Callback);

	/** Invite a friend to the session */
	bool InviteFriend(FUniqueNetIdRepl FriendUserId);

	/** Join a friend's session */
	bool JoinFriend(FUniqueNetIdRepl FriendUserId);

protected:

	/*----------------------------------------------------
	    Session internals
	----------------------------------------------------*/

	/** Session has been created */
	void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);

	/** Session has started */
	void OnStartOnlineGameComplete(FName SessionName, bool bWasSuccessful);

	/** Session has been destroyed */
	void OnDestroySessionComplete(FName SessionName, bool bWasSuccessful);

	/** Sessions have been found */
	void OnFindSessionsComplete(bool bWasSuccessful);

	/** Session has joined */
	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);

	/*----------------------------------------------------
	    Friends internals
	----------------------------------------------------*/

	/** Friend list is available */
	void OnReadFriendsComplete(int32 LocalPlayer, bool bWasSuccessful, const FString& ListName, const FString& ErrorStr);

	/** Invitation was accepted */
	void OnSessionUserInviteAccepted(
		bool bWasSuccess, const int32 ControllerId, TSharedPtr<const FUniqueNetId> UserId, const FOnlineSessionSearchResult& InviteResult);

	/** Friend sessions are available */
	void OnFindFriendSessionComplete(int32 LocalPlayer, bool bWasSuccessful, const TArray<FOnlineSessionSearchResult>& SearchResult);

	/*----------------------------------------------------
	    Internals
	----------------------------------------------------*/

	/** Network error */
	void OnNetworkError(UWorld* World, class UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString);

	/** Session error */
	void OnSessionError(ENeutronNetworkError Type);

	/** Process an action */
	void ProcessAction(FNeutronSessionAction Action);

private:

	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

	// Singleton pointer
	static UNeutronSessionsManager* Singleton;

	// Data
	TSharedPtr<class FOnlineSessionSettings> SessionSettings;
	TSharedPtr<class FOnlineSessionSearch>   SessionSearch;
	FString                                  NextURL;
	FNeutronSessionAction                    ActionAfterDestroy;
	FNeutronSessionAction                    ActionAfterError;
	class UNeutronGameInstance*              GameInstance;

	// Errors
	ENeutronNetworkState NetworkState;
	ENeutronNetworkError LastNetworkError;
	FString              LastNetworkErrorString;

	// Session created
	FOnCreateSessionCompleteDelegate OnCreateSessionCompleteDelegate;
	FDelegateHandle                  OnCreateSessionCompleteDelegateHandle;

	// Session started
	FOnStartSessionCompleteDelegate OnStartSessionCompleteDelegate;
	FDelegateHandle                 OnStartSessionCompleteDelegateHandle;

	// Sessions searched
	FOnFindSessionsCompleteDelegate OnFindSessionsCompleteDelegate;
	FDelegateHandle                 OnFindSessionsCompleteDelegateHandle;

	// Session joined
	FOnJoinSessionCompleteDelegate OnJoinSessionCompleteDelegate;
	FDelegateHandle                OnJoinSessionCompleteDelegateHandle;

	// Session destroyed
	FOnDestroySessionCompleteDelegate OnDestroySessionCompleteDelegate;
	FDelegateHandle                   OnDestroySessionCompleteDelegateHandle;

	// Friend list has been read
	FNeutronOnSessionSearchComplete OnSessionListReady;

	// Friend list is available
	FOnReadFriendsListComplete OnReadFriendsListCompleteDelegate;

	// Friend list has been read
	FNeutronOnFriendSearchComplete OnFriendListReady;

	// Friend invite accepted or joined manually
	FNeutronOnFriendInviteAccepted OnFriendInviteAccepted;

	// Friend invitation accepted through Steam, join that session
	FOnSessionUserInviteAcceptedDelegate OnSessionUserInviteAcceptedDelegate;
	FDelegateHandle                      OnSessionUserInviteAcceptedDelegateHandle;

	// Friend sessions searched
	FOnFindFriendSessionCompleteDelegate OnFindFriendSessionCompleteDelegate;
	FDelegateHandle                      OnFindFriendSessionCompleteDelegateHandle;

public:

	/*----------------------------------------------------
	    Getters
	----------------------------------------------------*/

	ENeutronNetworkState GetNetworkState() const
	{
		return NetworkState;
	}

	ENeutronNetworkError GetNetworkError() const
	{
		return LastNetworkError;
	}

	FText GetNetworkStateString() const;

	FText GetNetworkErrorString() const;
};
