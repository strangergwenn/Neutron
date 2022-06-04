// Neutron - Gwennaël Arbona

#include "NeutronLoadingScreen.h"
#include "Neutron/Player/NeutronGameViewportClient.h"
#include "Neutron/Neutron.h"

#include "Widgets/Images/SThrobber.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Materials/MaterialInstanceDynamic.h"

/*----------------------------------------------------
    Constructor
----------------------------------------------------*/

void SNeutronLoadingScreen::Construct(const FArguments& InArgs)
{
	// Get args
	AnimatedMaterialInstance = InArgs._Material;
	NCHECK(AnimatedMaterialInstance);
	uint32_t Width  = InArgs._Settings->Width;
	uint32_t Height = InArgs._Settings->Height;

	// Build the top loading brush
	LoadingScreenAnimatedBrush = FDeferredCleanupSlateBrush::CreateBrush(AnimatedMaterialInstance, FVector2D(Width, Height));
	NCHECK(LoadingScreenAnimatedBrush.IsValid());

	// clang-format off
	ChildSlot
	[
		SNew(SScaleBox)
		.IgnoreInheritedScale(true)
		.VAlign(VAlign_Fill)
		.HAlign(HAlign_Fill)
		[
			// Background
			SNew(SOverlay)
			+ SOverlay::Slot()
			.VAlign(VAlign_Fill)
			.HAlign(HAlign_Fill)
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("BlackBrush"))
				.BorderBackgroundColor(this, &SNeutronLoadingScreen::GetColor)
			]

			// Animated brush
			+ SOverlay::Slot()
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
			[
				SNew(SBox)
				.VAlign(VAlign_Fill)
				.HAlign(HAlign_Fill)
				.WidthOverride(Width)
				.HeightOverride(Height)
				[
					SNew(SBorder)
					.BorderImage(LoadingScreenAnimatedBrush->GetSlateBrush())
					.BorderBackgroundColor(this, &SNeutronLoadingScreen::GetColor)
				]
			]
		]
	];
	// clang-format on

	// Defaults
	SetVisibility((EVisibility::HitTestInvisible));
	CurrentAlpha = 1;
}

void SNeutronLoadingScreen::SetFadeAlpha(float Alpha)
{
	CurrentAlpha = Alpha;
}

FSlateColor SNeutronLoadingScreen::GetColor() const
{
	return FLinearColor(1, 1, 1, CurrentAlpha);
}

float SNeutronLoadingScreen::GetCurrentTime() const
{
	return LoadingScreenTime;
}

int32 SNeutronLoadingScreen::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	return SCompoundWidget::OnPaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
}

void SNeutronLoadingScreen::Tick(const FGeometry& AllottedGeometry, const double CurrentTime, const float DeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, CurrentTime, DeltaTime);
}
