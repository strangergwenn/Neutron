// Neutron - Gwennaël Arbona

#pragma once

#include "NeutronNavigationPanel.h"
#include "Neutron/UI/NeutronUI.h"
#include "Neutron/Neutron.h"

#include "Widgets/SCompoundWidget.h"

/** Modal panel that blocks input, steals focus and blurs the background */
class NEUTRON_API SNeutronModalPanel : public SNeutronNavigationPanel
{
	/*----------------------------------------------------
	    Slate arguments
	----------------------------------------------------*/

	SLATE_BEGIN_ARGS(SNeutronModalPanel) : _Menu(nullptr)
	{}

	SLATE_ARGUMENT(class SNeutronMenu*, Menu)
	SLATE_ARGUMENT(FNeutronAsyncCondition, IsConfirmEnabled)

	SLATE_END_ARGS()

public:

	void Construct(const FArguments& InArgs);

	/*----------------------------------------------------
	    Interaction
	----------------------------------------------------*/

	virtual void Tick(const FGeometry& AllottedGeometry, const double CurrentTime, const float DeltaTime) override;

	/** Show the panel and take focus, with optional callbacks and an optional content block */
	void Show(FText Title, FText Text, FSimpleDelegate NewOnConfirmed, FSimpleDelegate NewOnIgnore = FSimpleDelegate(),
		FSimpleDelegate NewOnCancel = FSimpleDelegate(), TSharedPtr<SWidget> Content = TSharedPtr<SWidget>());

	/** Hide the panel, set focus back to the parent */
	void Hide();

	/** Check if this panel is visible */
	bool IsVisible() const
	{
		return CurrentAlpha > 0;
	}

	virtual bool IsModal() const override
	{
		return true;
	}

	/** Set the text data on buttons */
	virtual void UpdateButtons();

	/*----------------------------------------------------
	    Content callbacks
	----------------------------------------------------*/

protected:

	virtual EVisibility GetConfirmVisibility() const;

	virtual bool IsConfirmEnabled() const;

	virtual EVisibility GetDismissVisibility() const;

	FLinearColor GetColor() const;

	FSlateColor GetBackgroundColor() const;

	/*----------------------------------------------------
	    Callbacks
	----------------------------------------------------*/

protected:

	/** The panel has been asked by the user to close by confirming */
	void OnConfirmPanel();

	/** The panel has been asked by the user to close without choosing */
	void OnDismissPanel();

	/** The panel has been asked by the user to close by canceling */
	void OnCancelPanel();

	/*----------------------------------------------------
	    Private data
	----------------------------------------------------*/

protected:

	// Parent menu reference
	SNeutronNavigationPanel* ParentPanel;

	// Settings
	float                  FadeDuration;
	FSimpleDelegate        OnConfirmed;
	FSimpleDelegate        OnDismissed;
	FSimpleDelegate        OnCancelled;
	FNeutronAsyncCondition OnIsConfirmEnabled;

	// Widgets
	TSharedPtr<STextBlock>           TitleText;
	TSharedPtr<STextBlock>           InformationText;
	TSharedPtr<SBox>                 ContentBox;
	TSharedPtr<SWidget>              InternalWidget;
	TSharedPtr<class SNeutronButton> ConfirmButton;
	TSharedPtr<class SNeutronButton> DismissButton;
	TSharedPtr<class SNeutronButton> CancelButton;

	// Data
	bool  ShouldShow;
	float CurrentFadeTime;
	float CurrentAlpha;
};
