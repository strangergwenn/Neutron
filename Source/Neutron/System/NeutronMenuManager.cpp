// Neutron - Gwennaël Arbona

#include "NeutronMenuManager.h"

#include "Neutron/Player/NeutronGameViewportClient.h"
#include "Neutron/Player/NeutronPlayerController.h"

#include "Neutron/Settings/NeutronWorldSettings.h"

#include "Neutron/UI/NeutronUI.h"
#include "Neutron/UI/Widgets/NeutronMenu.h"
#include "Neutron/UI/Widgets/NeutronButton.h"

#include "Neutron/Neutron.h"

#include "Framework/Application/SlateApplication.h"
#include "Engine/Console.h"
#include "Engine.h"

// Statics
UNeutronMenuManager* UNeutronMenuManager::Singleton = nullptr;

/*----------------------------------------------------
    Constructor
----------------------------------------------------*/

UNeutronMenuManager::UNeutronMenuManager()
	: Super()
	, UsingGamepad(false)
	, CurrentMenuState(ENeutronFadeState::FadingFromBlack)
	, DesiredInterfaceColor(FLinearColor::White)
	, DesiredHighlightColor(FLinearColor::White)
{
	// Settings
	FadeDuration        = ENeutronUIConstants::FadeDurationLong;
	ColorChangeDuration = ENeutronUIConstants::FadeDurationShort;
}

/*----------------------------------------------------
    Inherited
----------------------------------------------------*/

class FNeutronNavigationConfig : public FNavigationConfig
{
	EUINavigation GetNavigationDirectionFromKey(const FKeyEvent& InKeyEvent) const override
	{
		return EUINavigation::Invalid;
	}

	EUINavigation GetNavigationDirectionFromAnalog(const FAnalogInputEvent& InAnalogEvent) override
	{
		return EUINavigation::Invalid;
	}

	EUINavigationAction GetNavigationActionFromKey(const FKeyEvent& InKeyEvent) const override
	{
		return EUINavigationAction::Invalid;
	}
};

void UNeutronMenuManager::Initialize(UNeutronGameInstance* NewGameInstance)
{
	NLOG("UNeutronMenuManager::Initialize");

	Singleton = this;
	FSlateApplication::Get().SetNavigationConfig(MakeShared<FNeutronNavigationConfig>());
	FWorldDelegates::OnWorldCleanup.AddUObject(this, &UNeutronMenuManager::OnWorldCleanup);
}

void UNeutronMenuManager::Tick(float DeltaTime)
{
	if (IsValid(PlayerController) && PlayerController->IsReady() && PlayerController->GetGameTimeSinceCreation() > 2.0f)
	{
		switch (CurrentMenuState)
		{
			// Fade to black, call the provided callback, and move on
			case ENeutronFadeState::FadingToBlack: {
				FNeutronAsyncCommand* Command = CommandStack.Peek();
				if (Command)
				{
					FadeDuration = Command->FadeDuration;
				}

				CurrentFadingTime += DeltaTime;
				CurrentFadingTime = FMath::Clamp(CurrentFadingTime, 0.0f, FadeDuration);

				if (CurrentFadingTime >= FadeDuration && CommandStack.Dequeue(CurrentCommand))
				{
					CurrentMenuState = ENeutronFadeState::Black;
				}

				break;
			}

			// Process the queue of asynchronous actions + conditions
			case ENeutronFadeState::Black: {
				// Processing action
				if (CurrentCommand.Action.IsBound())
				{
					CurrentCommand.Action.Execute();
					CurrentCommand.Action.Unbind();
				}

				// Waiting condition
				if (CurrentCommand.Condition.IsBound())
				{
					if (CurrentCommand.Condition.Execute())
					{
						CurrentCommand.Condition.Unbind();

						CompleteAsyncAction();
					}
				}
				else
				{
					CompleteAsyncAction();
				}

				GEngine->ForceGarbageCollection(true);

				break;
			}

			// Fading back
			case ENeutronFadeState::FadingFromBlack: {
				CurrentFadingTime -= DeltaTime;
				CurrentFadingTime = FMath::Clamp(CurrentFadingTime, 0.0f, FadeDuration);

				break;
			}
		}
	}

	// Build focus filter
	TArray<TSharedPtr<SWidget>> AllowedFocusObjects;
	AllowedFocusObjects.Add(Menu);
	AllowedFocusObjects.Add(Overlay);

	// Conditionally allow the game viewport, when not in console
	UNeutronGameViewportClient* GameViewportClient = Cast<UNeutronGameViewportClient>(GetWorld()->GetGameViewport());
	if (GameViewportClient && (GameViewportClient->ViewportConsole == nullptr || !GameViewportClient->ViewportConsole->ConsoleActive()))
	{
		AllowedFocusObjects.Add(FSlateApplication::Get().GetGameViewport());
	}

	// Update focus when it's one of our widgets to another of our widgets
	TSharedPtr<class SWidget> CurrentFocusWidget = FSlateApplication::Get().GetUserFocusedWidget(0);
	if (DesiredFocusWidget.IsValid() && CurrentFocusWidget != DesiredFocusWidget && AllowedFocusObjects.Contains(CurrentFocusWidget) &&
		AllowedFocusObjects.Contains(DesiredFocusWidget))
	{
		NLOG("UNeutronMenuManager::Tick : moving focus from '%s' to '%s'",
			CurrentFocusWidget.IsValid() ? *CurrentFocusWidget->GetTypeAsString() : TEXT("null"), *DesiredFocusWidget->GetTypeAsString());

		FSlateApplication::Get().SetAllUserFocus(DesiredFocusWidget.ToSharedRef());
	}

	// Update UI color
	CurrentInterfaceColor.Set(DesiredInterfaceColor, DeltaTime);
	CurrentHighlightColor.Set(DesiredHighlightColor, DeltaTime);

	// Update game menus
	for (INeutronGameMenu* GameMenu : GameMenus)
	{
		GameMenu->UpdateGameObjects();
	}
}

