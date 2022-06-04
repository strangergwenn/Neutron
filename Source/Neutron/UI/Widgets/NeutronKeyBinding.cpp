#pragma once

#include "NeutronKeyBinding.h"
#include "Neutron/Neutron.h"

#include "GameFramework/InputSettings.h"

#define LOCTEXT_NAMESPACE "SNeutronKeyBind"

/*----------------------------------------------------
    Key binding structure
----------------------------------------------------*/

FNeutronKeyBinding* FNeutronKeyBinding::Action(const FName& Mapping)
{
	UInputSettings* InputSettings = UInputSettings::StaticClass()->GetDefaultObject<UInputSettings>();
	bool            Found         = false;

	// Find the binding
	for (const FInputActionKeyMapping& Action : InputSettings->GetActionMappings())
	{
		if (Mapping == Action.ActionName && !Action.Key.IsGamepadKey())
		{
			ActionMappings.Add(Action);

			if (UserKey == FKey())
			{
				UserKey = Action.Key;
			}

			Found = true;
		}
	}

	if (!Found)
	{
		FInputActionKeyMapping Action;
		Action.ActionName = Mapping;
		ActionMappings.Add(Action);
	}

	return this;
}

FNeutronKeyBinding* FNeutronKeyBinding::Axis(const FName& Mapping, float Scale)
{
	UInputSettings* InputSettings = UInputSettings::StaticClass()->GetDefaultObject<UInputSettings>();
	bool            Found         = false;

	// Find the binding
	for (const FInputAxisKeyMapping& Axis : InputSettings->GetAxisMappings())
	{
		if (Mapping == Axis.AxisName && Axis.Scale == Scale && !Axis.Key.IsGamepadKey())
		{
			AxisMappings.Add(Axis);

			if (UserKey == FKey())
			{
				UserKey = Axis.Key;
			}

			Found = true;
		}
	}

	if (!Found)
	{
		FInputAxisKeyMapping Action;
		Action.AxisName = Mapping;
		Action.Scale    = Scale;
		AxisMappings.Add(Action);
	}

	return this;
}

void FNeutronKeyBinding::Save()
{
	UInputSettings* InputSettings = UInputSettings::StaticClass()->GetDefaultObject<UInputSettings>();

	// Remove the original bindings
	for (auto& Bind : ActionMappings)
	{
		InputSettings->RemoveActionMapping(Bind);
	}
	for (auto& Bind : AxisMappings)
	{
		InputSettings->RemoveAxisMapping(Bind);
	}

	// Save action mappings
	TArray<FName> ActionMappingNames;
	for (FInputActionKeyMapping& Binding : ActionMappings)
	{
		Binding.Key = UserKey;
		InputSettings->AddActionMapping(Binding);
		ActionMappingNames.AddUnique(Binding.ActionName);
	}

	// Save axis mappings
	TArray<FName> AxisMappingNames;
	TArray<float> AxisMappingScales;
	for (FInputAxisKeyMapping& Binding : AxisMappings)
	{
		Binding.Key = UserKey;
		InputSettings->AddAxisMapping(Binding);

		AxisMappingNames.Add(Binding.AxisName);
		AxisMappingScales.Add(Binding.Scale);
	}

	// Reload action bindings
	ActionMappings.Empty();
	for (FName MappingName : ActionMappingNames)
	{
		Action(MappingName);
	}

	// Reload axis bindings
	AxisMappings.Empty();
	for (int i = 0; i < AxisMappingNames.Num(); i++)
	{
		Axis(AxisMappingNames[i], AxisMappingScales[i]);
	}
}

/*----------------------------------------------------
    Construct
----------------------------------------------------*/

void SNeutronKeyBinding::Construct(const FArguments& InArgs)
{
	// Arguments
	Binding             = InArgs._Binding;
	ThemeName           = InArgs._Theme;
	EditIcon            = InArgs._Icon;
	OnKeyBindingChanged = InArgs._OnKeyBindingChanged;

	// Parent constructor
	SNeutronButton::Construct(SNeutronButton::FArguments()
								  .Theme(InArgs._Theme)
								  .Icon(EditIcon)
								  .Text(this, &SNeutronKeyBinding::GetKeyName)
								  .HelpText(LOCTEXT("EditBinding", "Change this key binding")));

	// Initialize
	WaitingForKey = false;
}

