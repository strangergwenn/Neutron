#pragma once

#include "NeutronButton.h"

#include "Neutron/UI/NeutronUI.h"
#include "Neutron/Neutron.h"

#include "Widgets/SCompoundWidget.h"

/** Inherit this class for focus support, set DefaultNavigationButton for default focus (optional) */
class NEUTRON_API SNeutronNavigationPanel : public SCompoundWidget
{
	/*----------------------------------------------------
	    Slate arguments
	----------------------------------------------------*/

	SLATE_BEGIN_ARGS(SNeutronNavigationPanel) : _Menu(nullptr)
	{}

	SLATE_ARGUMENT(class SNeutronMenu*, Menu)

	SLATE_DEFAULT_SLOT(FArguments, Content)

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/*----------------------------------------------------
	    Interaction
	----------------------------------------------------*/

public:

	/** Create a new focusable button */
	template <typename WidgetType, typename RequiredArgsPayloadType>
	TSlateDecl<WidgetType, RequiredArgsPayloadType> NewNeutronButton(
		const ANSICHAR* InType, const ANSICHAR* InFile, int32 OnLine, RequiredArgsPayloadType&& InRequiredArgs, bool DefaultFocus)
	{
		auto Button =
			TSlateDecl<WidgetType, RequiredArgsPayloadType>(InType, InFile, OnLine, Forward<RequiredArgsPayloadType>(InRequiredArgs));

		if ((NavigationButtons.Num() == 0 && Button._Widget->IsEnabled()) || DefaultFocus)
		{
			DefaultNavigationButton = Button._Widget;
		}
		NavigationButtons.Add(Button._Widget);

		return Button;
	}

	/** Release a focusable button */
	void DestroyFocusableButton(TSharedPtr<SNeutronButton> Button)
	{
		NavigationButtons.Remove(Button);
	}

	/** Next item */
	virtual void Next()
	{}

	/** Previous item */
	virtual void Previous()
	{}

	/** Zoom in */
	virtual void ZoomIn()
	{}

	/** Zoom out */
	virtual void ZoomOut()
	{}

	/** Clicked */
	virtual void OnClicked(const FVector2D& Position)
	{}

	/** Clicked */
	virtual void OnDoubleClicked(const FVector2D& Position)
	{}

	/** Pass horizontal right-stick or mouse drag input to this widget */
	virtual void HorizontalAnalogInput(float Value)
	{}

	/** Pass vertical right-stick or mouse drag input to this widget */
	virtual void VerticalAnalogInput(float Value)
	{}

	/** Focus changed to this button */
	virtual void OnFocusChanged(TSharedPtr<class SNeutronButton> FocusButton)
	{}

	/** Check whether this panel is modal */
	virtual bool IsModal() const
	{
		return false;
	}

	/** Get the ideal default focus button */
	virtual TSharedPtr<SNeutronButton> GetDefaultFocusButton() const;

	/** Confirm whether a button is allowed to fire its action */
	virtual bool IsButtonActionAllowed(TSharedPtr<class SNeutronButton> Button) const;

	/** Confirm whether OnClicked and OnDoubleClicked will fire when clicked on real menu elements */
	virtual bool IsClickInsideMenuAllowed() const
	{
		return true;
	}

	/** Reset the focus to the default button, or any button */
	void ResetNavigation();

	/** Get all the buttons on this panel that we can navigate to */
	TArray<TSharedPtr<class SNeutronButton>>& GetNavigationButtons();

	/** Check if the focused button is valid and inside a widget of this panel */
	bool IsFocusedButtonInsideWidget(TSharedPtr<SWidget> Widget) const;

	/** Check if a particular button is valid and inside a widget of this panel */
	bool IsButtonInsideWidget(TSharedPtr<class SNeutronButton> Button, TSharedPtr<SWidget> Widget) const;

	/** Get the owner menu */
	class SNeutronMenu* GetMenu() const
	{
		return Menu;
	}

	/** Set the contents */
	void SetContent(const TSharedRef<SWidget>& InContent)
	{
		ChildSlot[InContent];
	}

protected:

	// Navigation state
	class SNeutronMenu*                Menu;
	TSharedPtr<SNeutronButton>         DefaultNavigationButton;
	TArray<TSharedPtr<SNeutronButton>> NavigationButtons;
};
