// Neutron - Gwennaël Arbona

#pragma once

#include "NeutronMenu.h"
#include "NeutronButton.h"
#include "NeutronNavigationPanel.h"
#include "Neutron/UI/NeutronUI.h"
#include "Neutron/Neutron.h"

#include "Widgets/SCompoundWidget.h"

/** Templatized list class to display elements from a TArray */
template <typename ItemType>
class SNeutronListView : public SCompoundWidget
{
	DECLARE_DELEGATE_RetVal_OneParam(TSharedRef<SWidget>, FNeutronOnGenerateItem, ItemType);
	DECLARE_DELEGATE_RetVal_OneParam(FText, FNeutronOnGenerateTooltip, ItemType);
	DECLARE_DELEGATE_TwoParams(FNeutronListSelectionChanged, ItemType, int32);

	/*----------------------------------------------------
	    Slate arguments
	----------------------------------------------------*/

	SLATE_BEGIN_ARGS(SNeutronListView<ItemType>) : _Horizontal(false), _ButtonTheme("DefaultButton"), _ButtonSize("DoubleButtonSize")
	{}

	SLATE_ARGUMENT(SNeutronNavigationPanel*, Panel)
	SLATE_ARGUMENT(const TArray<ItemType>*, ItemsSource)

	SLATE_EVENT(FNeutronOnGenerateItem, OnGenerateItem)
	SLATE_EVENT(FNeutronOnGenerateTooltip, OnGenerateTooltip)
	SLATE_EVENT(FNeutronListSelectionChanged, OnSelectionChanged)
	SLATE_EVENT(FSimpleDelegate, OnSelectionDoubleClicked)

	SLATE_ARGUMENT(bool, Horizontal)
	SLATE_ARGUMENT(FName, ButtonTheme)
	SLATE_ARGUMENT(FName, ButtonSize)

	SLATE_END_ARGS()

	/*----------------------------------------------------
	    Constructor
	----------------------------------------------------*/

public:

	void Construct(const FArguments& InArgs)
	{
		const FNeutronMainTheme& Theme = FNeutronStyleSet::GetMainTheme();

		// Setup
		Panel                    = InArgs._Panel;
		ItemsSource              = InArgs._ItemsSource;
		OnGenerateItem           = InArgs._OnGenerateItem;
		OnGenerateTooltip        = InArgs._OnGenerateTooltip;
		OnSelectionChanged       = InArgs._OnSelectionChanged;
		OnSelectionDoubleClicked = InArgs._OnSelectionDoubleClicked;
		ButtonTheme              = InArgs._ButtonTheme;
		ButtonSize               = InArgs._ButtonSize;

		// Sanity checks
		NCHECK(Panel);
		NCHECK(ItemsSource);

		// clang-format off
		ChildSlot
		.VAlign(VAlign_Fill)
		.HAlign(HAlign_Center)
		[
			SAssignNew(Container, SScrollBox)
			.AnimateWheelScrolling(true)
			.Style(&Theme.ScrollBoxStyle)
			.ScrollBarVisibility(EVisibility::Collapsed)
			.Orientation(InArgs._Horizontal ? Orient_Horizontal : Orient_Vertical)
			.ConsumeMouseWheel(EConsumeMouseWheel::Always)
		];
		// clang-format on

		// Initialize
		CurrentSelectedIndex   = 0;
		InitiallySelectedIndex = 0;
	}

	/*----------------------------------------------------
	    Public methods
	----------------------------------------------------*/

public:

	/** Refresh the list based on the items source */
	void Refresh(int32 SelectedIndex = INDEX_NONE)
	{
		SelectedIndex          = FMath::Min(SelectedIndex, ItemsSource->Num() - 1);
		InitiallySelectedIndex = SelectedIndex;

		// Get the state of the focused button
		int32               PreviousSelectedIndex       = INDEX_NONE;
		int32               FocusButtonIndex            = 0;
		FNeutronButtonState PreviousSelectedButtonState = FNeutronButtonState();
		for (TSharedPtr<SNeutronButton> Button : ListButtons)
		{
			if (Button->IsFocused())
			{
				PreviousSelectedButtonState = Button->GetState();
				PreviousSelectedIndex       = FocusButtonIndex;
				break;
			}

			FocusButtonIndex++;
		}

		// Clear UI
		for (TSharedPtr<SNeutronButton> Button : ListButtons)
		{
			Panel->DestroyFocusableButton(Button);
		}
		Container->ClearChildren();
		ListButtons.Empty();

		// Build new UI
		int32 BuildIndex = 0;
		for (ItemType Item : *ItemsSource)
		{
			// Add buttons
			TSharedPtr<SNeutronButton> Button;
			// clang-format off
			Container->AddSlot()
			[
				Panel->SNeutronAssignNew(Button, SNeutronButton)
				.Theme(ButtonTheme)
				.Size(ButtonSize)
				.OnFocused(this, &SNeutronListView<ItemType>::OnElementSelected, Item, BuildIndex)
				.OnClicked(this, &SNeutronListView<ItemType>::OnElementSelected, Item, BuildIndex)
				.OnDoubleClicked(FSimpleDelegate::CreateLambda([=]()
				{
					OnSelectionDoubleClicked.ExecuteIfBound();
				}))
			];
			// clang-format on

			// Fill tooltip
			if (OnGenerateTooltip.IsBound())
			{
				Button->SetHelpText(OnGenerateTooltip.Execute(Item));
			}

			// Fill contents
			BuildIndex++;
			if (OnGenerateItem.IsBound())
			{
				Button->GetContainer()->SetContent(OnGenerateItem.Execute(Item));
			}

			ListButtons.Add(Button);
		}

		// Refresh navigation
		Panel->GetMenu()->RefreshNavigationPanel();

		if (SelectedIndex != INDEX_NONE)
		{
			CurrentSelectedIndex = SelectedIndex;
			Panel->GetMenu()->SetFocusedButton(ListButtons[CurrentSelectedIndex], true);
		}
		else if (PreviousSelectedIndex != INDEX_NONE)
		{
			CurrentSelectedIndex = FMath::Min(CurrentSelectedIndex, ItemsSource->Num() - 1);

			PreviousSelectedIndex = FMath::Clamp(PreviousSelectedIndex, 0, BuildIndex - 1);
			Panel->GetMenu()->SetFocusedButton(ListButtons[PreviousSelectedIndex], true);
			ListButtons[PreviousSelectedIndex]->GetState() = PreviousSelectedButtonState;
		}
	}

	/** Force the initially selected index */
	void SetInitiallySelectedIndex(int32 Index)
	{
		InitiallySelectedIndex = Index;
	}

	/** Check if an item is currently selected */
	bool IsCurrentlySelected(const ItemType& Item) const
	{
		return Item == GetSelectedItem();
	}

	/** Get the selection icon */
	const FSlateBrush* GetSelectionIcon(const ItemType& Item) const
	{
		const FNeutronButtonTheme& Theme = FNeutronStyleSet::GetButtonTheme(ButtonTheme);

		bool IsInitialItem = InitiallySelectedIndex >= 0 && ItemsSource && InitiallySelectedIndex < ItemsSource->Num() &&
		                     Item == (*ItemsSource)[InitiallySelectedIndex];

		return IsInitialItem ? &Theme.ListOn : &Theme.ListOff;
	}

	/** Get the currently selected index */
	int32 GetSelectedIndex() const
	{
		return CurrentSelectedIndex;
	}

	/** Get the currently selected item */
	const ItemType& GetSelectedItem() const
	{
		NCHECK(CurrentSelectedIndex >= 0 && CurrentSelectedIndex < (*ItemsSource).Num());
		return (*ItemsSource)[CurrentSelectedIndex];
	}

	/*----------------------------------------------------
	    Callbacks
	----------------------------------------------------*/

protected:

	/** New list item was selected */
	void OnElementSelected(ItemType Selected, int32 Index)
	{
		CurrentSelectedIndex = Index;

		OnSelectionChanged.ExecuteIfBound(Selected, Index);
		Container->ScrollDescendantIntoView(ListButtons[Index], true, EDescendantScrollDestination::IntoView);
	}

	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

protected:

	// Settings
	SNeutronNavigationPanel*     Panel;
	const TArray<ItemType>*      ItemsSource;
	FNeutronOnGenerateItem       OnGenerateItem;
	FNeutronOnGenerateTooltip    OnGenerateTooltip;
	FNeutronListSelectionChanged OnSelectionChanged;
	FSimpleDelegate              OnSelectionDoubleClicked;
	FName                        ButtonTheme;
	FName                        ButtonSize;

	// State
	int32 CurrentSelectedIndex;
	int32 InitiallySelectedIndex;

	// Widgets
	TSharedPtr<SScrollBox>             Container;
	TArray<TSharedPtr<SNeutronButton>> ListButtons;
};
