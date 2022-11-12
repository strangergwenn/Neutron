﻿// Neutron - Gwennaël Arbona

#include "NeutronMenu.h"
#include "NeutronButton.h"
#include "NeutronNavigationPanel.h"
#include "NeutronModalPanel.h"

#include "Neutron/System/NeutronMenuManager.h"
#include "Neutron/Neutron.h"

#include "GameFramework/InputSettings.h"
#include "Input/HittestGrid.h"

#define LOCTEXT_NAMESPACE "SNeutronMenu"

/*----------------------------------------------------
    Constructor
----------------------------------------------------*/

SNeutronMenu::SNeutronMenu()
	: AnalogNavThreshold(0.25f)
	, AnalogNavMinPeriod(0.20f)
	, AnalogNavMaxPeriod(1.0f)
	, MousePressed(false)
	, MouseWasPressed(false)
	, CurrentNavigationPanel(nullptr)
	, PreviousNavigationPanel(nullptr)
	, CurrentAnalogNavigation(EUINavigation::Invalid)
	, CurrentAnalogNavigationTime(0)
{}

void SNeutronMenu::Construct(const FArguments& InArgs)
{
	// Data
	const FNeutronMainTheme& Theme = FNeutronStyleSet::GetMainTheme();
	MenuManager                    = InArgs._MenuManager;
	NCHECK(MenuManager.IsValid());

	// clang-format off
	ChildSlot
	[
		SAssignNew(ModalOverlay, SOverlay)

		+ SOverlay::Slot()
		[
			SAssignNew(MainContainer, SBox)
		]
	];
	// clang-format on

	SmoothedMouseLocation.SetPeriod(0.2f);
}

/*----------------------------------------------------
    Interaction
----------------------------------------------------*/

void SNeutronMenu::Tick(const FGeometry& AllottedGeometry, const double CurrentTime, const float DeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, CurrentTime, DeltaTime);

	// Reset navigation if nothing is focused
	if (CurrentNavigationPanel && !GetFocusedButton())
	{
		CurrentNavigationPanel->ResetNavigation();
	}

	// Update analog navigation
	CurrentAnalogNavigationTime += DeltaTime;

	// Update analog input processing
	if (CurrentNavigationPanel)
	{
		if (MenuManager->IsUsingGamepad())
		{
			MousePressed    = false;
			MouseWasPressed = false;
		}
		else
		{
			CurrentAnalogInput = FVector2D::ZeroVector;
		}

		if (MousePressed)
		{
			auto&     App           = FSlateApplication::Get();
			FVector2D MouseLocation = App.GetCursorPos();

			// Reset mouse state
			if (!MouseWasPressed)
			{
				SmoothedMouseLocation.Clear();
			}

			// Compute the current analog mouse input with axis snapping
			else
			{
				SmoothedMouseLocation.Set(MouseLocation, DeltaTime);
				const FVector2D MouseMovement = MouseLocation - SmoothedMouseLocation.Get();

				CurrentAnalogInput.X = MouseMovement.GetSafeNormal().X;
				CurrentAnalogInput.Y = -MouseMovement.GetSafeNormal().Y;
			}

			MouseWasPressed = true;
		}
		else
		{
			MouseWasPressed = false;
		}

		// Pass analog input when using gamepad
		bool                       AnalogInputConsumed = false;
		TSharedPtr<SNeutronButton> Button              = GetFocusedButton();
		if (MenuManager->IsUsingGamepad() && Button.IsValid())
		{
			AnalogInputConsumed = Button->HorizontalAnalogInput(CurrentAnalogInput.X);
			AnalogInputConsumed |= Button->VerticalAnalogInput(CurrentAnalogInput.Y);
		}
		if (!AnalogInputConsumed)
		{
			CurrentNavigationPanel->HorizontalAnalogInput(CurrentAnalogInput.X);
			CurrentNavigationPanel->VerticalAnalogInput(CurrentAnalogInput.Y);
		}
	}
}

bool SNeutronMenu::SupportsKeyboardFocus() const
{
	return true;
}

void SNeutronMenu::OnFocusLost(const FFocusEvent& InFocusEvent)
{
	SCompoundWidget::OnFocusLost(InFocusEvent);

	MousePressed    = false;
	MouseWasPressed = false;
}

