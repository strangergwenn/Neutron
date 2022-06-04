// Neutron - Gwennaël Arbona

#include "NeutronKeyLabel.h"
#include "Neutron/UI/NeutronUI.h"

/*----------------------------------------------------
    Construct
----------------------------------------------------*/

void SNeutronKeyLabel::Construct(const FArguments& InArgs)
{
	// Setup
	const FNeutronMainTheme& Theme = FNeutronStyleSet::GetMainTheme();
	KeyName                        = InArgs._Key;
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

FText SNeutronKeyLabel::GetKeyText() const
{
	return FNeutronStyleSet::GetKeyDisplay(KeyName.Get()).Key;
}

const FSlateBrush* SNeutronKeyLabel::GetKeyIcon() const
{
	return FNeutronStyleSet::GetKeyDisplay(KeyName.Get()).Value;
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
