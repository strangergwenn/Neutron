// Neutron - Gwennaël Arbona

#pragma once

#include "Engine/DPICustomScalingRule.h"
#include "NeutronScalingRule.generated.h"

UCLASS()
class UNeutronScalingRule : public UDPICustomScalingRule
{
	GENERATED_BODY()

public:

	virtual float GetDPIScaleBasedOnSize(FIntPoint Size) const override;
};
