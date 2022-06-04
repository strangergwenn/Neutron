// Neutron - Gwennaël Arbona

#pragma once

#include "Neutron/Actor/NeutronActorTools.h"
#include "Neutron/UI/NeutronUI.h"

#include "GameFramework/Actor.h"
#include "CoreMinimal.h"
#include "SlateBasics.h"
#include "Tickable.h"
#include "NeutronMenuManager.generated.h"

/*----------------------------------------------------
    Supporting types
----------------------------------------------------*/

/** Menu states */
enum class ENeutronFadeState : uint8
{
	FadingFromBlack,
	Black,
	FadingToBlack
};

/** Async command data */
struct FNeutronAsyncCommand
{
	FNeutronAsyncCommand() : Action(), Condition(), FadeDuration(0)
	{}

	FNeutronAsyncCommand(FNeutronAsyncAction A, FNeutronAsyncCondition C, bool ShortFade)
		: Action(A), Condition(C), FadeDuration(ShortFade ? ENeutronUIConstants::FadeDurationShort : ENeutronUIConstants::FadeDurationLong)
	{}

	FNeutronAsyncAction    Action;
	FNeutronAsyncCondition Condition;
	float                  FadeDuration;
};

/*----------------------------------------------------
    Menu manager class
----------------------------------------------------*/

/** Menu manager class */
UCLASS(ClassGroup = (Neutron))
class NEUTRON_API UNeutronMenuManager
	: public UObject
	, public TSharedFromThis<UNeutronMenuManager>
	, public FTickableGameObject
{
	GENERATED_BODY()

public:

	UNeutronMenuManager();

	/*----------------------------------------------------
	    Player interface
	----------------------------------------------------*/

	/** Get the singleton instance */
	static UNeutronMenuManager* Get()
	{
		return Singleton;
	}

	/** Initialize this class */
	void Initialize(class UNeutronGameInstance* NewGameInstance);

	/** Start playing on a new level */
	template <typename MenuClass, typename OverlayClass>
	void BeginPlay(class ANeutronPlayerController* PC)
	{
		NLOG("UNeutronMenuManager::BeginPlay");

		bool AddMenusToScreen = false;
		if (!Menu.IsValid() || !Overlay.IsValid())
		{
			SAssignNew(Menu, MenuClass).MenuManager(this);
			SAssignNew(Overlay, OverlayClass).MenuManager(this);
			AddMenusToScreen = true;
		}

		BeginPlayInternal(PC, AddMenusToScreen);
	}

	/*----------------------------------------------------
	    Tick
	----------------------------------------------------*/

	virtual void              Tick(float DeltaTime) override;
	virtual ETickableTickType GetTickableTickType() const override
	{
		return ETickableTickType::Always;
	}
	virtual TStatId GetStatId() const override
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(UNeutronMenuManager, STATGROUP_Tickables);
	}
	virtual bool IsTickableWhenPaused() const
	{
		return true;
	}
	virtual bool IsTickableInEditor() const
	{
		return false;
	}

	/*----------------------------------------------------
	    Menu management
	----------------------------------------------------*/

	/** Fade to black with a loading screen, call the action, wait for the condition to return true, then fade back */
	void RunWaitAction(ENeutronLoadingScreen LoadingScreen, FNeutronAsyncAction Action,
		FNeutronAsyncCondition Condition = FNeutronAsyncCondition(), bool ShortFade = false);

	/** Fade to black with a loading screen, call the action, stay black */
	void RunAction(ENeutronLoadingScreen LoadingScreen, FNeutronAsyncAction Action, bool ShortFade = false);

	/** Manually reveal the game image once the action is finished */
	void CompleteAsyncAction();

	/** Check if the menu system is idle */
	bool IsIdle() const
	{
		return CurrentMenuState == ENeutronFadeState::FadingFromBlack;
	}

	/** Request opening of the main menu */
	void OpenMenu(FNeutronAsyncAction Action = FNeutronAsyncAction(), FNeutronAsyncCondition Condition = FNeutronAsyncCondition());

	/** Request closing of the main menu */
	void CloseMenu(FNeutronAsyncAction Action = FNeutronAsyncAction(), FNeutronAsyncCondition Condition = FNeutronAsyncCondition());

	/** Check if the main menu is open */
	bool IsMenuOpen() const;

	/** Check if the main menu is open or currently opening */
	bool IsMenuOpening() const;

	/** Get the current loading screen alpha */
	float GetLoadingScreenAlpha() const;

	/** Start displaying the tooltip */
	void ShowTooltip(SWidget* TargetWidget, const FText& Content);

	/** Stop displaying the tooltip */
	void HideTooltip(SWidget* TargetWidget);

	/** Change the current UI colors */
	void SetInterfaceColor(const FLinearColor& Color, const FLinearColor& HighlightColor);

	/** Return the current UI color */
	FLinearColor GetInterfaceColor() const;

	/** Return the current highlight UI color */
	FLinearColor GetHighlightColor() const;

	/*----------------------------------------------------
	    Menu tools
	----------------------------------------------------*/

	/** Register a menu **/
	void RegisterGameMenu(TSharedPtr<class INeutronGameMenu> GameMenu)
	{
		GameMenus.Add(GameMenu.Get());
	}

	/** Get the local player controller owning the menus */
	template <typename T = ANeutronPlayerController>
	T* GetPC() const
	{
		return Cast<T>(PlayerController);
	}

	/** Get the main menu */
	template <typename T = class SNeutronMenu>
	TSharedPtr<T> GetMenu()
	{
		return StaticCastSharedPtr<T>(Menu);
	}

	/** Get the game overlay */
	template <typename T = class SWidget>
	TSharedPtr<T> GetOverlay() const
	{
		return StaticCastSharedPtr<T>(Overlay);
	}

	/** Set the focus to the main menu */
	void SetFocusToMenu();

	/** Set the focus to the game overlay */
	void SetFocusToOverlay();

	/** Set the focus to the game */
	void SetFocusToGame();

	/** Set if we are using a gamepad */
	void SetUsingGamepad(bool State);

	/** Check if we are using a gamepad */
	UFUNCTION(Category = Neutron, BlueprintCallable)
	bool IsUsingGamepad() const;

	/** Are we running on console ? */
	UFUNCTION(Category = Neutron, BlueprintCallable)
	bool IsOnConsole() const;

	/** Get the first key mapped to a specific action */
	FKey GetFirstActionKey(FName ActionName) const;

	/** Maximize the current window or restore it */
	void MaximizeOrRestore();

	/*----------------------------------------------------
	    Internal
	----------------------------------------------------*/