FReply SNeutronMenu::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	SCompoundWidget::OnMouseButtonDown(MyGeometry, MouseEvent);

	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		MousePressed = true;
	}

	MenuManager->SetUsingGamepad(false);

	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && CurrentNavigationPanel)
	{
		FVector2D   Position           = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
		FWidgetPath WidgetsUnderCursor = FSlateApplication::Get().LocateWindowUnderMouse(
			MouseEvent.GetScreenSpacePosition(), FSlateApplication::Get().GetTopLevelWindows(), false, MouseEvent.GetUserIndex());

		if (&WidgetsUnderCursor.GetLastWidget().Get() == CurrentNavigationPanel || CurrentNavigationPanel->IsClickInsideMenuAllowed())
		{
			CurrentNavigationPanel->OnClicked(Position);
		}
	}

	return FReply::Handled().SetUserFocus(SharedThis(this), EFocusCause::SetDirectly);
}

FReply SNeutronMenu::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	SCompoundWidget::OnMouseButtonUp(MyGeometry, MouseEvent);

	MousePressed = false;

	HandleKeyPress(MouseEvent.GetEffectingButton());

	return FReply::Handled().SetUserFocus(SharedThis(this), EFocusCause::SetDirectly);
}

FReply SNeutronMenu::OnMouseButtonDoubleClick(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	FReply Result = SCompoundWidget::OnMouseButtonDoubleClick(MyGeometry, MouseEvent);

	if (CurrentNavigationPanel)
	{
		FVector2D   Position           = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
		FWidgetPath WidgetsUnderCursor = FSlateApplication::Get().LocateWindowUnderMouse(
			MouseEvent.GetScreenSpacePosition(), FSlateApplication::Get().GetTopLevelWindows(), false, MouseEvent.GetUserIndex());

		if (&WidgetsUnderCursor.GetLastWidget().Get() == CurrentNavigationPanel || CurrentNavigationPanel->IsClickInsideMenuAllowed())
		{
			CurrentNavigationPanel->OnDoubleClicked(Position);
		}
	}

	return Result;
}

FReply SNeutronMenu::OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	FReply Result = SCompoundWidget::OnMouseWheel(MyGeometry, MouseEvent);

	return HandleKeyPress(MouseEvent.GetWheelDelta() > 0 ? EKeys::MouseScrollUp : EKeys::MouseScrollDown);
}

void SNeutronMenu::OnMouseLeave(const FPointerEvent& MouseEvent)
{
	SCompoundWidget::OnMouseLeave(MouseEvent);

	MousePressed = false;
}

FReply SNeutronMenu::OnAnalogValueChanged(const FGeometry& MyGeometry, const FAnalogInputEvent& AnalogInputEvent)
{
	SCompoundWidget::OnAnalogValueChanged(MyGeometry, AnalogInputEvent);

	// Get data
	const FKey                 Key           = AnalogInputEvent.GetKey();
	FReply                     Result        = FReply::Unhandled();
	TSharedPtr<SNeutronButton> FocusedButton = GetFocusedButton();
	TSharedPtr<SNeutronButton> DestinationButton;

	// Handle menu keys
	if (CurrentNavigationPanel)
	{
		EUINavigation AnalogNavigation   = EUINavigation::Invalid;
		float         CurrentInputPeriod = FMath::Lerp(AnalogNavMaxPeriod, AnalogNavMinPeriod,
					(FMath::Abs(AnalogInputEvent.GetAnalogValue()) - AnalogNavThreshold) / (1.0f - AnalogNavThreshold));

		// Handle navigation
		if (IsAxisKey(FNeutronPlayerInput::MenuMoveHorizontal, Key))
		{
			if (-AnalogInputEvent.GetAnalogValue() > AnalogNavThreshold)
			{
				AnalogNavigation = EUINavigation::Left;
			}
			else if (AnalogInputEvent.GetAnalogValue() > AnalogNavThreshold)
			{
				AnalogNavigation = EUINavigation::Right;
			}
		}
		else if (IsAxisKey(FNeutronPlayerInput::MenuMoveVertical, Key))
		{
			if (AnalogInputEvent.GetAnalogValue() > AnalogNavThreshold)
			{
				AnalogNavigation = EUINavigation::Up;
			}
			else if (-AnalogInputEvent.GetAnalogValue() > AnalogNavThreshold)
			{
				AnalogNavigation = EUINavigation::Down;
			}
		}

		// Update focus destination with a maximum period
		if (AnalogNavigation != EUINavigation::Invalid && CurrentAnalogNavigationTime >= CurrentInputPeriod)
		{
			DestinationButton =
				GetNextButton(SharedThis(CurrentNavigationPanel), FocusedButton, CurrentNavigationButtons, AnalogNavigation);
			if (DestinationButton.IsValid() && DestinationButton->SupportsKeyboardFocus())
			{
				SetFocusedButton(DestinationButton, true);

				CurrentAnalogNavigation     = AnalogNavigation;
				CurrentAnalogNavigationTime = 0;
				Result                      = FReply::Handled();
			}
		}
	}

	auto InputFilter = [&](float InputValue)
	{
		return FMath::Sign(InputValue) * FMath::Max(FMath::Abs(InputValue) - AnalogNavThreshold, 0.0f) / (1.0f - AnalogNavThreshold);
	};

	// Read analog input from controller axis
	if (IsAxisKey(FNeutronPlayerInput::MenuAnalogHorizontal, AnalogInputEvent.GetKey()))
	{
		CurrentAnalogInput.X = InputFilter(AnalogInputEvent.GetAnalogValue());
	}
	else if (IsAxisKey(FNeutronPlayerInput::MenuAnalogVertical, AnalogInputEvent.GetKey()))
	{
		CurrentAnalogInput.Y = InputFilter(AnalogInputEvent.GetAnalogValue());
	}

	return Result;
}

