// Neutron - Gwennaël Arbona

#include "NeutronSessionsManager.h"

#include "NeutronGameInstance.h"

#include "Neutron/UI/NeutronUI.h"
#include "Neutron/Neutron.h"

#include "Kismet/GameplayStatics.h"
#include "Engine/NetDriver.h"
#include "Engine/World.h"
#include "Engine.h"

#define LOCTEXT_NAMESPACE "UNeutronSessionsManager"

// Statics
UNeutronSessionsManager* UNeutronSessionsManager::Singleton = nullptr;

/*----------------------------------------------------
    Constructor
----------------------------------------------------*/

UNeutronSessionsManager::UNeutronSessionsManager() : Super(), NetworkState(ENeutronNetworkState::Offline)
{
	// Session callbacks
	OnCreateSessionCompleteDelegate =
		FOnCreateSessionCompleteDelegate::CreateUObject(this, &UNeutronSessionsManager::OnCreateSessionComplete);
	OnStartSessionCompleteDelegate =
		FOnStartSessionCompleteDelegate::CreateUObject(this, &UNeutronSessionsManager::OnStartOnlineGameComplete);
	OnFindSessionsCompleteDelegate = FOnFindSessionsCompleteDelegate::CreateUObject(this, &UNeutronSessionsManager::OnFindSessionsComplete);
	OnJoinSessionCompleteDelegate  = FOnJoinSessionCompleteDelegate::CreateUObject(this, &UNeutronSessionsManager::OnJoinSessionComplete);
	OnDestroySessionCompleteDelegate =
		FOnDestroySessionCompleteDelegate::CreateUObject(this, &UNeutronSessionsManager::OnDestroySessionComplete);

	// Friends callbacks
	OnReadFriendsListCompleteDelegate = FOnReadFriendsListComplete::CreateUObject(this, &UNeutronSessionsManager::OnReadFriendsComplete);
	OnSessionUserInviteAcceptedDelegate =
		FOnSessionUserInviteAcceptedDelegate::CreateUObject(this, &UNeutronSessionsManager::OnSessionUserInviteAccepted);
	OnFindFriendSessionCompleteDelegate =
		FOnFindFriendSessionCompleteDelegate::CreateUObject(this, &UNeutronSessionsManager::OnFindFriendSessionComplete);
}

/*----------------------------------------------------
    Inherited
----------------------------------------------------*/

void UNeutronSessionsManager::Initialize(UNeutronGameInstance* Instance)
{
	Singleton    = this;
	GameInstance = Instance;

	// Setup network errors
	GameInstance->GetEngine()->OnNetworkFailure().AddUObject(this, &UNeutronSessionsManager::OnNetworkError);

	// Setup friends invites
	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	if (OnlineSub)
	{
		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
		OnSessionUserInviteAcceptedDelegateHandle =
			Sessions->AddOnSessionUserInviteAcceptedDelegate_Handle(OnSessionUserInviteAcceptedDelegate);
	}
}

void UNeutronSessionsManager::Finalize()
{
	// Clean up friends invites
	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	if (OnlineSub)
	{
		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
		Sessions->ClearOnSessionUserInviteAcceptedDelegate_Handle(OnSessionUserInviteAcceptedDelegateHandle);
	}
}

/*----------------------------------------------------
    Sessions API
----------------------------------------------------*/

FString UNeutronSessionsManager::GetOnlineSubsystemName() const
{
	IOnlineSubsystem* const OnlineSub = IOnlineSubsystem::Get();

	if (OnlineSub)
	{
		return OnlineSub->GetOnlineServiceName().ToString();
	}

	return FString("Null");
}

bool UNeutronSessionsManager::IsOnline() const
{
	IOnlineSubsystem* const OnlineSub = IOnlineSubsystem::Get();

	if (OnlineSub)
	{
		ULocalPlayer*     Player   = GameInstance->GetFirstGamePlayer();
		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
		FUniqueNetIdRepl  UserId   = Player->GetPreferredUniqueNetId();

		if (Sessions.IsValid() && UserId.IsValid())
		{
			return Sessions->IsPlayerInSession(NAME_GameSession, *UserId);
		}
	}

	return false;
}

