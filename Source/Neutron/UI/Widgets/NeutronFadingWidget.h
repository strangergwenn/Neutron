// Neutron - Gwennaël Arbona

#pragma once

#include "Neutron/UI/NeutronUI.h"
#include "Widgets/SCompoundWidget.h"

/** Simple widget that can be displayed with fade-in and fade-out */
template <bool FadeOut = true>
class SNeutronFadingWidget : public SCompoundWidget
{
	/*----------------------------------------------------
	    Slate arguments
	----------------------------------------------------*/

	SLATE_BEGIN_ARGS(SNeutronFadingWidget) : _FadeDuration(ENeutronUIConstants::FadeDurationShort), _ColorAndOpacity(FLinearColor::White)
	{}

	SLATE_ARGUMENT(float, FadeDuration)
	SLATE_ARGUMENT(float, DisplayDuration)
	SLATE_ATTRIBUTE(FLinearColor, ColorAndOpacity)
	SLATE_DEFAULT_SLOT(FArguments, Content)

	SLATE_END_ARGS()

	/*----------------------------------------------------
	    Slate interface
	----------------------------------------------------*/

public:

	SNeutronFadingWidget() : CurrentFadeTime(0), CurrentDisplayTime(0), CurrentAlpha(0)
	{}

	void Construct(const FArguments& InArgs)
	{
		// Settings
		FadeDuration           = InArgs._FadeDuration;
		DisplayDuration        = InArgs._DisplayDuration;
		DesiredColorAndOpacity = InArgs._ColorAndOpacity;

		// clang-format off
		ChildSlot
		[
			InArgs._Content.Widget
		];
		// clang-format on

		// Defaults
		Reset();
	}

	virtual void Tick(const FGeometry& AllottedGeometry, const double CurrentTime, const float DeltaTime) override
	{
		SCompoundWidget::Tick(AllottedGeometry, CurrentTime, DeltaTime);

		// Update time
		if ((FadeOut && CurrentDisplayTime > DisplayDuration) || IsDirty() || IsHidden())
		{
			CurrentFadeTime -= DeltaTime;
		}
		else
		{
			CurrentFadeTime += DeltaTime;
		}
		CurrentFadeTime = FMath::Clamp(CurrentFadeTime, 0.0f, FadeDuration);
		CurrentAlpha    = FMath::InterpEaseInOut(0.0f, 1.0f, CurrentFadeTime / FadeDuration, ENeutronUIConstants::EaseStandard);

		// Update when invisible
		if (CurrentFadeTime <= 0 && IsDirty())
		{
			CurrentDisplayTime = 0;

			OnUpdate();
		}
		else
		{
			CurrentDisplayTime += DeltaTime;
		}
	}

	/** Reset the fading system */
	void Reset()
	{
		CurrentFadeTime    = 0;
		CurrentDisplayTime = 0;
		CurrentAlpha       = 0;
		CurrentDisplayTime = DisplayDuration;
	}

	/*----------------------------------------------------
	    Virtual interface
	----------------------------------------------------*/

protected:

	/** Return true to request a widget update */
	virtual bool IsDirty() const
	{
		return false;
	}

	/** Return true to request staying hidden */
	virtual bool IsHidden() const
	{
		return false;
	}

	/** Update the widget's contents */
	virtual void OnUpdate()
	{}

	/*----------------------------------------------------
	    Callbacks
	----------------------------------------------------*/

public:

	float GetCurrentAlpha() const
	{
		return CurrentAlpha;
	}

	FLinearColor GetLinearColor() const
	{
		FLinearColor Color = DesiredColorAndOpacity.Get();
		Color.A *= CurrentAlpha > 0.1f ? CurrentAlpha : 0.0f;
		return Color;
	}

	FSlateColor GetSlateColor() const
	{
		return GetLinearColor();
	}

	FSlateColor GetTextColor(const FTextBlockStyle& TextStyle) const
	{
		FLinearColor Color = TextStyle.ColorAndOpacity.GetSpecifiedColor();
		Color.A *= CurrentAlpha;

		return Color;
	}

