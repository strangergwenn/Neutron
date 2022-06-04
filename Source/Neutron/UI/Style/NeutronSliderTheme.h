// Neutron - Gwennaël Arbona

#pragma once

#include "SlateBasics.h"
#include "Slate/SlateGameResources.h"
#include "NeutronSliderTheme.generated.h"

USTRUCT()
struct NEUTRON_API FNeutronSliderTheme : public FSlateWidgetStyle
{
	GENERATED_BODY()

	/*----------------------------------------------------
	    Interface
	----------------------------------------------------*/

	static const FName TypeName;
	const FName        GetTypeName() const override
	{
		return TypeName;
	}

	static const FNeutronSliderTheme& GetDefault()
	{
		static FNeutronSliderTheme Default;
		return Default;
	}

	void GetResources(TArray<const FSlateBrush*>& OutBrushes) const
	{}

	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

	UPROPERTY(EditDefaultsOnly, Category = Slider) FSliderStyle SliderStyle;
};

/*----------------------------------------------------
    Wrapper classes
----------------------------------------------------*/

UCLASS()
class UNeutronSliderThemeContainer : public USlateWidgetStyleContainerBase
{
	GENERATED_BODY()

public:

	virtual const struct FSlateWidgetStyle* const GetStyle() const override
	{
		return static_cast<const struct FSlateWidgetStyle*>(&Style);
	}

	UPROPERTY(Category = Neutron, EditDefaultsOnly, meta = (ShowOnlyInnerProperties))
	FNeutronSliderTheme Style;
};