bool UNeutronSessionsManager::IsBusy() const
{
	return (NetworkState != ENeutronNetworkState::Offline && NetworkState != ENeutronNetworkState::OnlineClient &&
			NetworkState != ENeutronNetworkState::OnlineHost);
}

bool UNeutronSessionsManager::StartSession(FString URL, int32 MaxNumPlayers, bool Public)
{
	NLOG("UNeutronSessionsManager::CreateSession");

	IOnlineSubsystem* const OnlineSub = IOnlineSubsystem::Get();

	if (OnlineSub)
	{
		ULocalPlayer*     Player   = GameInstance->GetFirstGamePlayer();
		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
		FUniqueNetIdRepl  UserId   = Player->GetPreferredUniqueNetId();

		if (Sessions.IsValid() && UserId.IsValid())
		{
			ActionAfterError = FNeutronSessionAction(GetWorld()->GetName());

			// Player is already in session, leave it
			if (Sessions->IsPlayerInSession(NAME_GameSession, *UserId))
			{
				ActionAfterDestroy = FNeutronSessionAction(URL, MaxNumPlayers, Public);
				NetworkState       = ENeutronNetworkState::JoiningDestroying;

				OnDestroySessionCompleteDelegateHandle =
					Sessions->AddOnDestroySessionCompleteDelegate_Handle(OnDestroySessionCompleteDelegate);
				return Sessions->DestroySession(NAME_GameSession);
			}

			// Player isn't in a session, create it
			else
			{
				SessionSettings = MakeShared<FOnlineSessionSettings>();

				SessionSettings->NumPublicConnections             = MaxNumPlayers;
				SessionSettings->NumPrivateConnections            = 0;
				SessionSettings->bIsLANMatch                      = false;
				SessionSettings->bUsesPresence                    = false;
				SessionSettings->bAllowInvites                    = true;
				SessionSettings->bAllowJoinInProgress             = true;
				SessionSettings->bShouldAdvertise                 = Public;
				SessionSettings->bAllowJoinViaPresence            = true;
				SessionSettings->bAllowJoinViaPresenceFriendsOnly = false;

				NextURL = URL;
				SessionSettings->Set(SETTING_MAPNAME, URL, EOnlineDataAdvertisementType::ViaOnlineService);

				NetworkState = ENeutronNetworkState::Starting;

				// Start
				OnCreateSessionCompleteDelegateHandle =
					Sessions->AddOnCreateSessionCompleteDelegate_Handle(OnCreateSessionCompleteDelegate);
				return Sessions->CreateSession(*UserId, NAME_GameSession, *SessionSettings);
			}
		}
	}

	return false;
}

bool UNeutronSessionsManager::EndSession(FString URL)
{
	NLOG("UNeutronSessionsManager::EndSession");

	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();

	if (OnlineSub)
	{
		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();

		// Destroy
		if (Sessions.IsValid())
		{
			ActionAfterError   = FNeutronSessionAction(GetWorld()->GetName());
			ActionAfterDestroy = FNeutronSessionAction(URL);
			NetworkState       = ENeutronNetworkState::Ending;

			OnDestroySessionCompleteDelegateHandle = Sessions->AddOnDestroySessionCompleteDelegate_Handle(OnDestroySessionCompleteDelegate);
			return Sessions->DestroySession(NAME_GameSession);
		}
	}

	return false;
}