FReply SNeutronMenu::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& KeyEvent)
{
	SCompoundWidget::OnKeyDown(MyGeometry, KeyEvent);

	// Get data
	const FKey                 Key           = KeyEvent.GetKey();
	FReply                     Result        = FReply::Unhandled();
	TSharedPtr<SNeutronButton> FocusedButton = GetFocusedButton();

	// Set gamepad used
	MenuManager->SetUsingGamepad(Key.IsGamepadKey());

	// Pass the event down
	if (CurrentNavigationPanel)
	{
		Result = CurrentNavigationPanel->OnKeyDown(CurrentNavigationPanel->GetCachedGeometry(), KeyEvent);
	}
	if (FocusedButton.IsValid())
	{
		Result = FocusedButton->OnKeyDown(FocusedButton->GetCachedGeometry(), KeyEvent);
	}

	return HandleKeyPress(Key);
}

/*----------------------------------------------------
    Input handling
----------------------------------------------------*/

void SNeutronMenu::UpdateKeyBindings()
{
	UInputSettings* InputSettings = UInputSettings::StaticClass()->GetDefaultObject<UInputSettings>();
	NCHECK(InputSettings);

	// Update action bindings
	ActionBindings.Reset();
	for (int32 i = 0; i < InputSettings->GetActionMappings().Num(); i++)
	{
		FInputActionKeyMapping Action = InputSettings->GetActionMappings()[i];
		ActionBindings.Add(Action.Key.GetFName(), Action.ActionName);
	}

	// Update axis bindings
	AxisBindings.Reset();
	for (int32 i = 0; i < InputSettings->GetAxisMappings().Num(); i++)
	{
		FInputAxisKeyMapping Axis = InputSettings->GetAxisMappings()[i];
		AxisBindings.Add(Axis.Key.GetFName(), Axis.AxisName);
	}
}

bool SNeutronMenu::IsActionKey(FName ActionName, const FKey& Key) const
{
	return ActionBindings.FindPair(Key.GetFName(), ActionName) != nullptr;
}

bool SNeutronMenu::IsAxisKey(FName AxisName, const FKey& Key) const
{
	return AxisBindings.FindPair(Key.GetFName(), AxisName) != nullptr;
}

/*----------------------------------------------------
    Focus handling
----------------------------------------------------*/

void SNeutronMenu::SetNavigationPanel(SNeutronNavigationPanel* Panel)
{
	// NLOG("SNeutronMenu::SetNavigationPanel : '%s'", Panel ? *Panel->GetTypeAsString() : TEXT("nullptr"));

	NCHECK(Panel);

	if (Panel != CurrentNavigationPanel)
	{
		NCHECK(CurrentNavigationButtons.Num() == 0);
		NCHECK(CurrentNavigationPanel == nullptr);

		CurrentNavigationPanel   = Panel;
		CurrentNavigationButtons = Panel->GetNavigationButtons();
	}
	else
	{
		RefreshNavigationPanel();
	}
}

