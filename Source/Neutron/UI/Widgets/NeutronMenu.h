// Neutron - Gwennaël Arbona

#pragma once

#include "Neutron/Actor/NeutronActorTools.h"
#include "Neutron/UI/NeutronUI.h"

#include "Widgets/SCompoundWidget.h"

class NEUTRON_API SNeutronMenu : public SCompoundWidget
{
	/*----------------------------------------------------
	    Slate arguments
	----------------------------------------------------*/

	SLATE_BEGIN_ARGS(SNeutronMenu) : _MenuManager(nullptr)
	{}

	SLATE_ARGUMENT(TWeakObjectPtr<class UNeutronMenuManager>, MenuManager)

	SLATE_END_ARGS()

public:

	SNeutronMenu();

	void Construct(const FArguments& InArgs);

	/*----------------------------------------------------
	    Interaction
	----------------------------------------------------*/

	virtual void Tick(const FGeometry& AllottedGeometry, const double CurrentTime, const float DeltaTime) override;

	virtual bool SupportsKeyboardFocus() const override;

	virtual void OnFocusLost(const FFocusEvent& InFocusEvent) override;

	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

	virtual FReply OnMouseButtonDoubleClick(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

	virtual FReply OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

	virtual void OnMouseLeave(const FPointerEvent& MouseEvent) override;

	virtual FReply OnAnalogValueChanged(const FGeometry& MyGeometry, const FAnalogInputEvent& AnalogInputEvent) override;

	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& KeyEvent) override;

	/*----------------------------------------------------
	    Main interface
	----------------------------------------------------*/

	/** Show this menu */
	virtual void Show()
	{}

	/** Start hiding this menu */
	virtual void Hide()
	{}

	/** Start displaying the tooltip */
	virtual void ShowTooltip(SWidget* TargetWidget, FText Content)
	{}

	/** Stop displaying the tooltip */
	virtual void HideTooltip(SWidget* TargetWidget)
	{}

	/*----------------------------------------------------
	    Input handling
	----------------------------------------------------*/

	/** Reload the key bindings map */
	void UpdateKeyBindings();

	/** Check if a specific action is mapped to that specific key */
	bool IsActionKey(FName ActionName, const FKey& Key) const;

	/** Check if a specific axis is mapped to that specific key */
	bool IsAxisKey(FName AxisName, const FKey& Key) const;

	/*----------------------------------------------------
	    Navigation handling
	----------------------------------------------------*/

	/** Set the currently active navigation panel */
	void SetNavigationPanel(class SNeutronNavigationPanel* Panel);

	/** Unset the currently active navigation panel */
	void ClearNavigationPanel();

	/** Refresh the current navigation panel */
	void RefreshNavigationPanel();

	/** Get the currently active navigation panel */
	class SNeutronNavigationPanel* GetActiveNavigationPanel() const
	{
		return CurrentNavigationPanel;
	}

	/** Check if a panel is currently used for navigation */
	bool IsActiveNavigationPanel(const SNeutronNavigationPanel* Panel) const
	{
		return (CurrentNavigationPanel == Panel);
	}

	/** Create a new modal panel */
	template <typename T = class SNeutronModalPanel>
	TSharedPtr<T> CreateModalPanel()
	{
		TSharedPtr<T> Panel;

		MainOverlay->AddSlot()[SAssignNew(Panel, T).Menu(this)];

		return Panel;
	}

	/** Set a currently active navigation panel */
	void SetModalNavigationPanel(class SNeutronNavigationPanel* Panel);

	/** Set a currently active navigation panel */
	void ClearModalNavigationPanel();

	/** Set FocusButton focused, unfocus others*/
	void SetFocusedButton(TSharedPtr<class SNeutronButton> FocusButton, bool FromNavigation);

	/** Get the currently focused button */
	TSharedPtr<class SNeutronButton> GetFocusedButton();

protected:

	/** Create a new button that can be triggered by actions */
	template <typename WidgetType, typename RequiredArgsPayloadType>
	TSlateDecl<WidgetType, RequiredArgsPayloadType> NewNeutronButton(
		const ANSICHAR* InType, const ANSICHAR* InFile, int32 OnLine, RequiredArgsPayloadType&& InRequiredArgs, bool DefaultFocus)
	{
		auto Button =
			TSlateDecl<WidgetType, RequiredArgsPayloadType>(InType, InFile, OnLine, Forward<RequiredArgsPayloadType>(InRequiredArgs));

		AdditionalActionButtons.Add(Button._Widget);

		return Button;
	}

	/** Get all action buttons */
	TArray<TSharedPtr<class SNeutronButton>> GetActionButtons() const
	{
		TArray<TSharedPtr<class SNeutronButton>> Result = CurrentNavigationButtons;
		Result.Append(AdditionalActionButtons);
		return Result;
	}

	/** Trigger actions bounds to this key */
	FReply HandleKeyPress(FKey Key);

	/** Get the next button to focus in a specific direction */
	static TSharedPtr<class SNeutronButton> GetNextButton(TSharedRef<class SWidget> Widget, TSharedPtr<const class SWidget> Current,
		TArray<TSharedPtr<class SNeutronButton>> Candidates, EUINavigation Direction);

	/** Get the next button to focus in a specific direction */
	static TSharedPtr<class SNeutronButton> GetNextButtonInternal(TSharedRef<class SWidget> Widget, TSharedPtr<const class SWidget> Current,
		TArray<TSharedPtr<class SNeutronButton>> Candidates, EUINavigation Direction);

	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

protected:

	// Settings
	float AnalogNavThreshold;
	float AnalogNavMinPeriod;
	float AnalogNavMaxPeriod;

	// General data
	TWeakObjectPtr<class UNeutronMenuManager> MenuManager;
	TSharedPtr<SBox>                          MainContainer;
	TSharedPtr<class SOverlay>                MainOverlay;

	// Mouse input data
	bool                            MousePressed;
	bool                            MouseWasPressed;
	TNeutronTimedAverage<FVector2D> SmoothedMouseLocation;

	// Input data
	TMultiMap<FName, FName> ActionBindings;
	TMultiMap<FName, FName> AxisBindings;
	FVector2D               CurrentAnalogInput;

	// Focus data
	class SNeutronNavigationPanel*           CurrentNavigationPanel;
	class SNeutronNavigationPanel*           PreviousNavigationPanel;
	TArray<TSharedPtr<class SNeutronButton>> CurrentNavigationButtons;
	TArray<TSharedPtr<class SNeutronButton>> AdditionalActionButtons;
	EUINavigation                            CurrentAnalogNavigation;
	float                                    CurrentAnalogNavigationTime;
};