void UNeutronSessionsManager::SetSessionAdvertised(bool Public)
{
	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();

	if (OnlineSub)
	{
		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
		if (Sessions.IsValid())
		{
			FOnlineSessionSettings* Settings = Sessions->GetSessionSettings(NAME_GameSession);

			if (Settings && Settings->bShouldAdvertise != Public)
			{
				NLOG("UNeutronSessionsManager::SetSessionAdvertised %d", Public);

				Settings->bShouldAdvertise = Public;
				Sessions->UpdateSession(NAME_GameSession, *Settings, true);
			}
		}
	}
}

bool UNeutronSessionsManager::SearchSessions(bool OnLan, FNeutronOnSessionSearchComplete Callback)
{
	NLOG("UNeutronSessionsManager::SearchSessions");

	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();

	OnSessionListReady = Callback;

	if (OnlineSub)
	{
		ULocalPlayer*     Player   = GameInstance->GetFirstGamePlayer();
		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
		FUniqueNetIdRepl  UserId   = Player->GetPreferredUniqueNetId();

		// Start searching
		if (Sessions.IsValid() && UserId.IsValid())
		{
			SessionSearch = MakeShared<FOnlineSessionSearch>();

			SessionSearch->bIsLanQuery      = OnLan;
			SessionSearch->MaxSearchResults = 10;
			SessionSearch->PingBucketSize   = 100;
			SessionSearch->TimeoutInSeconds = 3;

			SessionSearch->QuerySettings.Set(SEARCH_PRESENCE, true, EOnlineComparisonOp::Equals);

			NetworkState = ENeutronNetworkState::Searching;

			// Start
			TSharedRef<FOnlineSessionSearch> SearchSettingsRef = SessionSearch.ToSharedRef();
			OnFindSessionsCompleteDelegateHandle = Sessions->AddOnFindSessionsCompleteDelegate_Handle(OnFindSessionsCompleteDelegate);
			return Sessions->FindSessions(*UserId, SearchSettingsRef);
		}
	}

	return false;
}

bool UNeutronSessionsManager::JoinSearchResult(const FOnlineSessionSearchResult& SearchResult)
{
	NLOG("UNeutronSessionsManager::JoinSearchResult");

	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();

	if (OnlineSub)
	{
		ULocalPlayer*     Player   = GameInstance->GetFirstGamePlayer();
		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
		FUniqueNetIdRepl  UserId   = Player->GetPreferredUniqueNetId();

		// Start joining
		if (Sessions.IsValid() && UserId.IsValid())
		{
			// Player is already in session, leave it
			if (Sessions->IsPlayerInSession(NAME_GameSession, *UserId))
			{
				OnDestroySessionCompleteDelegateHandle =
					Sessions->AddOnDestroySessionCompleteDelegate_Handle(OnDestroySessionCompleteDelegate);

				ActionAfterError   = FNeutronSessionAction(GetWorld()->GetName());
				ActionAfterDestroy = FNeutronSessionAction(SearchResult);

				NetworkState = ENeutronNetworkState::JoiningDestroying;

				return Sessions->DestroySession(NAME_GameSession);
			}

			// Join directly
			else
			{
				NetworkState = ENeutronNetworkState::Joining;

				OnJoinSessionCompleteDelegateHandle = Sessions->AddOnJoinSessionCompleteDelegate_Handle(OnJoinSessionCompleteDelegate);

				return Sessions->JoinSession(*Player->GetPreferredUniqueNetId(), NAME_GameSession, SearchResult);
			}
		}
	}

	return false;
}

void UNeutronSessionsManager::ClearErrors()
{
	LastNetworkError       = ENeutronNetworkError::Success;
	LastNetworkErrorString = "";
}

/*----------------------------------------------------
    Friends API
----------------------------------------------------*/

void UNeutronSessionsManager::SetAcceptedInvitationCallback(FNeutronOnFriendInviteAccepted Callback)
{
	OnFriendInviteAccepted = Callback;
}

