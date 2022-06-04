// Neutron - Gwennaël Arbona

#include "NeutronModalPanel.h"
#include "NeutronMenu.h"
#include "NeutronButton.h"

#include "Neutron/UI/NeutronUI.h"

#include "Widgets/Layout/SBackgroundBlur.h"

#define LOCTEXT_NAMESPACE "SNeutronModalPanel"

/*----------------------------------------------------
    Construct
----------------------------------------------------*/

void SNeutronModalPanel::Construct(const FArguments& InArgs)
{
	// Setup
	const FNeutronMainTheme&   Theme       = FNeutronStyleSet::GetMainTheme();
	const FNeutronButtonTheme& ButtonTheme = FNeutronStyleSet::GetButtonTheme();

	// Parent constructor
	SNeutronNavigationPanel::Construct(SNeutronNavigationPanel::FArguments().Menu(InArgs._Menu));

	// clang-format off
	ChildSlot
	[
		SNew(SVerticalBox)

		+ SVerticalBox::Slot()

		+ SVerticalBox::Slot()
		.AutoHeight()
		.VAlign(VAlign_Center)
		[
			SNew(SBorder)
			.BorderImage(&ButtonTheme.Border)
			.ColorAndOpacity(this, &SNeutronModalPanel::GetColor)
			.BorderBackgroundColor(this, &SNeutronModalPanel::GetBackgroundColor)
			.Padding(FMargin(0, 1))
			.VAlign(VAlign_Center)
			[
				SNew(SBackgroundBlur)
				.BlurRadius(Theme.BlurRadius)
				.BlurStrength(Theme.BlurStrength)
				.bApplyAlphaToBlur(true)
				.Padding(0)
				[
					SNew(SBorder)
					.Padding(0)
					.BorderImage(&Theme.MainMenuGenericBackground)
					.ColorAndOpacity(this, &SNeutronModalPanel::GetColor)
					.BorderBackgroundColor(this, &SNeutronModalPanel::GetBackgroundColor)
					[
						SNew(SVerticalBox)

						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(SBorder)
							.Padding(0)
							.BorderImage(&Theme.MainMenuGenericBorder)
							.Padding(Theme.ContentPadding)
							.HAlign(HAlign_Center)
							[
								SAssignNew(TitleText, STextBlock)
								.TextStyle(&Theme.HeadingFont)
							]
						]

						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(Theme.ContentPadding)
						[
							SNew(SVerticalBox)

							+ SVerticalBox::Slot()
							.AutoHeight()
							.HAlign(HAlign_Center)
							[
								SAssignNew(InformationText, STextBlock)
								.TextStyle(&Theme.InfoFont)
								.WrapTextAt(750)
							]

							+ SVerticalBox::Slot()
							[
								SAssignNew(ContentBox, SBox)
								.MaxDesiredHeight(750)
							]
						]

						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(SBorder)
							.Padding(0)
							.BorderImage(&Theme.MainMenuGenericBorder)
							.Padding(Theme.ContentPadding)
							.HAlign(HAlign_Center)
							[
								SNew(SHorizontalBox)

								+ SHorizontalBox::Slot()
								[
									SNeutronAssignNew(ConfirmButton, SNeutronButton)
									.Text(FText())
									.Action(FNeutronPlayerInput::MenuConfirm)
									.ActionFocusable(false)
									.OnClicked(this, &SNeutronModalPanel::OnConfirmPanel)
									.Visibility(this, &SNeutronModalPanel::GetConfirmVisibility)
									.Enabled(this, &SNeutronModalPanel::IsConfirmEnabled)
								]

								+ SHorizontalBox::Slot()
								[
									SNeutronAssignNew(DismissButton, SNeutronButton)
									.Text(FText())
									.Action(FNeutronPlayerInput::MenuSecondary)
									.ActionFocusable(false)
									.OnClicked(this, &SNeutronModalPanel::OnDismissPanel)
									.Visibility(this, &SNeutronModalPanel::GetDismissVisibility)
								]

								+ SHorizontalBox::Slot()
								[
									SNeutronAssignNew(CancelButton, SNeutronButton)
									.Text(FText())
									.Action(FNeutronPlayerInput::MenuCancel)
									.ActionFocusable(false)
									.OnClicked(this, &SNeutronModalPanel::OnCancelPanel)
								]
							]
						]
					]
				]
			]
		]

		+ SVerticalBox::Slot()
	];
	// clang-format on

	SAssignNew(InternalWidget, SBox);

	// Defaults
	FadeDuration = ENeutronUIConstants::FadeDurationShort;

	// Initialization
	ShouldShow      = false;
	CurrentAlpha    = 0;
	CurrentFadeTime = 0;
	SetVisibility(EVisibility::HitTestInvisible);
}

/*----------------------------------------------------
    Interaction
----------------------------------------------------*/

