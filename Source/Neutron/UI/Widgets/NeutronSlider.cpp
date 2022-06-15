// Neutron - Gwennaël Arbona

#include "NeutronSlider.h"
#include "NeutronButton.h"
#include "NeutronKeyLabel.h"

#include "Neutron/System/NeutronMenuManager.h"
#include "Neutron/Neutron.h"

#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBorder.h"

/*----------------------------------------------------
    Construct
----------------------------------------------------*/

void SNeutronSlider::Construct(const FArguments& InArgs)
{
	// Arguments
	Action       = InArgs._Action;
	ValueStep    = InArgs._ValueStep;
	Analog       = InArgs._Analog;
	CurrentValue = InArgs._Value;
	ValueChanged = InArgs._OnValueChanged;

	// Setup
	const FNeutronButtonTheme& ButtonTheme = FNeutronStyleSet::GetButtonTheme();
	const FNeutronMainTheme&   Theme       = FNeutronStyleSet::GetMainTheme();
	const FNeutronButtonSize&  Size        = FNeutronStyleSet::GetButtonSize(InArgs._Size);
	SliderSpeed                            = 2.0f;
	SliderAnalogSpeed                      = 0.01f;

	// Border padding
	FMargin HeaderBorderPadding = FMargin(Theme.SliderStyle.NormalThumbImage.GetImageSize().X + ButtonTheme.HoverAnimationPadding.Left, 0);

	// clang-format off
	SNeutronButton::Construct(SNeutronButton::FArguments()
		.HelpText(InArgs._HelpText)
		.Action(InArgs._Action)
		.Size(InArgs._Size)
		.Theme(InArgs._Theme)
		.Enabled(InArgs._Enabled)
		.ActionFocusable(true)
		.Header()
		[
			SNew(SBox)
			.Padding(HeaderBorderPadding)
			[
				InArgs._Header.Widget
			]
		]
		.Footer()
		[
			SNew(SBox)
			.Padding(HeaderBorderPadding)
			[
				InArgs._Footer.Widget
			]
		]);

	// Add the internal layout
	TSharedPtr<SHorizontalBox> Box;
	InnerContainer->SetContent(SAssignNew(Box, SHorizontalBox)

		// Action binding
		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SBox)
			.Padding(ButtonTheme.IconPadding)
			.Visibility(this, &SNeutronSlider::GetActionVisibility)
			[
				SNew(SNeutronKeyLabel)
				.Key(this, &SNeutronButton::GetActionKey)
			]
		]
	
		// Slider
		+ SHorizontalBox::Slot()
		[
			SAssignNew(Slider, SSlider)
			.IsFocusable(false)
			.Locked(TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateLambda([=]()
			{
				if (InArgs._Enabled.IsSet() || InArgs._Enabled.IsBound())
				{
					return !InArgs._Enabled.Get();
				}
				else
				{
					return false;
				}
			})))
			.Value(InArgs._Value)
			.MinValue(InArgs._MinValue)
			.MaxValue(InArgs._MaxValue)
			.OnValueChanged(this, &SNeutronSlider::OnSliderValueChanged)
			.OnMouseCaptureBegin(this, &SNeutronSlider::OnMouseCaptured)
			.OnMouseCaptureEnd(this, &SNeutronSlider::OnMouseReleased)
			.Style(&Theme.SliderStyle)
			.SliderBarColor(FLinearColor(0, 0, 0, 0))
		]
	);

	// clang-format on
}

/*----------------------------------------------------
    Interaction
----------------------------------------------------*/

void SNeutronSlider::Tick(const FGeometry& AllottedGeometry, const double CurrentTime, const float DeltaTime)
{
	SNeutronButton::Tick(AllottedGeometry, CurrentTime, DeltaTime);

	if (Slider->GetValue() != CurrentValue)
	{
		float Delta = CurrentValue - Slider->GetValue();
		float Range = Slider->GetMaxValue() - Slider->GetMinValue();
		Delta       = FMath::Clamp(Delta, -SliderSpeed * Range * DeltaTime, SliderSpeed * Range * DeltaTime);

		Slider->SetValue(Slider->GetValue() + Delta);
	}
}

