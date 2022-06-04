// Neutron - Gwennaël Arbona

#pragma once

#include "SlateBasics.h"
#include "Slate/SlateGameResources.h"

#include "Style/NeutronStyleSet.h"
#include "Style/NeutronMainTheme.h"

#include "NeutronUI.generated.h"

/*----------------------------------------------------
    Slate widget creation macros
    Implement NewNeutronButton in the user class
----------------------------------------------------*/

/** Equivalent to SNew, automatically store the button into FocusableButtons */
#define SNeutronNew(WidgetType, ...)                                                                                      \
	NewNeutronButton<WidgetType>(#WidgetType, __FILE__, __LINE__, RequiredArgs::MakeRequiredArgs(__VA_ARGS__), false) <<= \
		TYPENAME_OUTSIDE_TEMPLATE WidgetType::FArguments()

/** Equivalent to SNew, automatically store the button into FocusableButtons and set as default focus */
#define SNeutronDefaultNew(WidgetType, ...)                                                                              \
	NewNeutronButton<WidgetType>(#WidgetType, __FILE__, __LINE__, RequiredArgs::MakeRequiredArgs(__VA_ARGS__), true) <<= \
		TYPENAME_OUTSIDE_TEMPLATE WidgetType::FArguments()

/** Equivalent to SAssignNew, automatically store the button into FocusableButtons */
#define SNeutronAssignNew(ExposeAs, WidgetType, ...)                                                                                       \
	NewNeutronButton<WidgetType>(#WidgetType, __FILE__, __LINE__, RequiredArgs::MakeRequiredArgs(__VA_ARGS__), false).Expose(ExposeAs) <<= \
		TYPENAME_OUTSIDE_TEMPLATE WidgetType::FArguments()

/** Equivalent to SAssignNew, automatically store the button into FocusableButtons and set as default focus */
#define SNeutronDefaultAssignNew(ExposeAs, WidgetType, ...)                                                                               \
	NewNeutronButton<WidgetType>(#WidgetType, __FILE__, __LINE__, RequiredArgs::MakeRequiredArgs(__VA_ARGS__), true).Expose(ExposeAs) <<= \
		TYPENAME_OUTSIDE_TEMPLATE WidgetType::FArguments()

/*----------------------------------------------------
    General purpose types
----------------------------------------------------*/

// Callbacks
DECLARE_DELEGATE(FNeutronAsyncAction);
DECLARE_DELEGATE_RetVal(bool, FNeutronAsyncCondition);

/** UI constants */
namespace ENeutronUIConstants {
constexpr float EaseLight           = 1.5f;
constexpr float EaseStandard        = 2.0f;
constexpr float EaseStrong          = 4.0f;
constexpr float FadeDurationMinimal = 0.1f;
constexpr float FadeDurationShort   = 0.25f;
constexpr float FadeDurationLong    = 0.4f;
constexpr float BlurAlphaOffset     = 0.25f;
}    // namespace ENeutronUIConstants

/** Notification type */
UENUM()
enum class ENeutronNotificationType : uint8
{
	Info,
	Error,
	Save,
	World,
	Time
};

/** Loading screen types */
UENUM()
enum class ENeutronLoadingScreen : uint8
{
	None,
	Launch,
	Black
};

/** Game menu interface */
class INeutronGameMenu
{
public:

	virtual void UpdateGameObjects(){};
};

/*----------------------------------------------------
    Player input types
----------------------------------------------------*/

/** Player input bindings */
class NEUTRON_API FNeutronPlayerInput
{
public:

	static const FName MenuToggle;
	static const FName MenuConfirm;
	static const FName MenuCancel;
	static const FName MenuUp;
	static const FName MenuDown;
	static const FName MenuLeft;
	static const FName MenuRight;
	static const FName MenuNext;
	static const FName MenuPrevious;

	static const FName MenuZoomIn;
	static const FName MenuZoomOut;
	static const FName MenuMoveHorizontal;
	static const FName MenuMoveVertical;
	static const FName MenuAnalogHorizontal;
	static const FName MenuAnalogVertical;

	static const FName MenuPrimary;
	static const FName MenuSecondary;
	static const FName MenuAltPrimary;
	static const FName MenuAltSecondary;
	static const FName MenuNextTab;
	static const FName MenuPreviousTab;
};

/*----------------------------------------------------
    Helpers
----------------------------------------------------*/

/** Carousel type animation */
template <int Size>
struct FNeutronCarouselAnimation
{
	FNeutronCarouselAnimation() : AnimationDuration(0.0f)
	{}

	FNeutronCarouselAnimation(float Duration, float EaseValue = ENeutronUIConstants::EaseLight)
	{
		Initialize(Duration, EaseValue);
	}

	/** Initialize the structure */
	void Initialize(float Duration, float EaseValue = ENeutronUIConstants::EaseLight)
	{
		AnimationDuration = Duration;
		AnimationEase     = EaseValue;
		CurrentTotalAlpha = 1.0f;

		FMemory::Memset(AlphaValues, 0, Size);
		FMemory::Memset(InterpolatedAlphaValues, 0, Size);
	}

	/** Feed the currently selected index */
	void Update(int32 SelectedIndex, float DeltaTime)
	{
		NCHECK(AnimationDuration > 0);

		CurrentTotalAlpha = 0.0f;
		for (int32 Index = 0; Index < Size; Index++)
		{
			if (Index == SelectedIndex)
			{
				AlphaValues[Index] += DeltaTime / AnimationDuration;
			}
			else
			{
				AlphaValues[Index] -= DeltaTime / AnimationDuration;
			}
			AlphaValues[Index] = FMath::Clamp(AlphaValues[Index], 0.0f, 1.0f);

			InterpolatedAlphaValues[Index] = FMath::InterpEaseInOut(0.0f, 1.0f, AlphaValues[Index], AnimationEase);
			CurrentTotalAlpha += InterpolatedAlphaValues[Index];
		}
	}

	/** Get the current animation alpha for this index */
	float GetAlpha(int32 Index) const
	{
		return CurrentTotalAlpha > 0.0f ? InterpolatedAlphaValues[Index] / CurrentTotalAlpha : 0.0f;
	}

private:

	// Animation data
	float AnimationDuration;
	float AnimationEase;
	float CurrentTotalAlpha;
	float AlphaValues[Size];
	float InterpolatedAlphaValues[Size];
};
