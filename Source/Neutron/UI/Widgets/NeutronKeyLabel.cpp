// Neutron - Gwennaël Arbona

#include "NeutronKeyLabel.h"
#include "Neutron/UI/NeutronUI.h"
#include "Neutron/System/NeutronMenuManager.h"

/*----------------------------------------------------
    Construct
----------------------------------------------------*/

void SNeutronKeyLabel::Construct(const FArguments& InArgs)
{
	// Setup
	const FNeutronMainTheme& Theme = FNeutronStyleSet::GetMainTheme();
	Action                         = InArgs._Action;
	ExplicitKey                    = InArgs._Key;
	CurrentAlpha                   = InArgs._Alpha;

	// clang-format off
	ChildSlot
	[
		SNew(SOverlay)

		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SImage)
			.Image(this, &SNeutronKeyLabel::GetKeyIcon)
			.ColorAndOpacity(this, &SNeutronKeyLabel::GetKeyIconColor)
		]

		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.TextStyle(&Theme.KeyFont)
			.Text(this, &SNeutronKeyLabel::GetKeyText)
			.ColorAndOpacity(this, &SNeutronKeyLabel::GetKeyTextColor)
		]
	];
	// clang-format on
}

/*----------------------------------------------------
    Callbacks
----------------------------------------------------*/

FKey SNeutronKeyLabel::GetBoundKey() const
{
	if (Action.Get() != NAME_None)
	{
		return UNeutronMenuManager::Get()->GetFirstActionKey(Action.Get());
	}
	else
	{
		return ExplicitKey.Get();
	}
}

FText SNeutronKeyLabel::GetKeyText() const
{
	return FNeutronStyleSet::GetKeyDisplay(GetBoundKey()).Key;
}

const FSlateBrush* SNeutronKeyLabel::GetKeyIcon() const
{
	return FNeutronStyleSet::GetKeyDisplay(GetBoundKey()).Value;
}

FSlateColor SNeutronKeyLabel::GetKeyIconColor() const
{
	return FLinearColor(1, 1, 1, CurrentAlpha.Get());
}

FSlateColor SNeutronKeyLabel::GetKeyTextColor() const
{
	const FNeutronMainTheme& Theme = FNeutronStyleSet::GetMainTheme();

	FLinearColor Color = Theme.KeyFont.ColorAndOpacity.GetSpecifiedColor();
	Color.A *= CurrentAlpha.Get();

	return Color;
}