void SNeutronMenu::ClearNavigationPanel()
{
	// NLOG("SNeutronMenu::ClearNavigationPanel : '%s'", CurrentNavigationPanel ? *CurrentNavigationPanel->GetTypeAsString() :
	// TEXT("nullptr"));

	for (TSharedPtr<SNeutronButton> Button : CurrentNavigationButtons)
	{
		Button->SetFocused(false);
	}

	CurrentNavigationPanel = nullptr;
	CurrentNavigationButtons.Empty();
}

void SNeutronMenu::RefreshNavigationPanel()
{
	if (CurrentNavigationPanel)
	{
		CurrentNavigationButtons = CurrentNavigationPanel->GetNavigationButtons();
	}
}

bool SNeutronMenu::HasVisibleOverlay() const
{
	for (const TSharedPtr<SNeutronModalPanel>& ModalPanel : ModalPanels)
	{
		if (ModalPanel->IsVisible())
		{
			return true;
		}
	}

	return false;
}

void SNeutronMenu::SetModalNavigationPanel(class SNeutronNavigationPanel* Panel)
{
	NCHECK(CurrentNavigationPanel);
	NCHECK(PreviousNavigationPanel == nullptr);

	PreviousNavigationPanel = CurrentNavigationPanel;

	ClearNavigationPanel();
	SetNavigationPanel(Panel);
}

void SNeutronMenu::ClearModalNavigationPanel()
{
	NCHECK(PreviousNavigationPanel);

	ClearNavigationPanel();
	SetNavigationPanel(PreviousNavigationPanel);

	PreviousNavigationPanel = nullptr;
}

void SNeutronMenu::SetFocusedButton(TSharedPtr<SNeutronButton> FocusButton, bool FromNavigation)
{
	if (CurrentNavigationButtons.Contains(FocusButton) && FocusButton->SupportsKeyboardFocus())
	{
		for (TSharedPtr<SNeutronButton> Button : CurrentNavigationButtons)
		{
			if (Button != FocusButton && Button->IsFocused())
			{
				Button->SetFocused(false);
			}
		}

		FocusButton->SetFocused(true);

		if (FromNavigation && CurrentNavigationPanel)
		{
			CurrentNavigationPanel->OnFocusChanged(FocusButton);
		}
	}
	else if (!FocusButton.IsValid())
	{
		for (TSharedPtr<SNeutronButton> Button : CurrentNavigationButtons)
		{
			if (Button != FocusButton && Button->IsFocused())
			{
				Button->SetFocused(false);
			}
		}

		if (FromNavigation && CurrentNavigationPanel)
		{
			CurrentNavigationPanel->OnFocusChanged(FocusButton);
		}
	}
}

TSharedPtr<SNeutronButton> SNeutronMenu::GetFocusedButton()
{
	for (TSharedPtr<SNeutronButton> Button : CurrentNavigationButtons)
	{
		if (Button->IsFocused() && Button->GetVisibility() == EVisibility::Visible)
		{
			return Button;
		}
	}

	return TSharedPtr<SNeutronButton>();
}

