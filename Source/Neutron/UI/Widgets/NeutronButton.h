// Neutron - Gwennaël Arbona

#pragma once

#include "Neutron/UI/NeutronUI.h"

#include "Widgets/Input/SButton.h"

/*----------------------------------------------------
    Supporting types
----------------------------------------------------*/

/** User callback type to alter the button size */
DECLARE_DELEGATE_RetVal(float, FNeutronButtonUserSizeCallback);

/** Animation state used by lists to seamlessly animate on refresh */
struct FNeutronButtonState
{
	FNeutronButtonState()
		: CurrentColorAnimationAlpha(0)
		, CurrentSizeAnimationAlpha(0)
		, CurrentUserSizeAnimationAlpha(0)
		, CurrentDisabledAnimationAlpha(0)
		, CurrentTimeSinceClicked(10000)
	{}

	float CurrentColorAnimationAlpha;
	float CurrentSizeAnimationAlpha;
	float CurrentUserSizeAnimationAlpha;
	float CurrentDisabledAnimationAlpha;

	float CurrentTimeSinceClicked;
};

/** Button layout imposter class */
class NEUTRON_API SNeutronButtonLayout : public SBox
{
	SLATE_BEGIN_ARGS(SNeutronButtonLayout) : _Theme("DefaultButton"), _Size(NAME_None)
	{}

	SLATE_ARGUMENT(FName, Theme)
	SLATE_ARGUMENT(FName, Size)
	SLATE_DEFAULT_SLOT(FArguments, Content)

	SLATE_END_ARGS()

public:

	void Construct(const FArguments& InArgs)
	{
		// Build padding
		const FNeutronButtonTheme& Theme   = FNeutronStyleSet::GetButtonTheme(InArgs._Theme);
		FMargin                    Padding = Theme.HoverAnimationPadding + 1;
		Padding.Top                        = 0;
		Padding.Bottom                     = 0;

		// clang-format off
		if (InArgs._Size != NAME_None)
		{
			const FNeutronButtonSize&  Size  = FNeutronStyleSet::GetButtonSize(InArgs._Size);

			SBox::Construct(SBox::FArguments()
				.WidthOverride(Size.Width)
				.MinDesiredHeight(Size.Height)
				.Padding(Padding)
				[
					InArgs._Content.Widget
				]
			);
		}
		else
		{
			SBox::Construct(SBox::FArguments()
				.Padding(Padding)
				[
					InArgs._Content.Widget
				]
			);
		}
		// clang-format on
	}
};

/*----------------------------------------------------
    Button class
----------------------------------------------------*/

/** Base button class */
class NEUTRON_API SNeutronButton : public SButton
{
	/*----------------------------------------------------
	    Slate arguments
	----------------------------------------------------*/

	SLATE_BEGIN_ARGS(SNeutronButton)
		: _Theme("DefaultButton")
		, _Size("DefaultButtonSize")
		, _BorderRotation(0)
		, _Enabled(true)
		, _Focusable(true)
		, _ActionFocusable(true)
		, _Toggle(false)
	{}

	SLATE_ATTRIBUTE(FText, Text)
	SLATE_ATTRIBUTE(FText, HelpText)
	SLATE_ATTRIBUTE(FName, Action)
	SLATE_ATTRIBUTE(const FSlateBrush*, Icon)
	SLATE_ARGUMENT(FName, Theme)
	SLATE_ARGUMENT(FName, Size)
	SLATE_ARGUMENT(FNeutronButtonUserSizeCallback, UserSizeCallback)
	SLATE_ARGUMENT(float, BorderRotation)

	SLATE_ATTRIBUTE(bool, Enabled)
	SLATE_ATTRIBUTE(bool, Focusable)
	SLATE_ARGUMENT(bool, ActionFocusable)
	SLATE_ARGUMENT(bool, Toggle)

	SLATE_NAMED_SLOT(FArguments, Header)
	SLATE_NAMED_SLOT(FArguments, Footer)
	SLATE_NAMED_SLOT(FArguments, Content)

	SLATE_EVENT(FSimpleDelegate, OnFocused)
	SLATE_EVENT(FSimpleDelegate, OnClicked)
	SLATE_EVENT(FSimpleDelegate, OnDoubleClicked)

