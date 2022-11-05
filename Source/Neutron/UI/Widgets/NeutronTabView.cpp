// Neutron - Gwennaël Arbona

#include "NeutronTabView.h"
#include "NeutronMenu.h"

#include "Neutron/UI/NeutronUI.h"
#include "Neutron/Neutron.h"

#include "Engine/Engine.h"

#include "Widgets/Layout/SBackgroundBlur.h"

/*----------------------------------------------------
    Tab view content widget
----------------------------------------------------*/

SNeutronTabPanel::SNeutronTabPanel() : SNeutronNavigationPanel(), Blurred(false), TabIndex(0), CurrentVisible(false), CurrentAlpha(0)
{}

void SNeutronTabPanel::Tick(const FGeometry& AllottedGeometry, const double CurrentTime, const float DeltaTime)
{
	SNeutronNavigationPanel::Tick(AllottedGeometry, CurrentTime, DeltaTime);

	// Update opacity value
	if (TabIndex != ParentTabView->GetDesiredTabIndex())
	{
		CurrentAlpha -= (DeltaTime / ENeutronUIConstants::FadeDurationShort);
	}
	else if (TabIndex == ParentTabView->GetCurrentTabIndex())
	{
		CurrentAlpha += (DeltaTime / ENeutronUIConstants::FadeDurationShort);
	}
	CurrentAlpha = FMath::Clamp(CurrentAlpha, 0.0f, 1.0f);

	// Hide or show the menu based on current alpha
	if (!CurrentVisible && CurrentAlpha > 0)
	{
		Show();
	}
	else if (CurrentVisible && CurrentAlpha == 0)
	{
		Hide();
	}
}

void SNeutronTabPanel::OnFocusChanged(TSharedPtr<class SNeutronButton> FocusButton)
{
	if (MainContainer.IsValid())
	{
		if (IsButtonInsideWidget(FocusButton, MainContainer))
		{
			MainContainer->ScrollDescendantIntoView(FocusButton, true, EDescendantScrollDestination::Center);
		}
	}
}

void SNeutronTabPanel::Initialize(int32 Index, bool IsBlurred, const FSlateBrush* OptionalBackground, SNeutronTabView* Parent)
{
	Blurred       = IsBlurred;
	TabIndex      = Index;
	Background    = OptionalBackground;
	ParentTabView = Parent;
}

void SNeutronTabPanel::Show()
{
	CurrentVisible = true;

	NCHECK(Menu);
	Menu->SetNavigationPanel(this);
}

void SNeutronTabPanel::Hide()
{
	CurrentVisible = false;

	NCHECK(Menu);
	Menu->ClearNavigationPanel();
}

bool SNeutronTabPanel::IsHidden() const
{
	return (CurrentAlpha == 0);
}

TOptional<int32> SNeutronTabPanel::GetBlurRadius() const
{
	const FNeutronMainTheme& Theme = FNeutronStyleSet::GetMainTheme();

	float CorrectedAlpha =
		FMath::Clamp((CurrentAlpha - ENeutronUIConstants::BlurAlphaOffset) / (1.0f - ENeutronUIConstants::BlurAlphaOffset), 0.0f, 1.0f);
	float BlurAlpha = FMath::InterpEaseInOut(0.0f, 1.0f, CorrectedAlpha, ENeutronUIConstants::EaseStandard);

	return static_cast<int32>(BlurAlpha * Theme.BlurRadius);
}

float SNeutronTabPanel::GetBlurStrength() const
{
	const FNeutronMainTheme& Theme = FNeutronStyleSet::GetMainTheme();

	float CorrectedAlpha =
		FMath::Clamp((CurrentAlpha - ENeutronUIConstants::BlurAlphaOffset) / (1.0f - ENeutronUIConstants::BlurAlphaOffset), 0.0f, 1.0f);
	float BlurAlpha = FMath::InterpEaseInOut(0.0f, 1.0f, CorrectedAlpha, ENeutronUIConstants::EaseStandard);

	return BlurAlpha * Theme.BlurStrength;
}

/*----------------------------------------------------
    Construct
----------------------------------------------------*/

SNeutronTabView::SNeutronTabView() : DesiredTabIndex(0), CurrentTabIndex(0), CurrentBlurAlpha(0)
{}

