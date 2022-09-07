// Neutron - Gwennaël Arbona

#pragma once

#include "Neutron/UI/NeutronUI.h"

/*----------------------------------------------------
    Table entry type
----------------------------------------------------*/

/** Table entry type */
template <typename T>
class TNeutronTableValue
{
public:

	TNeutronTableValue(T Value, const FText& Unit = FText()) : CurrentValue(Value), ValueUnit(Unit), ExplicitColor(FLinearColor::Black)
	{
		ReferenceValue = GetDefaultValue();
	}

	TNeutronTableValue(T Value, const FLinearColor& Color) : CurrentValue(Value), ValueUnit(), ExplicitColor(Color)
	{
		ReferenceValue = GetDefaultValue();
	}

	TNeutronTableValue(T Value, T Reference, const FText& Unit)
		: CurrentValue(Value), ReferenceValue(Reference), ValueUnit(Unit), ExplicitColor(FLinearColor::Black)
	{}

	// Generic number-focused display
	inline TPair<FText, FLinearColor> GetDisplay() const
	{
		// Fetch the style data
		const FNeutronMainTheme& Theme = FNeutronStyleSet::GetMainTheme();
		FNumberFormattingOptions Options;
		Options.MaximumFractionalDigits            = 1;
		FNumberFormattingOptions DifferenceOptions = Options;
		DifferenceOptions.AlwaysSign               = true;

		// Build the value string
		FLinearColor Color = FLinearColor::White;
		FText        Text  = FText::FromString(FText::AsNumber(CurrentValue, &Options).ToString() + " " + ValueUnit.ToString());

		// Explicit color mode
		if (ExplicitColor != FLinearColor::Black)
		{
			Color = ExplicitColor;
		}

		// Comparison mode
		else if (ReferenceValue != GetDefaultValue() && CurrentValue != ReferenceValue)
		{
			Color = CurrentValue > ReferenceValue ? Theme.PositiveColor : Theme.NegativeColor;
			Text  = FText::FromString(
				 Text.ToString() + " (" + FText::AsNumber(CurrentValue - ReferenceValue, &DifferenceOptions).ToString() + ")");
		}

		return TPair<FText, FLinearColor>(Text, Color);
	}

	// Generic number-focused default
	inline T GetDefaultValue() const
	{
		return -1;
	}

protected:

	T            CurrentValue;
	T            ReferenceValue;
	FText        ValueUnit;
	FLinearColor ExplicitColor;
};

// Text override
template <>
inline TPair<FText, FLinearColor> TNeutronTableValue<FText>::GetDisplay() const
{
	// Fetch the style data
	const FNeutronMainTheme& Theme = FNeutronStyleSet::GetMainTheme();

	// Pick a color
	FLinearColor Color = FLinearColor::White;
	if (ExplicitColor != FLinearColor::Black)
	{
		Color = ExplicitColor;
	}
	else if (!ReferenceValue.IsEmpty() && !CurrentValue.EqualTo(ReferenceValue))
	{
		Color = Theme.PositiveColor;
	}

	return TPair<FText, FLinearColor>(CurrentValue, Color);
}

// Text override
template <>
inline FText TNeutronTableValue<FText>::GetDefaultValue() const
{
	return FText();
}

/*----------------------------------------------------
    Actual table type
----------------------------------------------------*/

/** Generic table type */
template <int32 ColumnCount>
class SNeutronTable : public SCompoundWidget
{
	SLATE_BEGIN_ARGS(SNeutronTable) : _Width(FOptionalSize())
	{}

	SLATE_ARGUMENT(FText, Title)
	SLATE_ARGUMENT(FOptionalSize, Width)

	SLATE_END_ARGS()

public:

	SNeutronTable() : SCompoundWidget(), EvenRow(true)
	{}

	void Construct(const FArguments& InArgs)
	{
		const FNeutronMainTheme& Theme = FNeutronStyleSet::GetMainTheme();

		Width = InArgs._Width.IsSet() && InArgs._Width.Get() > 0 ? InArgs._Width : Theme.GenericMenuWidth;

		// clang-format off
		ChildSlot
		[
			SNew(SBox)
			.WidthOverride(Width)
			[
				SAssignNew(TableBox, SVerticalBox)

				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(STextBlock)
					.Text(InArgs._Title)
					.TextStyle(&Theme.HeadingFont)
				]
			]
		];
		// clang-format on
	}

	/** Empty the table */
	void Clear()
	{
		TableBox->ClearChildren();
	}

	/** Add a table header */
	void AddHeader(FText Label, FText Details = FText())
	{
		AddHeaders(Label, {Details});
	}

	/** Add a table header */
	void AddHeaders(FText Label, const TArray<FText>& Details)
	{
		const FNeutronMainTheme& Theme = FNeutronStyleSet::GetMainTheme();

		TArray<TPair<FText, FLinearColor>> ColorValues;
		for (const FText& Text : Details)
		{
			ColorValues.Add(TPair<FText, FLinearColor>(Text, FLinearColor::White));
		}

		AddRow(Label, ColorValues, Theme.InfoFont, true);
	}

	/** Add a numerical table entry */
	template <typename T>
	void AddEntry(FText Label, TNeutronTableValue<T> Value)
	{
		AddEntries<T>(Label, {Value});
	}

	/** Add a numerical table entry */
	template <typename T>
	void AddEntries(FText Label, const TArray<TNeutronTableValue<T>>& Values)
	{
		const FNeutronMainTheme& Theme = FNeutronStyleSet::GetMainTheme();

		TArray<TPair<FText, FLinearColor>> ColorValues;
		for (const auto& Value : Values)
		{
			ColorValues.Add(Value.GetDisplay());
		}

		AddRow(Label, ColorValues, Theme.MainFont, false);
	}

protected:

	/** Add a generic row to the table */
	void AddRow(FText Label, const TArray<TPair<FText, FLinearColor>>& Values, const FTextBlockStyle& TextStyle, bool Header)
	{
		const FNeutronMainTheme&   Theme = FNeutronStyleSet::GetMainTheme();
		TSharedPtr<SHorizontalBox> Row;

		// clang-format off
		TableBox->AddSlot()
		.AutoHeight()
		.Padding(Header ? Theme.VerticalContentPadding : FMargin(0))
		[
			SNew(SBorder)
			.BorderImage(Header ? &Theme.TableHeaderBackground : (EvenRow ? &Theme.TableEvenBackground : &Theme.TableOddBackground))
			[
				SAssignNew(Row, SHorizontalBox)

				+ SHorizontalBox::Slot()
				[
					SNew(SRichTextBlock)
					.Text(Label)
					.TextStyle(&TextStyle)
					.WrapTextAt(Width.Get() / ColumnCount)
					.DecoratorStyleSet(&FNeutronStyleSet::GetStyle())
					+ SRichTextBlock::ImageDecorator()
				]
			]
		];
		
		for (const TPair<FText, FLinearColor>& TextAndColor : Values)
		{
			Row->AddSlot()
			[
				SNew(STextBlock)
				.Text(TextAndColor.Key)
				.TextStyle(&TextStyle)
				.WrapTextAt(Width.Get() / ColumnCount)
				.ColorAndOpacity(TextAndColor.Value)
			];
		}

		// clang-format on

		EvenRow = !EvenRow;
	}

protected:

	TSharedPtr<SVerticalBox> TableBox;
	bool                     EvenRow;
	FOptionalSize            Width;
};

#undef LOCTEXT_NAMESPACE