bool SNeutronSlider::HorizontalAnalogInput(float Value)
{
	if (IsButtonEnabled())
	{
		float Range = Slider->GetMaxValue() - Slider->GetMinValue();

		if (Analog)
		{
			OnSliderValueChanged(CurrentValue + SliderAnalogSpeed * Range * Value);
		}
		else if (Slider->GetValue() == CurrentValue)
		{
			if (Value >= 0.8f)
			{
				OnIncrement();
			}
			else if (Value <= -0.8f)
			{
				OnDecrement();
			}
		}

		return true;
	}
	else
	{
		return false;
	}
}

float SNeutronSlider::GetCurrentValue() const
{
	return CurrentValue;
}

float SNeutronSlider::GetMinValue() const
{
	return Slider->GetMinValue();
}

float SNeutronSlider::GetMaxValue() const
{
	return Slider->GetMaxValue();
}

void SNeutronSlider::SetMinValue(float Value)
{
	Slider->SetMinAndMaxValues(Value, Slider->GetMaxValue());
}

void SNeutronSlider::SetMaxValue(float Value)
{
	Slider->SetMinAndMaxValues(Slider->GetMinValue(), Value);
}

void SNeutronSlider::SetCurrentValue(float Value)
{
	CurrentValue = Value;
	Slider->SetValue(CurrentValue);
}

/*----------------------------------------------------
    Callbacks
----------------------------------------------------*/

FLinearColor SNeutronSlider::GetColor() const
{
	const UNeutronMenuManager* MenuManager = UNeutronMenuManager::Get();
	NCHECK(MenuManager);

	return MenuManager->GetInterfaceColor();
}

FSlateColor SNeutronSlider::GetSlateColor() const
{
	return GetColor();
}

const FSlateBrush* SNeutronSlider::GetBackgroundBrush() const
{
	const FNeutronMainTheme& Theme = FNeutronStyleSet::GetMainTheme();

	if (!IsEnabled())
	{
		return &Theme.SliderStyle.DisabledBarImage;
	}
	else
	{
		return &Theme.SliderStyle.NormalBarImage;
	}
}

EVisibility SNeutronSlider::GetActionVisibility() const
{
	if ((Action.IsBound() || Action.IsSet()) && Action.Get() != NAME_None)
	{
		if (UNeutronMenuManager::Get()->GetFirstActionKey(Action.Get()) != FKey(NAME_None))
		{
			return EVisibility::Visible;
		}
	}

	return EVisibility::Collapsed;
}

void SNeutronSlider::OnSliderValueChanged(float Value)
{
	Value = FMath::Clamp(Value, Slider->GetMinValue(), Slider->GetMaxValue());
	if (Value != CurrentValue)
	{
		if (Analog)
		{
			ValueChanged.ExecuteIfBound(Value);
		}
		else
		{
			float StepCount = 1.0f / ValueStep;
			float StepValue = FMath::RoundToInt(StepCount * Value);
			Value           = StepValue / StepCount;

			if (Value != CurrentValue)
			{
				ValueChanged.ExecuteIfBound(Value);
			}

			if (IsMouseCaptured)
			{
				Slider->SetValue(Value);
			}
		}

		CurrentValue = Value;
	}
}

void SNeutronSlider::OnIncrement()
{
	float NewValue = CurrentValue + ValueStep;
	NewValue       = FMath::Clamp(NewValue, Slider->GetMinValue(), Slider->GetMaxValue());
	CurrentValue   = NewValue;
	ValueChanged.ExecuteIfBound(CurrentValue);
}

void SNeutronSlider::OnDecrement()
{
	float NewValue = CurrentValue - ValueStep;
	NewValue       = FMath::Clamp(NewValue, Slider->GetMinValue(), Slider->GetMaxValue());
	CurrentValue   = NewValue;
	ValueChanged.ExecuteIfBound(CurrentValue);
}

void SNeutronSlider::OnMouseCaptured()
{
	IsMouseCaptured = true;

	OnButtonClicked();
}

void SNeutronSlider::OnMouseReleased()
{
	IsMouseCaptured = false;
}