void SNeutronTabView::Construct(const FArguments& InArgs)
{
	// Data
	const FNeutronMainTheme&   Theme       = FNeutronStyleSet::GetMainTheme();
	const FNeutronButtonTheme& ButtonTheme = FNeutronStyleSet::GetButtonTheme();

	// clang-format off
	ChildSlot
	[
		SNew(SOverlay)

		// Fullscreen blur
		+ SOverlay::Slot()
		[
			SNew(SBox)
			.Padding(this, &SNeutronTabView::GetBlurPadding)
			[
				SNew(SBackgroundBlur)
				.BlurRadius(this, &SNeutronTabView::GetBlurRadius)
				.BlurStrength(this, &SNeutronTabView::GetBlurStrength)
				.Padding(0)
			]
		]

		// Optional background overlay
		+ SOverlay::Slot()
		[
			SNew(SBorder)
			.BorderImage(this, &SNeutronTabView::GetGlobalBackground)
			.BorderBackgroundColor(this, &SNeutronTabView::GetGlobalBackgroundColor)
		]

		+ SOverlay::Slot()
		[
			SNew(SVerticalBox)

			// Header
			+ SVerticalBox::Slot()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Top)
			.AutoHeight()
			.Padding(0)
			[
				SNew(SBackgroundBlur)
				.BlurRadius(this, &SNeutronTabView::GetHeaderBlurRadius)
				.BlurStrength(this, &SNeutronTabView::GetHeaderBlurStrength)
				.Padding(0)
				[
					SNew(SOverlay)

					+ SOverlay::Slot()
					[
						SNew(SVerticalBox)

						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							InArgs._BackgroundWidget.Widget
						]

						+ SVerticalBox::Slot()
					]
					
					+ SOverlay::Slot()
					[
						SAssignNew(HeaderContainer, SBorder)
						.BorderImage(&Theme.MainMenuBackground)
						.Padding(0)
						.Visibility(EVisibility::SelfHitTestInvisible)
						[
							SNew(SVerticalBox)
		
							// Header buttons
							+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(Theme.ContentPadding)
							[
								SNew(SHorizontalBox)
				
								+ SHorizontalBox::Slot()
								.AutoWidth()
								[
									InArgs._LeftNavigation.Widget
								]

								+ SHorizontalBox::Slot()
								.AutoWidth()
								[
									SAssignNew(Header, SHorizontalBox)
								]
				
								+ SHorizontalBox::Slot()
								.AutoWidth()
								[
									InArgs._RightNavigation.Widget
								]

								+ SHorizontalBox::Slot()
				
								+ SHorizontalBox::Slot()
								.AutoWidth()
								[
									InArgs._End.Widget
								]
							]

							// User-supplied header widget
							+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(0)
							.HAlign(HAlign_Fill)
							[
								InArgs._Header.Widget
							]
						]
					]
				]
			]

			// Main view
			+ SVerticalBox::Slot()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
			.Padding(0)
			[
				SNew(SBorder)
				.BorderImage(&Theme.MainMenuBackground)
				.ColorAndOpacity(this, &SNeutronTabView::GetColor)
				.BorderBackgroundColor(this, &SNeutronTabView::GetBackgroundColor)
				.Padding(0)
				[
					SAssignNew(Content, SWidgetSwitcher)
					.WidgetIndex(this, &SNeutronTabView::GetCurrentTabIndex)
				]
			]
		]
	];

	// Slot contents
	int32 Index = 0;
	for (FSlot::FSlotArguments& Arg : const_cast<TArray<FSlot::FSlotArguments>&>(InArgs._Slots))
	{
		// Add header entry
		Header->AddSlot()
		.AutoWidth()
		[
			InArgs._Menu->SNeutronNew(SNeutronButton)
			.Icon(Arg._Icon)
			.Theme("TabButton")
			.Size("TabButtonSize")
			.Action(Arg._Action)
			.Text(Arg._Header)
			.HelpText(Arg._HeaderHelp)
			.OnClicked(this, &SNeutronTabView::SetTabIndex, Index)
			.Visibility(this, &SNeutronTabView::GetTabVisibility, Index)
			.Enabled(this, &SNeutronTabView::IsTabEnabled, Index)
			.Focusable(false)
		];
		

		// Add content
		SNeutronTabPanel* TabPanel = static_cast<SNeutronTabPanel*>(Arg.GetAttachedWidget().Get());
		TabPanel->Initialize(Index,
			Arg._Blur.IsSet() ? Arg._Blur.Get() : false,
			Arg._Background.IsSet() ? Arg._Background.Get() : nullptr, 
			this);

		Content->AddSlot()
		[
			Arg.GetAttachedWidget().ToSharedRef()
		];

		Panels.Add(TabPanel);
		PanelVisibility.Add(Arg._Visible);

		Index++;
	}

	// clang-format on
	SetVisibility(EVisibility::SelfHitTestInvisible);
}

/*----------------------------------------------------
    Interaction
----------------------------------------------------*/

void SNeutronTabView::Tick(const FGeometry& AllottedGeometry, const double CurrentTime, const float DeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, CurrentTime, DeltaTime);

	// If we lost the current tab, find another one
	if (!IsTabVisible(CurrentTabIndex) && !IsTabVisible(DesiredTabIndex))
	{
		for (int32 Index = 0; Index < Panels.Num(); Index++)
		{
			int32 RelativeIndex = (Index / 2 + 1) * (Index % 2 != 0 ? 1 : -1);
			RelativeIndex       = CurrentTabIndex + (RelativeIndex % Panels.Num());

			if (RelativeIndex >= 0 && IsTabVisible(RelativeIndex))
			{
				NLOG("SNeutronTabView::Tick : tab %d isn't visible, fallback to %d", CurrentTabIndex, RelativeIndex);
				SetTabIndex(RelativeIndex);
				break;
			}
		}
	}

	// Process widget change
	if (CurrentTabIndex != DesiredTabIndex)
	{
		if (GetCurrentTabContent()->IsHidden())
		{
			CurrentTabIndex = DesiredTabIndex;
		}
	}

	// Process blur
	CurrentBlurAlpha += (GetCurrentTabContent()->IsBlurred() ? 1 : -1) * (DeltaTime / ENeutronUIConstants::FadeDurationMinimal);
	CurrentBlurAlpha = FMath::Clamp(CurrentBlurAlpha, 0.0f, 1.0f);
}