/*----------------------------------------------------
    Menu management
----------------------------------------------------*/

void UNeutronMenuManager::RunWaitAction(
	ENeutronLoadingScreen LoadingScreen, FNeutronAsyncAction Action, FNeutronAsyncCondition Condition, bool ShortFade)
{
	NLOG("UNeutronMenuManager::RunWaitAction");

	Cast<UNeutronGameViewportClient>(GetWorld()->GetGameViewport())->SetLoadingScreen(LoadingScreen);

	CommandStack.Enqueue(FNeutronAsyncCommand(Action, Condition, ShortFade));
	CurrentMenuState = ENeutronFadeState::FadingToBlack;
}

void UNeutronMenuManager::RunAction(ENeutronLoadingScreen LoadingScreen, FNeutronAsyncAction Action, bool ShortFade)
{
	RunWaitAction(LoadingScreen, Action,
		FNeutronAsyncCondition::CreateLambda(
			[=]()
			{
				return false;
			}),
		ShortFade);
}

void UNeutronMenuManager::CompleteAsyncAction()
{
	if (!CommandStack.Dequeue(CurrentCommand))
	{
		CurrentMenuState = ENeutronFadeState::FadingFromBlack;
	}
}

void UNeutronMenuManager::OpenMenu(FNeutronAsyncAction Action, FNeutronAsyncCondition Condition)
{
	NLOG("UNeutronMenuManager::OpenMenu");

	RunWaitAction(ENeutronLoadingScreen::Black,
		FNeutronAsyncAction::CreateLambda(
			[=]()
			{
				if (Menu.IsValid())
				{
					Action.ExecuteIfBound();
					Menu->Show();
					SetFocusToMenu();
				}
			}),
		Condition);
}

void UNeutronMenuManager::CloseMenu(FNeutronAsyncAction Action, FNeutronAsyncCondition Condition)
{
	NLOG("UNeutronMenuManager::CloseMenu");

	RunWaitAction(ENeutronLoadingScreen::Black,
		FNeutronAsyncAction::CreateLambda(
			[=]()
			{
				if (Menu.IsValid())
				{
					Action.ExecuteIfBound();
					Menu->Hide();
					SetFocusToGame();
				}
			}),
		Condition);
}

bool UNeutronMenuManager::IsMenuOpen() const
{
	return Menu && Menu->GetVisibility() == EVisibility::Visible;
}

bool UNeutronMenuManager::IsMenuOpening() const
{
	return IsMenuOpen() || CurrentMenuState == ENeutronFadeState::FadingToBlack;
}

float UNeutronMenuManager::GetLoadingScreenAlpha() const
{
	return FMath::InterpEaseInOut(0.0f, 1.0f, CurrentFadingTime / FadeDuration, ENeutronUIConstants::EaseStrong);
}

void UNeutronMenuManager::ShowTooltip(SWidget* TargetWidget, const FText& Content)
{
	if (Menu.IsValid())
	{
		Menu->ShowTooltip(TargetWidget, Content);
	}
}

void UNeutronMenuManager::HideTooltip(SWidget* TargetWidget)
{
	if (Menu.IsValid())
	{
		Menu->HideTooltip(TargetWidget);
	}
}

FLinearColor UNeutronMenuManager::GetInterfaceColor() const
{
	return CurrentInterfaceColor.Get();
}

FLinearColor UNeutronMenuManager::GetHighlightColor() const
{
	return CurrentHighlightColor.Get();
}

void UNeutronMenuManager::SetInterfaceColor(const FLinearColor& Color, const FLinearColor& HighlightColor)
{
	DesiredInterfaceColor = Color;
	DesiredHighlightColor = HighlightColor;
}

/*----------------------------------------------------
    Menu tools
----------------------------------------------------*/

void UNeutronMenuManager::SetFocusToMenu()
{
	if (!IsUsingGamepad())
	{
		GetPC()->SetInputMode(FInputModeUIOnly());
	}

	DesiredFocusWidget = Menu;
}