/*----------------------------------------------------
    Interaction
----------------------------------------------------*/

void SNeutronKeyBinding::Tick(const FGeometry& AllottedGeometry, const double CurrentTime, const float DeltaTime)
{
	SNeutronButton::Tick(AllottedGeometry, CurrentTime, DeltaTime);

	// Make sure the mouse pointer doesn't leave the button as long as we're waiting and focused
	if (WaitingForKey)
	{
		if (!IsFocused())
		{
			FinishWaiting();
		}
		else
		{
			FSlateApplication::Get().GetPlatformApplication()->Cursor->SetPosition(WaitingMousePos.X, WaitingMousePos.Y);
		}
	}
}

FReply SNeutronKeyBinding::OnButtonClicked()
{
	// Get the center of the widget so we can lock our mouse there
	FSlateRect Rect   = GetCachedGeometry().GetLayoutBoundingRect();
	WaitingMousePos.X = (Rect.Left + Rect.Right) * 0.5f;
	WaitingMousePos.Y = (Rect.Top + Rect.Bottom) * 0.5f;
	FSlateApplication::Get().GetPlatformApplication()->Cursor->SetPosition(WaitingMousePos.X, WaitingMousePos.Y);

	// Block input
	WaitingForKey = true;
	FSlateApplication::Get().GetPlatformApplication()->Cursor->Show(false);

	return SNeutronButton::OnButtonClicked();
}

void SNeutronKeyBinding::OnKeyPicked(FKey NewKey, bool bCanReset, bool Notify)
{
	if (NewKey.IsValid())
	{
		// Save new key
		FKey CurrentKey = Binding->GetKey();
		if (NewKey == Binding->GetKey() && bCanReset)
		{
			NewKey = FKey();
		}
		Binding->SetKey(NewKey);

		// Resume input
		FinishWaiting();

		// Inform user
		if (Notify)
		{
			OnKeyBindingChanged.ExecuteIfBound(CurrentKey, NewKey);
		}

		const FNeutronButtonTheme& Theme = FNeutronStyleSet::GetButtonTheme(ThemeName);
		FSlateApplication::Get().PlaySound(Theme.ClickSound);
	}
}

void SNeutronKeyBinding::FinishWaiting()
{
	WaitingForKey = false;
	FSlateApplication::Get().GetPlatformApplication()->Cursor->Show(true);
}

/*----------------------------------------------------
    Callbacks
----------------------------------------------------*/

FText SNeutronKeyBinding::GetKeyName() const
{
	if (WaitingForKey)
	{
		return (LOCTEXT("PressKey", "Press a key"));
	}
	else
	{
		return Binding->GetKeyName();
	}
}

FReply SNeutronKeyBinding::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	if (WaitingForKey && !InKeyEvent.GetKey().IsGamepadKey())
	{
		OnKeyPicked(InKeyEvent.GetKey());

		return FReply::Handled();
	}
	else
	{
		FinishWaiting();
		return FReply::Unhandled();
	}
}

FReply SNeutronKeyBinding::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (WaitingForKey)
	{
		OnKeyPicked(MouseEvent.GetEffectingButton());

		return FReply::Handled();
	}

	// Super call is required for the button to even click
	return SNeutronButton::OnMouseButtonDown(MyGeometry, MouseEvent);
}

FReply SNeutronKeyBinding::OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (WaitingForKey)
	{
		OnKeyPicked(MouseEvent.GetWheelDelta() > 0 ? EKeys::MouseScrollUp : EKeys::MouseScrollDown);
		return FReply::Handled();
	}
	else
	{
		return FReply::Unhandled();
	}
}

#undef LOCTEXT_NAMESPACE