	SLATE_END_ARGS()

public:

	void Construct(const FArguments& InArgs);

	/*----------------------------------------------------
	    Public methods
	----------------------------------------------------*/

	virtual void Tick(const FGeometry& AllottedGeometry, const double CurrentTime, const float DeltaTime) override;

	virtual void OnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

	virtual void OnMouseLeave(const FPointerEvent& MouseEvent) override;

	bool SupportsKeyboardFocus() const override
	{
		return ButtonFocusable.Get(true);
	}

	/** Mouse click	*/
	virtual FReply OnButtonClicked();

	/** Mouse double click */
	virtual FReply OnMouseButtonDoubleClick(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

	/** Set the focused state */
	void SetFocused(bool State);

	/** Get the focused state */
	bool IsFocused() const
	{
		return Focused;
	}

	/** Check if this button can toggle focus through its action button */
	bool IsButtonActionFocusable() const
	{
		return ButtonActionFocusable;
	}

	/** Check if this button is locked */
	bool IsButtonEnabled() const
	{
		return ButtonEnabled.Get(true);
	}

	/** Set the active state */
	void SetActive(bool ActiveState)
	{
		Active = ActiveState;
	}

	/** Get the active state */
	bool IsActive() const
	{
		return Active;
	}

	/** Force a new text */
	void SetText(FText NewText);

	/** Force a new help text */
	void SetHelpText(FText NewText);

	/** Pass horizontal input through the widget, return true if consumed */
	virtual bool HorizontalAnalogInput(float Value)
	{
		return false;
	}

	/** Pass vertical input through the widget, return true if consumed */
	virtual bool VerticalAnalogInput(float Value)
	{
		return false;
	}

	/** Get the current help text */
	FText GetHelpText()
	{
		return HelpText.Get();
	}

	/** Get the internal state */
	FNeutronButtonState& GetState()
	{
		return State;
	}

	/** Get the action binding for closing this menu */
	FName GetActionName() const;

	/*----------------------------------------------------
	    Callbacks
	----------------------------------------------------*/

protected:

	/** Icon brush callback */
	const FSlateBrush* GetIconBrush() const;

	/** Activity overlay color */
	FSlateColor GetBackgroundBrushColor() const;

	/** Overall button color */
	FLinearColor GetMainColor() const;

	/** Button border color */
	FSlateColor GetBorderColor() const;

	/** Current width callback (visual box) */
	FOptionalSize GetVisualWidth() const;

	/** Current height callback (visual box) */
	FOptionalSize GetVisualHeight() const;

	/** Current width callback (actual layout) */
	FOptionalSize GetLayoutWidth() const;

	/** Current height callback (actual layout) */
	FOptionalSize GetLayoutHeight() const;

	/** Get the text font */
	FSlateFontInfo GetFont() const;

	/** Get the render transform of the border */
	TOptional<FSlateRenderTransform> GetBorderRenderTransform() const;

protected:

	/*----------------------------------------------------
	    Private data
	----------------------------------------------------*/

	// Settings & attributes
	TAttribute<bool>                      ButtonEnabled;
	TAttribute<bool>                      ButtonFocusable;
	bool                                  ButtonActionFocusable;
	TAttribute<FText>                     Text;
	TAttribute<FText>                     HelpText;
	TAttribute<FName>                     Action;
	TAttribute<const struct FSlateBrush*> Icon;
	FName                                 ThemeName;
	FName                                 SizeName;
	float                                 BorderRotation;
	bool                                  IsToggle;
	float                                 WrapSize;

	// Callbacks
	FSimpleDelegate                OnFocused;
	FSimpleDelegate                OnClicked;
	FSimpleDelegate                OnDoubleClicked;
	FNeutronButtonUserSizeCallback UserSizeCallback;

	// State
	bool                Focused;
	bool                Hovered;
	bool                Active;
	FNeutronButtonState State;

	// Slate elements
	TSharedPtr<class SBorder>    InnerContainer;
	TSharedPtr<class STextBlock> TextBlock;

public:

	/*----------------------------------------------------
	    Get & Set
	----------------------------------------------------*/

	TSharedPtr<SBorder> GetContainer() const
	{
		return InnerContainer;
	}
};