void SNeutronTabView::ShowPreviousTab()
{
	for (int32 Index = CurrentTabIndex - 1; Index >= 0; Index--)
	{
		if (IsTabVisible(Index))
		{
			SetTabIndex(Index);
			break;
		}
	}
}

void SNeutronTabView::ShowNextTab()
{
	for (int32 Index = CurrentTabIndex + 1; Index < Panels.Num(); Index++)
	{
		if (IsTabVisible(Index))
		{
			SetTabIndex(Index);
			break;
		}
	}
}

void SNeutronTabView::SetTabIndex(int32 Index)
{
	NLOG("SNeutronTabView::SetTabIndex : %d, was %d", Index, CurrentTabIndex);

	if (Index >= 0 && Index < Panels.Num() && Index != CurrentTabIndex)
	{
		DesiredTabIndex = Index;
	}
}

int32 SNeutronTabView::GetCurrentTabIndex() const
{
	return CurrentTabIndex;
}

int32 SNeutronTabView::GetDesiredTabIndex() const
{
	return DesiredTabIndex;
}

bool SNeutronTabView::IsTabVisible(int32 Index) const
{
	NCHECK(Index >= 0 && Index < PanelVisibility.Num());

	if (PanelVisibility[Index].IsBound() || PanelVisibility[Index].IsSet())
	{
		return PanelVisibility[Index].Get();
	}

	return true;
}

TSharedRef<SNeutronTabPanel> SNeutronTabView::GetCurrentTabContent() const
{
	return SharedThis(Panels[CurrentTabIndex]);
}

/*----------------------------------------------------
    Callbacks
----------------------------------------------------*/

EVisibility SNeutronTabView::GetTabVisibility(int32 Index) const
{
	return IsTabVisible(Index) ? EVisibility::Visible : EVisibility::Collapsed;
}

bool SNeutronTabView::IsTabEnabled(int32 Index) const
{
	return DesiredTabIndex != Index;
}

FLinearColor SNeutronTabView::GetColor() const
{
	return FLinearColor(1.0f, 1.0f, 1.0f, GetCurrentTabContent()->GetCurrentAlpha());
}

FSlateColor SNeutronTabView::GetBackgroundColor() const
{
	return FLinearColor(1.0f, 1.0f, 1.0f, CurrentBlurAlpha);
}

FSlateColor SNeutronTabView::GetGlobalBackgroundColor() const
{
	return FLinearColor(1.0f, 1.0f, 1.0f, GetCurrentTabContent()->GetCurrentAlpha());
}

const FSlateBrush* SNeutronTabView::GetGlobalBackground() const
{
	return GetCurrentTabContent()->GetBackground();
}

bool SNeutronTabView::IsBlurSplit() const
{
	return !GetCurrentTabContent()->IsBlurred() || CurrentBlurAlpha < 1.0f;
}

FMargin SNeutronTabView::GetBlurPadding() const
{
	float TopPadding = IsBlurSplit() ? HeaderContainer->GetCachedGeometry().Size.Y : 0;
	return FMargin(0, TopPadding, 0, 0);
}

TOptional<int32> SNeutronTabView::GetBlurRadius() const
{
	const FNeutronMainTheme& Theme = FNeutronStyleSet::GetMainTheme();

	float Alpha = FMath::InterpEaseInOut(0.0f, 1.0f, CurrentBlurAlpha, ENeutronUIConstants::EaseStandard);
	return static_cast<int32>(Theme.BlurRadius * Alpha);
}

float SNeutronTabView::GetBlurStrength() const
{
	const FNeutronMainTheme& Theme = FNeutronStyleSet::GetMainTheme();

	float Alpha = FMath::InterpEaseInOut(0.0f, 1.0f, CurrentBlurAlpha, ENeutronUIConstants::EaseStandard);
	return Theme.BlurStrength * Alpha;
}

TOptional<int32> SNeutronTabView::GetHeaderBlurRadius() const
{
	const FNeutronMainTheme& Theme = FNeutronStyleSet::GetMainTheme();

	return static_cast<int32>(IsBlurSplit() ? Theme.BlurRadius : 0);
}

float SNeutronTabView::GetHeaderBlurStrength() const
{
	const FNeutronMainTheme& Theme = FNeutronStyleSet::GetMainTheme();

	return IsBlurSplit() ? Theme.BlurStrength : 0;
}
