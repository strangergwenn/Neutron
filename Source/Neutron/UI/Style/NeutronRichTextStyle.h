// Neutron - Gwennaël Arbona

#pragma once

#include "SlateBasics.h"
#include "Slate/SlateGameResources.h"
#include "NeutronRichTextStyle.generated.h"

/*----------------------------------------------------
    Wrapper classes
----------------------------------------------------*/

UCLASS()
class NEUTRON_API UNeutronRichTextStyleContainer : public USlateWidgetStyleContainerBase
{
	GENERATED_BODY()

public:

	virtual const struct FSlateWidgetStyle* const GetStyle() const override
	{
		return static_cast<const struct FSlateWidgetStyle*>(&Style);
	}

	UPROPERTY(Category = Neutron, EditDefaultsOnly, meta = (ShowOnlyInnerProperties))
	FInlineTextImageStyle Style;
};