void SNeutronModalPanel::Tick(const FGeometry& AllottedGeometry, const double CurrentTime, const float DeltaTime)
{
	SNeutronNavigationPanel::Tick(AllottedGeometry, CurrentTime, DeltaTime);

	// Update alpha
	if (ShouldShow)
	{
		CurrentFadeTime += DeltaTime;
	}
	else
	{
		CurrentFadeTime -= DeltaTime;
	}
	CurrentFadeTime = FMath::Clamp(CurrentFadeTime, 0.0f, FadeDuration);
	CurrentAlpha    = FMath::InterpEaseInOut(0.0f, 1.0f, CurrentFadeTime / FadeDuration, ENeutronUIConstants::EaseStandard);

	// Update visibility
	if (CurrentAlpha > 0)
	{
		SetVisibility(EVisibility::Visible);
	}
	else
	{
		SetVisibility(EVisibility::HitTestInvisible);
	}
}

void SNeutronModalPanel::Show(FText Title, FText Text, FSimpleDelegate NewOnConfirmed, FSimpleDelegate NewOnDismissed,
	FSimpleDelegate NewOnCancelled, TSharedPtr<SWidget> Content)
{
	// NLOG("SNeutronModalPanel::Show");

	ShouldShow  = true;
	OnConfirmed = NewOnConfirmed;
	OnDismissed = NewOnDismissed;
	OnCancelled = NewOnCancelled;

	UpdateButtons();
	TitleText->SetText(Title);
	InformationText->SetText(Text);
	InformationText->SetVisibility(Text.IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible);

	if (Content.IsValid())
	{
		ContentBox->SetContent(Content.ToSharedRef());
	}
	else
	{
		ContentBox->SetContent(InternalWidget.ToSharedRef());
	}

	if (Menu)
	{
		ResetNavigation();
		Menu->SetModalNavigationPanel(this);
	}
}

void SNeutronModalPanel::Hide()
{
	// NLOG("SNeutronModalPanel::Hide");

	ShouldShow = false;

	if (Menu && Menu->IsActiveNavigationPanel(this))
	{
		Menu->ClearModalNavigationPanel();
	}
}

void SNeutronModalPanel::UpdateButtons()
{
	ConfirmButton->SetText(LOCTEXT("Confirm", "Confirm"));
	ConfirmButton->SetHelpText(LOCTEXT("ConfirmHelp", "Confirm the current action"));
	DismissButton->SetText(LOCTEXT("Ignore", "Ignore"));
	DismissButton->SetHelpText(LOCTEXT("IgnoreHelp", "Ignore and close this message"));
	CancelButton->SetText(LOCTEXT("Cancel", "Cancel"));
	CancelButton->SetHelpText(LOCTEXT("CancelHelp", "Cancel the current action"));
}

/*----------------------------------------------------
    Content callbacks
----------------------------------------------------*/

EVisibility SNeutronModalPanel::GetConfirmVisibility() const
{
	return OnConfirmed.IsBound() ? EVisibility::Visible : EVisibility::Collapsed;
}

bool SNeutronModalPanel::IsConfirmEnabled() const
{
	return true;
}

EVisibility SNeutronModalPanel::GetDismissVisibility() const
{
	return OnDismissed.IsBound() ? EVisibility::Visible : EVisibility::Collapsed;
}

FLinearColor SNeutronModalPanel::GetColor() const
{
	// Hack for proper smoothing of background blur
	return FLinearColor(1.0f, 1.0f, 1.0f, CurrentAlpha > 0.1f ? CurrentAlpha : 0.0f);
}

FSlateColor SNeutronModalPanel::GetBackgroundColor() const
{
	return GetColor();
}

/*----------------------------------------------------
    Callbacks
----------------------------------------------------*/

void SNeutronModalPanel::OnConfirmPanel()
{
	NLOG("SNeutronModalPanel::OnConfirmPanel");

	Hide();

	const FNeutronMainTheme& Theme = FNeutronStyleSet::GetMainTheme();
	FSlateApplication::Get().PlaySound(Theme.ConfirmSound);

	OnConfirmed.ExecuteIfBound();
}

void SNeutronModalPanel::OnDismissPanel()
{
	NLOG("SNeutronModalPanel::OnDismissPanel");

	Hide();

	const FNeutronMainTheme& Theme = FNeutronStyleSet::GetMainTheme();
	FSlateApplication::Get().PlaySound(Theme.CancelSound);

	OnDismissed.ExecuteIfBound();
}

void SNeutronModalPanel::OnCancelPanel()
{
	NLOG("SNeutronModalPanel::OnCancelPanel");

	Hide();

	const FNeutronMainTheme& Theme = FNeutronStyleSet::GetMainTheme();
	FSlateApplication::Get().PlaySound(Theme.CancelSound);

	OnCancelled.ExecuteIfBound();
}

#undef LOCTEXT_NAMESPACE