protected:

	/** Signal the player is ready */
	void BeginPlayInternal(class ANeutronPlayerController* PC, bool AddMenusToScreen);

	/** World was cleaned up, warn menus */
	void OnWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources);

	/*----------------------------------------------------
	    Properties
	----------------------------------------------------*/

public:

	// Time it takes to fade in or out in seconds
	UPROPERTY(Category = Neutron, EditDefaultsOnly)
	float FadeDuration;

	// Time it takes to change the UI color in seconds
	UPROPERTY(Category = Neutron, EditDefaultsOnly)
	float ColorChangeDuration;

protected:

	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

	// Singleton pointer
	static UNeutronMenuManager* Singleton;

	// Player owner
	UPROPERTY()
	class ANeutronPlayerController* PlayerController;

	// Menu pointers
	TSharedPtr<class SNeutronMenu> Menu;
	TSharedPtr<class SWidget>      Overlay;
	TSharedPtr<class SWidget>      DesiredFocusWidget;

	// Current menu state
	bool                            UsingGamepad;
	FNeutronAsyncCommand            CurrentCommand;
	TQueue<FNeutronAsyncCommand>    CommandStack;
	ENeutronFadeState               CurrentMenuState;
	TArray<class INeutronGameMenu*> GameMenus;

	// Current color state
	float                              CurrentFadingTime;
	FLinearColor                       DesiredInterfaceColor;
	FLinearColor                       DesiredHighlightColor;
	TNeutronTimedAverage<FLinearColor> CurrentInterfaceColor;
	TNeutronTimedAverage<FLinearColor> CurrentHighlightColor;

	// Dynamic material pool
	UPROPERTY()
	TArray<class UMaterialInterface*> OwnedMaterials;
};