	TOptional<int32> GetBlurRadius() const
	{
		const FNeutronMainTheme& Theme = FNeutronStyleSet::GetMainTheme();

		float CorrectedAlpha =
			FMath::Clamp((CurrentAlpha - ENeutronUIConstants::BlurAlphaOffset) / (1.0f - ENeutronUIConstants::BlurAlphaOffset), 0.0f, 1.0f);
		float BlurAlpha = FMath::InterpEaseInOut(0.0f, 1.0f, CorrectedAlpha, ENeutronUIConstants::EaseStandard);

		return static_cast<int32>(BlurAlpha * Theme.BlurRadius);
	}

	float GetBlurStrength() const
	{
		const FNeutronMainTheme& Theme = FNeutronStyleSet::GetMainTheme();

		float CorrectedAlpha =
			FMath::Clamp((CurrentAlpha - ENeutronUIConstants::BlurAlphaOffset) / (1.0f - ENeutronUIConstants::BlurAlphaOffset), 0.0f, 1.0f);
		float BlurAlpha = FMath::InterpEaseInOut(0.0f, 1.0f, CorrectedAlpha, ENeutronUIConstants::EaseStandard);

		return BlurAlpha * Theme.BlurStrength;
	}

	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

protected:

	// Settings
	float                    FadeDuration;
	float                    DisplayDuration;
	TAttribute<FLinearColor> DesiredColorAndOpacity;

	// Current state
	float CurrentFadeTime;
	float CurrentDisplayTime;
	float CurrentAlpha;
};

/** Image callback */
DECLARE_DELEGATE_RetVal(const FSlateBrush*, FNeutronImageGetter);

/** Simple SImage analog that fades smoothly when the image changes */
class SNeutronImage : public SNeutronFadingWidget<false>
{
	SLATE_BEGIN_ARGS(SNeutronImage) : _ColorAndOpacity(FLinearColor::White)
	{}

	SLATE_ARGUMENT(FNeutronImageGetter, Image)
	SLATE_ATTRIBUTE(FLinearColor, ColorAndOpacity)

	SLATE_END_ARGS()

public:

	void Construct(const FArguments& InArgs)
	{
		const FNeutronMainTheme& Theme = FNeutronStyleSet::GetMainTheme();

		Getter = InArgs._Image;

		// clang-format off
		SNeutronFadingWidget::Construct(SNeutronFadingWidget::FArguments()
			.FadeDuration(ENeutronUIConstants::FadeDurationShort)
			.DisplayDuration(4.0f)
			.ColorAndOpacity(InArgs._ColorAndOpacity)
		);

		ChildSlot
		[
			SNew(SImage)
			.Image(this, &SNeutronImage::GetImage)
			.ColorAndOpacity(this, &SNeutronFadingWidget::GetSlateColor)

		];
		// clang-format on
	}

	virtual void Tick(const FGeometry& AllottedGeometry, const double CurrentTime, const float DeltaTime)
	{
		SNeutronFadingWidget::Tick(AllottedGeometry, CurrentTime, DeltaTime);

		if (Getter.IsBound())
		{
			DesiredImage = Getter.Execute();
		}
	}

	virtual bool IsDirty() const override
	{
		return DesiredImage != CurrentImage;
	}

	virtual void OnUpdate() override
	{
		CurrentImage = DesiredImage;
	}

	const FSlateBrush* GetImage() const
	{
		return CurrentImage;
	}

private:

	const FSlateBrush*  DesiredImage;
	const FSlateBrush*  CurrentImage;
	FNeutronImageGetter Getter;
};

/** Text callback */
DECLARE_DELEGATE_RetVal(FText, FNeutronTextGetter);

/** Simple STextBlock analog that fades smoothly when the text changes */
class SNeutronText : public SNeutronFadingWidget<false>
{
	SLATE_BEGIN_ARGS(SNeutronText) : _AutoWrapText(false), _Justification(ETextJustify::Left)
	{}