bool UNeutronSessionsManager::SearchFriends(FNeutronOnFriendSearchComplete Callback)
{
	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();

	OnFriendListReady = Callback;

	if (OnlineSub && OnlineSub->GetFriendsInterface())
	{
		ULocalPlayer* Player = GameInstance->GetFirstGamePlayer();

		return OnlineSub->GetFriendsInterface()->ReadFriendsList(
			Player->GetControllerId(), EFriendsLists::ToString(EFriendsLists::Default), OnReadFriendsListCompleteDelegate);
	}

	return false;
}

bool UNeutronSessionsManager::InviteFriend(FUniqueNetIdRepl FriendUserId)
{
	NLOG("UNeutronSessionsManager::InviteFriend");

	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();

	if (OnlineSub)
	{
		ULocalPlayer*     Player   = GameInstance->GetFirstGamePlayer();
		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
		FUniqueNetIdRepl  UserId   = Player->GetPreferredUniqueNetId();

		// Send invite
		return Sessions->SendSessionInviteToFriend(*UserId, NAME_GameSession, *FriendUserId);
	}

	return false;
}

bool UNeutronSessionsManager::JoinFriend(FUniqueNetIdRepl FriendUserId)
{
	NLOG("UNeutronSessionsManager::JoinFriend");

	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();

	if (OnlineSub)
	{
		ULocalPlayer*     Player   = GameInstance->GetFirstGamePlayer();
		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
		FUniqueNetIdRepl  UserId   = Player->GetPreferredUniqueNetId();

		// Find the session and join
		ActionAfterError = FNeutronSessionAction(GetWorld()->GetName());
		OnFindFriendSessionCompleteDelegateHandle =
			Sessions->AddOnFindFriendSessionCompleteDelegate_Handle(Player->GetControllerId(), OnFindFriendSessionCompleteDelegate);
		return Sessions->FindFriendSession(*UserId, *FriendUserId);
	}

	return false;
}

/*----------------------------------------------------
    Session internals
----------------------------------------------------*/

void UNeutronSessionsManager::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	NLOG("UNeutronSessionsManager::OnCreateSessionComplete %s %d", *SessionName.ToString(), bWasSuccessful);

	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	NCHECK(OnlineSub);
	IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();

	// Clear delegate, start session
	if (Sessions.IsValid())
	{
		Sessions->ClearOnCreateSessionCompleteDelegate_Handle(OnCreateSessionCompleteDelegateHandle);
		if (bWasSuccessful)
		{
			OnStartSessionCompleteDelegateHandle = Sessions->AddOnStartSessionCompleteDelegate_Handle(OnStartSessionCompleteDelegate);
			Sessions->StartSession(SessionName);
		}
		else
		{
			NetworkState = ENeutronNetworkState::Offline;
			OnSessionError(ENeutronNetworkError::StartFailed);
		}
	}
}

void UNeutronSessionsManager::OnStartOnlineGameComplete(FName SessionName, bool bWasSuccessful)
{
	NLOG("UNeutronSessionsManager::OnStartOnlineGameComplete %d", bWasSuccessful);

	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	NCHECK(OnlineSub);

	// Clear delegate
	IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
	if (Sessions.IsValid())
	{
		Sessions->ClearOnStartSessionCompleteDelegate_Handle(OnStartSessionCompleteDelegateHandle);
	}

	// Travel to listen server
	if (bWasSuccessful)
	{
		NetworkState = ENeutronNetworkState::OnlineHost;

		GameInstance->GetFirstLocalPlayerController()->ClientTravel(NextURL + TEXT("?listen"), ETravelType::TRAVEL_Absolute, false);

		NextURL = FString();
	}
	else
	{
		NetworkState = ENeutronNetworkState::Offline;
		OnSessionError(ENeutronNetworkError::StartFailed);
	}
}

