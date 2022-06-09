// Neutron - Gwennaël Arbona

#pragma once

#include "CoreMinimal.h"
#include "NeutronMeshInterface.h"
#include "Components/SplineMeshComponent.h"
#include "Engine/StaticMesh.h"

#include "NeutronSplineMeshComponent.generated.h"

/** Modified spline mesh component that supports dynamic materials */
UCLASS(ClassGroup = (Neutron), meta = (BlueprintSpawnableComponent))
class NEUTRON_API UNeutronSplineMeshComponent
	: public USplineMeshComponent
	, public INeutronMeshInterface
	, public FNeutronMeshInterfaceBehavior
{
	GENERATED_BODY()

public:

	UNeutronSplineMeshComponent()
	{
		PrimaryComponentTick.bCanEverTick = true;
		SetRenderCustomDepth(true);
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

		bAffectDistanceFieldLighting = CurrentMaterializationTime >= 0.99f;
	}

	virtual bool SetStaticMesh(UStaticMesh* Mesh) override
	{
		bool Changed = USplineMeshComponent::SetStaticMesh(Mesh);
		FNeutronMeshInterfaceBehavior::SetupMaterial(this, Mesh->GetMaterial(0));
		return Changed;
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
