// Neutron - Gwennaël Arbona

#pragma once

#include "NeutronButton.h"
#include "NeutronListView.h"
#include "NeutronMenu.h"
#include "NeutronModalPanel.h"

#include "Neutron/UI/NeutronUI.h"
#include "Neutron/Neutron.h"

/** Templatized, modal, list class to display elements from a TArray */
template <typename ItemType>
class SNeutronModalListView : public SNeutronButton
{
public:

	typedef TPair<int32, TArray<ItemType>> SelfRefreshType;

	DECLARE_DELEGATE_RetVal(SelfRefreshType, FNeutronOnSelfRefresh);
	DECLARE_DELEGATE_RetVal_OneParam(TSharedRef<SWidget>, FNeutronOnGenerateItem, ItemType);
	DECLARE_DELEGATE_RetVal_TwoParams(bool, FNeutronOnFilterItem, ItemType, TArray<int32>);
	DECLARE_DELEGATE_RetVal_OneParam(FText, FNeutronOnGenerateName, ItemType);
	DECLARE_DELEGATE_RetVal_OneParam(FText, FNeutronOnGenerateTooltip, ItemType);
	DECLARE_DELEGATE_TwoParams(FNeutronListSelectionChanged, ItemType, int32);

private:

	/*----------------------------------------------------
	    Slate arguments
	----------------------------------------------------*/

	SLATE_BEGIN_ARGS(SNeutronModalListView<ItemType>)
		: _Panel(nullptr)
		, _ItemsSource(nullptr)
		, _Enabled(true)
		, _ActionFocusable(true)
		, _Horizontal(false)
		, _ButtonTheme("DefaultButton")
		, _ButtonSize("DefaultButtonSize")
		, _ListButtonTheme("DefaultButton")
		, _ListButtonSize("DoubleButtonSize")
		, _FilterButtonSize("DefaultButtonSize")
	{}

	SLATE_ARGUMENT(SNeutronNavigationPanel*, Panel)
	SLATE_ARGUMENT(const TArray<ItemType>*, ItemsSource)
	SLATE_ATTRIBUTE(FText, TitleText)
	SLATE_ATTRIBUTE(FText, HelpText)
	SLATE_ATTRIBUTE(FName, Action)
	SLATE_ATTRIBUTE(bool, Enabled)
	SLATE_ATTRIBUTE(bool, Focusable)
	SLATE_ARGUMENT(bool, ActionFocusable)
	SLATE_ARGUMENT(TArray<FText>, FilterOptions)

	SLATE_ARGUMENT(FSimpleDelegate, OnDoubleClicked)
	SLATE_ARGUMENT(FNeutronOnSelfRefresh, OnSelfRefresh)
	SLATE_EVENT(FNeutronOnGenerateItem, OnGenerateItem)
	SLATE_EVENT(FNeutronOnFilterItem, OnFilterItem)
	SLATE_EVENT(FNeutronOnGenerateName, OnGenerateName)
	SLATE_EVENT(FNeutronOnGenerateTooltip, OnGenerateTooltip)
	SLATE_EVENT(FNeutronListSelectionChanged, OnSelectionChanged)

	SLATE_ARGUMENT(bool, Horizontal)
	SLATE_ARGUMENT(FName, ButtonTheme)
	SLATE_ARGUMENT(FName, ButtonSize)
	SLATE_ARGUMENT(FName, ListButtonTheme)
	SLATE_ARGUMENT(FName, ListButtonSize)
	SLATE_ARGUMENT(FName, FilterButtonSize)
	SLATE_ARGUMENT(FNeutronButtonUserSizeCallback, UserSizeCallback)
	SLATE_NAMED_SLOT(FArguments, ButtonContent)

	SLATE_END_ARGS()

	/*----------------------------------------------------
	    Constructor
	----------------------------------------------------*/

public:

	void Construct(const FArguments& InArgs)
	{
		TitleText          = InArgs._TitleText;
		HelpText           = InArgs._HelpText;
		OnSelfRefresh      = InArgs._OnSelfRefresh;
		OnGenerateName     = InArgs._OnGenerateName;
		OnSelectionChanged = InArgs._OnSelectionChanged;
		ListPanel          = InArgs._Panel->GetMenu()->CreateModalPanel(FNeutronAsyncCondition::CreateLambda(    //
            [this]()
            {
                return IsConfirmEnabled();
            }));

		SNeutronButton::Construct(
			SNeutronButton::FArguments()
				.Text(InArgs._ButtonContent.Widget == SNullWidget::NullWidget &&
							  (OnGenerateName.IsBound() || TitleText.IsSet() || TitleText.IsBound())
						  ? TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateSP(this, &SNeutronModalListView::GetButtonText))
						  : TAttribute<FText>())
				.HelpText(HelpText)
				.Action(InArgs._Action)
				.ActionFocusable(InArgs._ActionFocusable)
				.Theme(InArgs._ButtonTheme)
				.UserSizeCallback(InArgs._UserSizeCallback)
				.Size(InArgs._ButtonSize)
				.Enabled(InArgs._Enabled)
				.Focusable(InArgs._Focusable)
				.OnClicked(this, &SNeutronModalListView::OnOpenList)
				.OnDoubleClicked(InArgs._OnDoubleClicked)
				.Content()[InArgs._ButtonContent.Widget]);