void UNeutronSessionsManager::OnDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	NLOG("UNeutronSessionsManager::OnDestroySessionComplete %d", bWasSuccessful);

	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	NCHECK(OnlineSub);

	IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
	if (Sessions.IsValid())
	{
		Sessions->ClearOnDestroySessionCompleteDelegate_Handle(OnDestroySessionCompleteDelegateHandle);

		if (bWasSuccessful)
		{
			ProcessAction(ActionAfterDestroy);
			return;
		}
	}

	// An error happened
	NetworkState = ENeutronNetworkState::Offline;
	OnSessionError(ENeutronNetworkError::DestroyFailed);
}

void UNeutronSessionsManager::OnFindSessionsComplete(bool bWasSuccessful)
{
	NLOG("UNeutronSessionsManager::OnFindSessionsComplete %d", bWasSuccessful);

	IOnlineSubsystem* const OnlineSub = IOnlineSubsystem::Get();
	NCHECK(OnlineSub);

	IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
	if (Sessions.IsValid())
	{
		Sessions->ClearOnFindSessionsCompleteDelegate_Handle(OnFindSessionsCompleteDelegateHandle);

		EOnlineSessionState::Type SessionState = Sessions->GetSessionState(NAME_GameSession);
		if (SessionState == EOnlineSessionState::NoSession || SessionState == EOnlineSessionState::Ended)
		{
			NetworkState = ENeutronNetworkState::Offline;
		}
		else
		{
			NetworkState = ENeutronNetworkState::OnlineHost;
		}

		OnSessionListReady.ExecuteIfBound(SessionSearch->SearchResults);
	}
}

void UNeutronSessionsManager::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	NLOG("UNeutronSessionsManager::OnJoinSessionComplete");

	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	NCHECK(OnlineSub);

	IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
	if (Sessions.IsValid())
	{
		// Clear delegate
		Sessions->ClearOnJoinSessionCompleteDelegate_Handle(OnJoinSessionCompleteDelegateHandle);

		// Travel to server
		if (Result == EOnJoinSessionCompleteResult::Success)
		{
			NetworkState = ENeutronNetworkState::OnlineClient;

			FString TravelURL;
			if (Sessions->GetResolvedConnectString(SessionName, TravelURL))
			{
				GameInstance->GetFirstLocalPlayerController()->ClientTravel(TravelURL, ETravelType::TRAVEL_Absolute, false);
			}
		}

		// Handle error
		else
		{
			EOnlineSessionState::Type SessionState = Sessions->GetSessionState(SessionName);
			if (SessionState == EOnlineSessionState::NoSession || SessionState == EOnlineSessionState::Ended)
			{
				NetworkState = ENeutronNetworkState::Offline;
			}
			else
			{
				NetworkState = ENeutronNetworkState::OnlineHost;
			}

			switch (Result)
			{
				case EOnJoinSessionCompleteResult::SessionIsFull:
					OnSessionError(ENeutronNetworkError::JoinSessionIsFull);
					break;
				case EOnJoinSessionCompleteResult::SessionDoesNotExist:
					OnSessionError(ENeutronNetworkError::JoinSessionDoesNotExist);
					break;
				case EOnJoinSessionCompleteResult::CouldNotRetrieveAddress:
					OnSessionError(ENeutronNetworkError::JoinSessionConnectionError);
					break;
				default:
					OnSessionError(ENeutronNetworkError::UnknownError);
					break;
			}
		}
	}
}

/*----------------------------------------------------
    Friends internals
----------------------------------------------------*/

void UNeutronSessionsManager::OnReadFriendsComplete(
	int32 LocalPlayer, bool bWasSuccessful, const FString& ListName, const FString& ErrorStr)
{
	TArray<TSharedRef<FOnlineFriend>> Friends;

	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	if (OnlineSub && OnlineSub->GetFriendsInterface().IsValid() && bWasSuccessful)
	{
		ULocalPlayer*     Player   = GameInstance->GetFirstGamePlayer();
		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();

		OnlineSub->GetFriendsInterface()->GetFriendsList(
			Player->GetControllerId(), EFriendsLists::ToString(EFriendsLists::Default), Friends);
	}

	OnFriendListReady.ExecuteIfBound(Friends);
}