void UNeutronMenuManager::SetFocusToOverlay()
{
	if (!IsUsingGamepad())
	{
		GetPC()->SetInputMode(FInputModeUIOnly());
	}

	DesiredFocusWidget = Overlay;
}

void UNeutronMenuManager::SetFocusToGame()
{
	GetPC()->SetInputMode(FInputModeGameOnly());

	DesiredFocusWidget = FSlateApplication::Get().GetGameViewport();
}

void UNeutronMenuManager::SetUsingGamepad(bool State)
{
	if (State != UsingGamepad)
	{
		if (State)
		{
			NLOG("UNeutronMenuManager::SetUsingGamepad : using gamepad");

			FSlateApplication::Get().GetPlatformApplication()->Cursor->Show(false);
		}
		else
		{
			NLOG("UNeutronMenuManager::SetUsingGamepad : using mouse + keyboard");

			FSlateApplication::Get().GetPlatformApplication()->Cursor->Show(true);
			FSlateApplication::Get().ReleaseAllPointerCapture();
			FSlateApplication::Get().GetPlatformApplication()->SetCapture(nullptr);
		}
	}

	UsingGamepad = State;
}

bool UNeutronMenuManager::IsUsingGamepad() const
{
	return UsingGamepad;
}

bool UNeutronMenuManager::IsOnConsole() const
{
	return false;
}

FKey UNeutronMenuManager::GetFirstActionKey(FName ActionName) const
{
	UInputSettings* InputSettings = UInputSettings::StaticClass()->GetDefaultObject<UInputSettings>();
	NCHECK(InputSettings);

	for (int32 i = 0; i < InputSettings->GetActionMappings().Num(); i++)
	{
		FInputActionKeyMapping Action = InputSettings->GetActionMappings()[i];
		if (Action.ActionName == ActionName && Action.Key.IsGamepadKey() == IsUsingGamepad())
		{
			return Action.Key;
		}
	}

	return FKey(NAME_None);
}

void UNeutronMenuManager::MaximizeOrRestore()
{
	TSharedPtr<SWindow> Window = FSlateApplication::Get().GetTopLevelWindows()[0];
	NCHECK(Window);

	if (!Window->IsWindowMaximized())
	{
		Window->Maximize();
	}
	else
	{
		Window->Restore();
	}
}

/*----------------------------------------------------
    Internal
----------------------------------------------------*/

void UNeutronMenuManager::BeginPlayInternal(ANeutronPlayerController* PC, bool AddMenusToScreen)
{
	NLOG("UNeutronMenuManager::BeginPlayInternal");

	PlayerController = PC;

	// Assign menus
	if (AddMenusToScreen)
	{
		UGameViewportClient* GameViewportClient = GetWorld()->GetGameViewport();
		GameViewportClient->AddViewportWidgetContent(SNew(SWeakWidget).PossiblyNullContent(Menu.ToSharedRef()), 50);
		GameViewportClient->AddViewportWidgetContent(SNew(SWeakWidget).PossiblyNullContent(Overlay.ToSharedRef()), 100);
	}

	// Check for maximized state at boot
	TSharedPtr<SWindow> Window = FSlateApplication::Get().GetTopLevelWindows()[0];
	NCHECK(Window);
	FVector2D ViewportResolution = Window->GetViewportSize();
	FIntPoint DesktopResolution  = GEngine->GetGameUserSettings()->GetDesktopResolution();
	if (!Window->IsWindowMaximized() && ViewportResolution.X == DesktopResolution.X && ViewportResolution.Y != DesktopResolution.Y)
	{
		Window->Maximize();
	}

	// Initialize
	CurrentMenuState  = ENeutronFadeState::FadingFromBlack;
	CurrentFadingTime = FadeDuration;
	Menu->UpdateKeyBindings();

	// Initialize the desired color
	CurrentInterfaceColor.SetPeriod(ColorChangeDuration);
	CurrentHighlightColor.SetPeriod(ColorChangeDuration);
	CurrentInterfaceColor.Set(DesiredInterfaceColor);
	CurrentHighlightColor.Set(DesiredHighlightColor);

	// Open the menu if desired
	RunWaitAction(ENeutronLoadingScreen::Black,
		FNeutronAsyncAction::CreateLambda(
			[=]()
			{
				NLOG("UNeutronMenuManager::BeginPlayInternal : setting up menu");

				if (Cast<ANeutronWorldSettings>(GetWorld()->GetWorldSettings())->IsMenuMap())
				{
					Menu->Show();
					SetFocusToMenu();
				}
				else
				{
					Menu->Hide();
					SetFocusToGame();
				}

				NLOG("UNeutronMenuManager::BeginPlayInternal : waiting for a bit");
			}),
		FNeutronAsyncCondition::CreateLambda(
			[=]()
			{
				NLOG("UNeutronMenuManager::BeginPlayInternal : done");
				return true;
			}));
}

void UNeutronMenuManager::OnWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources)
{
	NLOG("UNeutronMenuManager::OnWorldCleanup");

	for (INeutronGameMenu* GameMenu : GameMenus)
	{
		GameMenu->UpdateGameObjects();
	}
}