FReply SNeutronMenu::HandleKeyPress(FKey Key)
{
	FReply                     Result        = FReply::Unhandled();
	TSharedPtr<SNeutronButton> FocusedButton = GetFocusedButton();
	TSharedPtr<SNeutronButton> DestinationButton;

	// Handle menu keys
	if (CurrentNavigationPanel)
	{
		// Handle navigation
		if (IsActionKey(FNeutronPlayerInput::MenuUp, Key))
		{
			DestinationButton =
				GetNextButton(SharedThis(CurrentNavigationPanel), FocusedButton, CurrentNavigationButtons, EUINavigation::Up);
		}
		else if (IsActionKey(FNeutronPlayerInput::MenuDown, Key))
		{
			DestinationButton =
				GetNextButton(SharedThis(CurrentNavigationPanel), FocusedButton, CurrentNavigationButtons, EUINavigation::Down);
		}
		else if (IsActionKey(FNeutronPlayerInput::MenuLeft, Key))
		{
			DestinationButton =
				GetNextButton(SharedThis(CurrentNavigationPanel), FocusedButton, CurrentNavigationButtons, EUINavigation::Left);
		}
		else if (IsActionKey(FNeutronPlayerInput::MenuRight, Key))
		{
			DestinationButton =
				GetNextButton(SharedThis(CurrentNavigationPanel), FocusedButton, CurrentNavigationButtons, EUINavigation::Right);
		}

		// Handle menu actions
		else if (IsActionKey(FNeutronPlayerInput::MenuNext, Key))
		{
			CurrentNavigationPanel->Next();
			Result = FReply::Handled();
		}
		else if (IsActionKey(FNeutronPlayerInput::MenuPrevious, Key))
		{
			CurrentNavigationPanel->Previous();
			Result = FReply::Handled();
		}
		else if (IsActionKey(FNeutronPlayerInput::MenuZoomIn, Key))
		{
			CurrentNavigationPanel->ZoomIn();
			Result = FReply::Handled();
		}
		else if (IsActionKey(FNeutronPlayerInput::MenuZoomOut, Key))
		{
			CurrentNavigationPanel->ZoomOut();
			Result = FReply::Handled();
		}
	}

	// Update focus destination
	if (DestinationButton.IsValid() && DestinationButton->SupportsKeyboardFocus())
	{
		SetFocusedButton(DestinationButton, true);
		Result = FReply::Handled();
	}

	// Trigger action buttons
	for (TSharedPtr<SNeutronButton>& Button : GetActionButtons())
	{
		if (MenuManager->GetFirstActionKey(Button->GetActionName()) == Key && Button->IsButtonEnabled() &&
			Button->GetVisibility() == EVisibility::Visible)
		{
			bool ActionPassedToWidget = false;
			bool WasFocused           = Button->IsFocused();

			if (CurrentNavigationPanel == nullptr || CurrentNavigationPanel->IsButtonActionAllowed(Button) ||
				MenuActionButtons.Contains(Button))
			{
				Button->OnButtonClicked();
				ActionPassedToWidget = true;
			}

			if (CurrentNavigationPanel && WasFocused && Button->IsButtonActionFocusable())
			{
				CurrentNavigationPanel->ResetNavigation();
			}

			if (ActionPassedToWidget)
			{
				Result = FReply::Handled();
				break;
			}
		}
	}

	// Activate focused button
	if (IsActionKey(FNeutronPlayerInput::MenuConfirm, Key))
	{
		if (FocusedButton.IsValid())
		{
			FocusedButton->OnButtonClicked();
			Result = FReply::Handled();
		}
	}
	else if (CurrentNavigationPanel)
	{
		CurrentNavigationPanel->OnKeyPressed(Key);
	}

	return Result;
}

TSharedPtr<SNeutronButton> SNeutronMenu::GetNextButton(
	TSharedRef<SWidget> Widget, TSharedPtr<const SWidget> Current, TArray<TSharedPtr<SNeutronButton>> Candidates, EUINavigation Direction)
{
	if (Current)
	{
		TSharedPtr<SNeutronButton> Result = GetNextButtonInternal(Widget, Current, Candidates, Direction);
		if (!Result)
		{
			Result = GetNextButtonInternal(Widget, Current->GetParentWidget(), Candidates, Direction);
		}

		return Result;
	}
	else
	{
		return TSharedPtr<SNeutronButton>();
	}
}

TSharedPtr<SNeutronButton> SNeutronMenu::GetNextButtonInternal(
	TSharedRef<SWidget> Widget, TSharedPtr<const SWidget> Current, TArray<TSharedPtr<SNeutronButton>> Candidates, EUINavigation Direction)
{
	FWidgetPath Source;
	FWidgetPath Boundary;

	// Find the next widget in the required direction within the tab view
	if (Current.IsValid())
	{
		if (FSlateApplication::Get().FindPathToWidget(Current.ToSharedRef(), Source) &&
			FSlateApplication::Get().FindPathToWidget(Widget, Boundary))
		{
			FNavigationReply       NavigationReply = FNavigationReply::Explicit(Widget);
			const FArrangedWidget& SourceWidget    = Source.Widgets.Last();
			const FArrangedWidget& BoundaryWidget  = Boundary.Widgets.Last();

			TSharedPtr<SWidget> DestinationWidget = Source.GetWindow()->GetHittestGrid().FindNextFocusableWidget(
				SourceWidget, Direction, NavigationReply, BoundaryWidget, FSlateApplication::Get().CursorUserIndex);

			if (DestinationWidget.IsValid() && Candidates.Contains(DestinationWidget))
			{
				SNeutronButton* DestinationButton = static_cast<SNeutronButton*>(DestinationWidget.Get());
				return SharedThis(DestinationButton);
			}
		}
	}

	return TSharedPtr<SNeutronButton>();
}

#undef LOCTEXT_NAMESPACE
