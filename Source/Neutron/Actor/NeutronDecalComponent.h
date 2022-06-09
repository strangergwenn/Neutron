// Neutron - Gwennaël Arbona

#pragma once

#include "CoreMinimal.h"
#include "NeutronMeshInterface.h"
#include "Components/DecalComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

#include "NeutronDecalComponent.generated.h"

/** Modified decal component that supports dynamic materials */
UCLASS(ClassGroup = (Neutron), meta = (BlueprintSpawnableComponent))
class NEUTRON_API UNeutronDecalComponent
	: public UDecalComponent
	, public INeutronMeshInterface
	, public FNeutronMeshInterfaceBehavior
{
	GENERATED_BODY()

public:

	UNeutronDecalComponent()
	{
		PrimaryComponentTick.bCanEverTick = true;
	}

	/*----------------------------------------------------
	    Inherited
	----------------------------------------------------*/

	virtual void BeginPlay() override
	{
		Super::BeginPlay();

		FNeutronMeshInterfaceBehavior::SetupMaterial(this, GetMaterial(0));
	}

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override
	{
		Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

		FNeutronMeshInterfaceBehavior::TickMaterial(DeltaTime);

		SetVisibility(CurrentMaterializationTime > 0);
	}

	virtual void SetMaterial(int32 ElementIndex, UMaterialInterface* InMaterial) override
	{
		FNeutronMeshInterfaceBehavior::SetupMaterial(this, InMaterial);
		Super::SetMaterial(0, ComponentMaterial);
	}

	virtual void Materialize(bool Force = false) override
	{
		FNeutronMeshInterfaceBehavior::Materialize(Force);
	}

	virtual void Dematerialize(bool Force = false) override
	{
		FNeutronMeshInterfaceBehavior::Dematerialize(Force);
	}

	virtual bool IsDematerializing() const override
	{
		return FNeutronMeshInterfaceBehavior::GetMaterializationAlpha() < 1;
	}

	virtual bool IsDematerialized() const override
	{
		return FNeutronMeshInterfaceBehavior::GetMaterializationAlpha() == 0;
	}

	virtual bool IsMaterialized() const override
	{
		return FNeutronMeshInterfaceBehavior::GetMaterializationAlpha() == 1;
	}

	virtual void RequestParameter(FName Name, float Value, bool Immediate = false) override
	{
		FNeutronMeshInterfaceBehavior::RequestParameter(Name, Value, Immediate);
	}

	virtual void RequestParameter(FName Name, FLinearColor Value, bool Immediate = false) override
	{
		FNeutronMeshInterfaceBehavior::RequestParameter(Name, Value, Immediate);
	}
};