void UNeutronSessionsManager::OnSessionUserInviteAccepted(
	bool bWasSuccess, const int32 ControllerId, TSharedPtr<const FUniqueNetId> UserId, const FOnlineSessionSearchResult& InviteResult)
{
	if (bWasSuccess)
	{
		OnFriendInviteAccepted.ExecuteIfBound(InviteResult);
	}
}

void UNeutronSessionsManager::OnFindFriendSessionComplete(
	int32 LocalPlayer, bool bWasSuccessful, const TArray<FOnlineSessionSearchResult>& SearchResult)
{
	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	NCHECK(OnlineSub);

	IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
	if (Sessions.IsValid())
	{
		Sessions->ClearOnFindFriendSessionCompleteDelegate_Handle(LocalPlayer, OnFindFriendSessionCompleteDelegateHandle);

		// Join session
		if (bWasSuccessful && SearchResult.Num() > 0 && SearchResult[0].Session.OwningUserId.IsValid() &&
			SearchResult[0].Session.SessionInfo.IsValid())
		{
			JoinSearchResult(SearchResult[0]);
		}

		// Handle error
		else
		{
			EOnlineSessionState::Type SessionState = Sessions->GetSessionState(NAME_GameSession);
			if (SessionState == EOnlineSessionState::NoSession || SessionState == EOnlineSessionState::Ended)
			{
				NetworkState = ENeutronNetworkState::Offline;
			}
			else
			{
				NetworkState = ENeutronNetworkState::OnlineHost;
			}

			OnSessionError(ENeutronNetworkError::JoinFriendFailed);
		}
	}
}

/*----------------------------------------------------
    Internals
----------------------------------------------------*/

void UNeutronSessionsManager::OnNetworkError(
	UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString)
{
	NLOG("UNeutronSessionsManager::OnNetworkError %d, '%s'", static_cast<int>(FailureType), *ErrorString);

	if (LastNetworkError != ENeutronNetworkError::Success)
	{
		return;
	}

	LastNetworkErrorString = ErrorString;

	switch (FailureType)
	{
		case ENetworkFailure::NetDriverAlreadyExists:
		case ENetworkFailure::NetDriverCreateFailure:
		case ENetworkFailure::NetDriverListenFailure:
			LastNetworkError = ENeutronNetworkError::NetDriverError;
			break;

		case ENetworkFailure::ConnectionLost:
		case ENetworkFailure::PendingConnectionFailure:
			LastNetworkError = ENeutronNetworkError::ConnectionLost;
			break;

		case ENetworkFailure::ConnectionTimeout:
			LastNetworkError = ENeutronNetworkError::ConnectionTimeout;
			break;

		case ENetworkFailure::FailureReceived:
			LastNetworkError = ENeutronNetworkError::ConnectionFailed;
			break;

		case ENetworkFailure::OutdatedClient:
		case ENetworkFailure::OutdatedServer:
		case ENetworkFailure::NetGuidMismatch:
		case ENetworkFailure::NetChecksumMismatch:
			LastNetworkError = ENeutronNetworkError::VersionMismatch;
			break;
	}

	EndSession("");
}

void UNeutronSessionsManager::OnSessionError(ENeutronNetworkError Type)
{
	NLOG("UNeutronSessionsManager::OnSessionError");

	LastNetworkError       = Type;
	LastNetworkErrorString = "";

	ProcessAction(ActionAfterError);
}

