// Neutron - Gwennaël Arbona

#pragma once

#include "Neutron/UI/NeutronUI.h"
#include "Neutron/Neutron.h"

#include "Widgets/SCompoundWidget.h"

/** Key label to show which keys are bound to an action */
class NEUTRON_API SNeutronKeyLabel : public SCompoundWidget
{
	/*----------------------------------------------------
	    Slate arguments
	----------------------------------------------------*/

	SLATE_BEGIN_ARGS(SNeutronKeyLabel) : _Alpha(1)
	{}

	SLATE_ATTRIBUTE(FKey, Key)
	SLATE_ATTRIBUTE(float, Alpha)

	SLATE_END_ARGS()

public:

	/*----------------------------------------------------
	    Constructor
	----------------------------------------------------*/

	SNeutronKeyLabel() : SCompoundWidget()
	{}

	void Construct(const FArguments& InArgs);

	/*----------------------------------------------------
	    Callbacks
	----------------------------------------------------*/

protected:

	FText GetKeyText() const;

	const FSlateBrush* GetKeyIcon() const;

	FSlateColor GetKeyIconColor() const;

	FSlateColor GetKeyTextColor() const;

	/*----------------------------------------------------
	    Private data
	----------------------------------------------------*/

protected:

	// Attributes
	TAttribute<FKey>  KeyName;
	TAttribute<float> CurrentAlpha;
};