	SLATE_ARGUMENT(FNeutronTextGetter, Text)
	SLATE_STYLE_ARGUMENT(FTextBlockStyle, TextStyle)
	SLATE_ATTRIBUTE(float, WrapTextAt)
	SLATE_ATTRIBUTE(bool, AutoWrapText)
	SLATE_ATTRIBUTE(EVisibility, Visibility)
	SLATE_ATTRIBUTE(ETextJustify::Type, Justification)

	SLATE_END_ARGS()

public:

	void Construct(const FArguments& InArgs)
	{
		const FNeutronMainTheme& Theme = FNeutronStyleSet::GetMainTheme();

		Getter = InArgs._Text;

		// clang-format off
		SNeutronFadingWidget::Construct(SNeutronFadingWidget::FArguments()
			.FadeDuration(ENeutronUIConstants::FadeDurationShort)
			.DisplayDuration(4.0f)
		);

		ChildSlot
		[
			SAssignNew(TextBlock, STextBlock)
			.Text(this, &SNeutronText::GetText)
			.TextStyle(InArgs._TextStyle)
			.WrapTextAt(InArgs._WrapTextAt)
			.AutoWrapText(InArgs._AutoWrapText)
			.ColorAndOpacity(this, &SNeutronFadingWidget::GetSlateColor)
			.Visibility(InArgs._Visibility)
			.Justification(InArgs._Justification)
		];
		// clang-format on
	}

	void SetText(const FText& Text, bool InstantUpdate = false)
	{
		if (Getter.IsBound())
		{
			Getter.Unbind();
		}

		DesiredText = Text;
		if (InstantUpdate && !CurrentText.IsEmpty())
		{
			CurrentText = Text;
		}
	}

	FText GetText() const
	{
		return CurrentText;
	}

	virtual void Tick(const FGeometry& AllottedGeometry, const double CurrentTime, const float DeltaTime)
	{
		SNeutronFadingWidget::Tick(AllottedGeometry, CurrentTime, DeltaTime);

		if (Getter.IsBound())
		{
			DesiredText = Getter.Execute();
		}
	}

	virtual bool IsDirty() const override
	{
		return !DesiredText.EqualTo(CurrentText);
	}

	virtual void OnUpdate() override
	{
		CurrentText = DesiredText;
	}

protected:

	FText                  DesiredText;
	FText                  CurrentText;
	FNeutronTextGetter     Getter;
	TSharedPtr<STextBlock> TextBlock;
};

/** Simple SRichTextBlock analog that fades smoothly when the text changes */
class SNeutronRichText : public SNeutronText
{
	SLATE_BEGIN_ARGS(SNeutronRichText) : _AutoWrapText(false), _Justification(ETextJustify::Left)
	{}

	SLATE_ARGUMENT(FNeutronTextGetter, Text)
	SLATE_STYLE_ARGUMENT(FTextBlockStyle, TextStyle)
	SLATE_ATTRIBUTE(float, WrapTextAt)
	SLATE_ATTRIBUTE(bool, AutoWrapText)
	SLATE_ATTRIBUTE(ETextJustify::Type, Justification)

	SLATE_END_ARGS()

public:

	void Construct(const FArguments& InArgs)
	{
		const FNeutronMainTheme& Theme = FNeutronStyleSet::GetMainTheme();

		Getter = InArgs._Text;

		// clang-format off
		SNeutronFadingWidget::Construct(SNeutronFadingWidget::FArguments()
			.FadeDuration(ENeutronUIConstants::FadeDurationShort)
			.DisplayDuration(4.0f)
		);

		ChildSlot
		[
			SNew(SBorder)
			.BorderImage(new FSlateNoResource)
			.ColorAndOpacity(this, &SNeutronFadingWidget::GetLinearColor)
			.Padding(0)
			[
				SNew(SRichTextBlock)
				.Text(this, &SNeutronText::GetText)
				.TextStyle(InArgs._TextStyle)
				.WrapTextAt(InArgs._WrapTextAt)
				.AutoWrapText(InArgs._AutoWrapText)
				.Justification(InArgs._Justification)
				.DecoratorStyleSet(&FNeutronStyleSet::GetStyle())
				+ SRichTextBlock::ImageDecorator()
			]

		];
		// clang-format on
	}
};