void UNeutronSessionsManager::ProcessAction(FNeutronSessionAction Action)
{
	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	NCHECK(OnlineSub);

	IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
	if (Sessions.IsValid())
	{
		// We killed the previous session to join a specific one, join it
		if (Action.SessionToJoin.IsValid())
		{
			NetworkState = ENeutronNetworkState::Joining;

			ULocalPlayer* Player                = GameInstance->GetFirstGamePlayer();
			OnJoinSessionCompleteDelegateHandle = Sessions->AddOnJoinSessionCompleteDelegate_Handle(OnJoinSessionCompleteDelegate);

			Sessions->JoinSession(*Player->GetPreferredUniqueNetId(), NAME_GameSession, Action.SessionToJoin);
		}

		// We are playing alone, reload a level
		else if (Action.URL.Len())
		{
			// New online session
			if (Action.Online)
			{
				StartSession(Action.URL, Action.MaxNumPlayers, Action.Public);
			}

			// Exit multiplayer and go back to a level
			else
			{
				NetworkState = ENeutronNetworkState::Offline;

				GameInstance->GetFirstLocalPlayerController()->ClientTravel(Action.URL, ETravelType::TRAVEL_Absolute, false);
			}
		}
	}
}

/*----------------------------------------------------
    Getters
----------------------------------------------------*/

FText UNeutronSessionsManager::GetNetworkStateString() const
{
	switch (NetworkState)
	{
		case ENeutronNetworkState::Starting:
			return LOCTEXT("Starting", "Starting");
		case ENeutronNetworkState::OnlineHost:
			return LOCTEXT("OnlineHost", "Hosting");
		case ENeutronNetworkState::OnlineClient:
			return LOCTEXT("OnlineClient", "Connected");
		case ENeutronNetworkState::Searching:
			return LOCTEXT("Searching", "Searching");
		case ENeutronNetworkState::Joining:
			return LOCTEXT("Joining", "Joining");
		case ENeutronNetworkState::JoiningDestroying:
			return LOCTEXT("JoiningDestroying", "Joining");
		case ENeutronNetworkState::Ending:
			return LOCTEXT("Ending", "Ending");
		default:
			return LOCTEXT("Offline", "Offline");
	}
}

FText UNeutronSessionsManager::GetNetworkErrorString() const
{
	if (!LastNetworkErrorString.IsEmpty())
	{
		return FText::FromString(LastNetworkErrorString);
	}

	switch (LastNetworkError)
	{
		case ENeutronNetworkError::Success:
			return LOCTEXT("Success", "Success");
		case ENeutronNetworkError::CreateFailed:
			return LOCTEXT("CreateFailed", "Failed to create session");
		case ENeutronNetworkError::StartFailed:
			return LOCTEXT("StartFailed", "Failed to start session");
		case ENeutronNetworkError::DestroyFailed:
			return LOCTEXT("DestroyFailed", "Failed to leave session");
		case ENeutronNetworkError::ReadFriendsFailed:
			return LOCTEXT("ReadFriendsFailed", "Couldn't read the friend list");
		case ENeutronNetworkError::JoinFriendFailed:
			return LOCTEXT("JoinFriendFailed", "Failed to join friend");
		case ENeutronNetworkError::JoinSessionIsFull:
			return LOCTEXT("JoinSessionIsFull", "Session is full");
		case ENeutronNetworkError::JoinSessionDoesNotExist:
			return LOCTEXT("JoinSessionDoesNotExist", "Session does not exist anymore");
		case ENeutronNetworkError::JoinSessionConnectionError:
			return LOCTEXT("JoinSessionConnectionError", "Failed to join session");

		case ENeutronNetworkError::NetDriverError:
			return LOCTEXT("NetDriverError", "Net driver error");
		case ENeutronNetworkError::ConnectionLost:
			return LOCTEXT("ConnectionLost", "Connection lost");
		case ENeutronNetworkError::ConnectionTimeout:
			return LOCTEXT("ConnectionTimeout", "Connection timed out");
		case ENeutronNetworkError::ConnectionFailed:
			return LOCTEXT("ConnectionFailed", "Disconnected");
		case ENeutronNetworkError::VersionMismatch:
			return LOCTEXT("VersionMismatch", "Game version mismatch");

		default:
			return LOCTEXT("Unknown", "UnknownError");
	}
}

#undef LOCTEXT_NAMESPACE
