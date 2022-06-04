#pragma once

#include "NeutronNavigationPanel.h"
#include "NeutronMenu.h"
#include "Neutron/Neutron.h"

/*----------------------------------------------------
    Construct
----------------------------------------------------*/

void SNeutronNavigationPanel::Construct(const FArguments& InArgs)
{
	Menu = InArgs._Menu;

	ChildSlot[InArgs._Content.Widget];
}

/*----------------------------------------------------
    Interaction
----------------------------------------------------*/

TSharedPtr<SNeutronButton> SNeutronNavigationPanel::GetDefaultFocusButton() const
{
	if (DefaultNavigationButton && DefaultNavigationButton->SupportsKeyboardFocus())
	{
		return DefaultNavigationButton;
	}
	else
	{
		for (TSharedPtr<SNeutronButton> Button : NavigationButtons)
		{
			if (Button->SupportsKeyboardFocus())
			{
				return Button;
			}
		}
	}

	return nullptr;
}

bool SNeutronNavigationPanel::IsButtonActionAllowed(TSharedPtr<SNeutronButton> Button) const
{
	TSharedPtr<SWidget> ButtonParentWidget = Button->GetParentWidget();
	while (ButtonParentWidget.IsValid() && ButtonParentWidget.Get() != this)
	{
		ButtonParentWidget = ButtonParentWidget->GetParentWidget();
	}

	return ButtonParentWidget.IsValid();
}

TArray<TSharedPtr<SNeutronButton>>& SNeutronNavigationPanel::GetNavigationButtons()
{
	return NavigationButtons;
}

void SNeutronNavigationPanel::ResetNavigation()
{
	if (Menu)
	{
		TSharedPtr<SNeutronButton> FocusButton = GetDefaultFocusButton();
		if (FocusButton && FocusButton->SupportsKeyboardFocus())
		{
			// NLOG("SNeutronNavigationPanel::ResetNavigation : reset to '%s'", *FocusButton->ToString());
			Menu->SetFocusedButton(FocusButton, true);
		}
		else
		{
			Menu->SetFocusedButton(nullptr, true);
		}
	}
}

bool SNeutronNavigationPanel::IsFocusedButtonInsideWidget(TSharedPtr<SWidget> Widget) const
{
	return Menu ? IsButtonInsideWidget(Menu->GetFocusedButton(), Widget) : false;
}

bool SNeutronNavigationPanel::IsButtonInsideWidget(TSharedPtr<SNeutronButton> Button, TSharedPtr<SWidget> Widget) const
{
	if (Button.IsValid() && NavigationButtons.Contains(Button))
	{
		const FVector2D& OriginPoint = Button->GetCachedGeometry().GetAbsolutePosition();
		const FVector2D& Size        = Button->GetCachedGeometry().GetAbsoluteSize();
		if (Widget->GetCachedGeometry().GetLayoutBoundingRect().ContainsPoint(OriginPoint) ||
			Widget->GetCachedGeometry().GetLayoutBoundingRect().ContainsPoint(OriginPoint + Size))
		{
			return true;
		}
	}

	return false;
}