		SAssignNew(ListView, SNeutronListView<ItemType>)
			.Panel(ListPanel.Get())
			.ItemsSource(InArgs._ItemsSource ? InArgs._ItemsSource : &InternalItemsSource)
			.FilterOptions(InArgs._FilterOptions)
			.OnGenerateItem(InArgs._OnGenerateItem)
			.OnFilterItem(InArgs._OnFilterItem)
			.OnGenerateTooltip(InArgs._OnGenerateTooltip)
			.OnSelectionChanged(this, &SNeutronModalListView::OnListSelectionChanged)
			.OnSelectionDoubleClicked(this, &SNeutronModalListView::OnListConfirmed)
			.ButtonTheme(InArgs._ListButtonTheme)
			.ButtonSize(InArgs._ListButtonSize)
			.FilterButtonSize(InArgs._FilterButtonSize);
	}

	/*----------------------------------------------------
	    Public methods
	----------------------------------------------------*/

public:

	/** Refresh the list based on the items source */
	void Refresh(int32 SelectedIndex = INDEX_NONE)
	{
		ListView->Refresh(SelectedIndex);
		CurrentConfirmed = ListView->GetSelectedItem();
	}

	/** Show the list */
	void Show()
	{
		OnOpenList();
	}

	/** Check if an item is currently selected */
	bool IsCurrentlySelected(const ItemType& Item) const
	{
		return Item == ListView->GetSelectedItem();
	}

	/** Get the selection icon */
	const FSlateBrush* GetSelectionIcon(const ItemType& Item) const
	{
		return ListView->GetSelectionIcon(Item);
	}

	/** Get the currently selected item */
	const ItemType& GetSelectedItem() const
	{
		return ListView->GetSelectedItem();
	}

	/*----------------------------------------------------
	    Callbacks
	----------------------------------------------------*/

protected:

	FText GetButtonText() const
	{
		if (OnGenerateName.IsBound())
		{
			return OnGenerateName.Execute(CurrentConfirmed);
		}
		else
		{
			return TitleText.Get();
		}
	}

	void OnOpenList()
	{
		CurrentUserSelectedIndex = INDEX_NONE;
		int32 CurrentListIndex   = ListView->GetSelectedIndex();

		if (OnSelfRefresh.IsBound())
		{
			auto RefreshResult = OnSelfRefresh.Execute();

			if (RefreshResult.Key != INDEX_NONE)
			{
				CurrentListIndex = RefreshResult.Key;
			}
			InternalItemsSource = RefreshResult.Value;
		}

		ListPanel->Show(HelpText.Get(), FText(), FSimpleDelegate::CreateSP(this, &SNeutronModalListView::OnListConfirmed),
			FSimpleDelegate(), FSimpleDelegate(), ListView);

		ListView->Refresh(CurrentListIndex);
	}

	void OnListConfirmed()
	{
		if (CurrentUserSelectedIndex != INDEX_NONE)
		{
			CurrentConfirmed = CurrentSelected;
			OnSelectionChanged.ExecuteIfBound(CurrentSelected, CurrentUserSelectedIndex);
		}

		ListPanel->Hide();
	}

	void OnListSelectionChanged(ItemType Selected, int32 Index)
	{
		CurrentSelected          = Selected;
		CurrentUserSelectedIndex = Index;
	}

	bool IsConfirmEnabled() const
	{
		for (const TSharedPtr<SNeutronButton>& Button : ListView->GetFilterButtons())
		{
			if (Button->IsFocused())
			{
				return false;
			}
		}

		return true;
	}

	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

protected:

	// State
	TAttribute<FText>            TitleText;
	TAttribute<FText>            HelpText;
	ItemType                     CurrentSelected;
	ItemType                     CurrentConfirmed;
	int32                        CurrentUserSelectedIndex;
	FSimpleDelegate              OnOpen;
	FNeutronOnSelfRefresh        OnSelfRefresh;
	FNeutronOnGenerateName       OnGenerateName;
	FNeutronListSelectionChanged OnSelectionChanged;

	// Internal list
	TArray<ItemType> InternalItemsSource;

	// Widgets
	TSharedPtr<SNeutronModalPanel>         ListPanel;
	TSharedPtr<SNeutronListView<ItemType>> ListView;
};
