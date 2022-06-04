// Neutron - Gwennaël Arbona

#pragma once

#include "Neutron/UI/NeutronUI.h"
#include "Widgets/SCompoundWidget.h"

/** Info box color type */
enum class ENeutronInfoBoxType : uint8
{
	None,
	Positive,
	Negative,
	Neutral
};

/** Info box class */
class SNeutronInfoText : public SCompoundWidget
{
	SLATE_BEGIN_ARGS(SNeutronInfoText)
	{}

	SLATE_DEFAULT_SLOT(FArguments, Content)
	SLATE_ATTRIBUTE(ENeutronInfoBoxType, Type)
	SLATE_ATTRIBUTE(FText, Text)

	SLATE_END_ARGS()

public:

	SNeutronInfoText()
		: DisplayedType(ENeutronInfoBoxType::None), CurrentColor(0, 0, 0, 0), PreviousColor(0, 0, 0, 0), TimeSinceColorChange(0.0f)
	{}

	void Construct(const FArguments& InArgs)
	{
		const FNeutronMainTheme& Theme = FNeutronStyleSet::GetMainTheme();

		Type = InArgs._Type;

		// clang-format off
		ChildSlot
		[
			SNew(SBorder)
			.BorderImage(&Theme.White)
			.BorderBackgroundColor(this, &SNeutronInfoText::GetColor)
			.Padding(Theme.ContentPadding)
			[
				SNew(SRichTextBlock)
				.Text(InArgs._Text)
				.TextStyle(&Theme.MainFont)
				.DecoratorStyleSet(&FNeutronStyleSet::GetStyle())
				+ SRichTextBlock::ImageDecorator()
			]
		];
		// clang-format on
	}

	virtual void Tick(const FGeometry& AllottedGeometry, const double CurrentTime, const float DeltaTime) override
	{
		SCompoundWidget::Tick(AllottedGeometry, CurrentTime, DeltaTime);

		if (Type.IsSet() || Type.IsBound())
		{
			ENeutronInfoBoxType DesiredType = Type.Get();

			if (DesiredType != ENeutronInfoBoxType::None && DisplayedType == ENeutronInfoBoxType::None)
			{
				CurrentColor = GetDesiredColor(DesiredType);

				DisplayedType        = DesiredType;
				TimeSinceColorChange = 0.0f;
				PreviousColor        = CurrentColor;
			}
			else if (DesiredType != DisplayedType)
			{
				FLinearColor DesiredColor = GetDesiredColor(DesiredType);

				float Alpha = FMath::Clamp(TimeSinceColorChange / ENeutronUIConstants::FadeDurationShort, 0.0f, 1.0f);
				TimeSinceColorChange += DeltaTime;

				CurrentColor = FMath::InterpEaseInOut(PreviousColor, DesiredColor, Alpha, ENeutronUIConstants::EaseStandard);

				if (Alpha >= 1.0f)
				{
					DisplayedType        = DesiredType;
					TimeSinceColorChange = 0.0f;
					PreviousColor        = CurrentColor;
				}
			}
		}
	}

protected:

	FSlateColor GetColor() const
	{
		return CurrentColor;
	}

	FLinearColor GetDesiredColor(ENeutronInfoBoxType DesiredType) const
	{
		const FNeutronMainTheme& Theme = FNeutronStyleSet::GetMainTheme();
		switch (DesiredType)
		{
			case ENeutronInfoBoxType::Positive:
				return Theme.PositiveColor;
			case ENeutronInfoBoxType::Negative:
				return Theme.NegativeColor;
			case ENeutronInfoBoxType::Neutral:
				return Theme.NeutralColor;
		}

		return FLinearColor(0, 0, 0, 0);
	}

protected:

	// General state
	TAttribute<ENeutronInfoBoxType> Type;
	ENeutronInfoBoxType             DisplayedType;
	FLinearColor                    CurrentColor;
	FLinearColor                    PreviousColor;
	float                           TimeSinceColorChange;
};
