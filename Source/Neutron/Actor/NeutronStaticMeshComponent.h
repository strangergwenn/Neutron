// Neutron - Gwennaël Arbona

#pragma once

#include "CoreMinimal.h"
#include "NeutronMeshInterface.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/StaticMesh.h"

#include "NeutronStaticMeshComponent.generated.h"

/** Modified static mesh component that supports dynamic materials */
UCLASS(ClassGroup = (Neutron), meta = (BlueprintSpawnableComponent))
class NEUTRON_API UNeutronStaticMeshComponent
	: public UStaticMeshComponent
	, public INeutronMeshInterface
	, public FNeutronMeshInterfaceBehavior
{
	GENERATED_BODY()

public:

	UNeutronStaticMeshComponent()
	{
		PrimaryComponentTick.bCanEverTick = true;
	}

	/*----------------------------------------------------
	    Inherited
	----------------------------------------------------*/

	virtual void BeginPlay() override
	{
		Super::BeginPlay();

		UMaterialInterface* CurrentMaterial = GetMaterial(0);
		if (CurrentMaterial == nullptr || !CurrentMaterial->IsA<UMaterialInstanceDynamic>())
		{
			FNeutronMeshInterfaceBehavior::SetupMaterial(this, CurrentMaterial);
		}
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
		bool Changed = UStaticMeshComponent::SetStaticMesh(Mesh);
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

	virtual bool HasSocket(FName SocketName) const override
	{
		return GetSocketByName(SocketName) != nullptr;
	}

	virtual FTransform GetRelativeSocketTransform(FName SocketName) const override
	{
		FVector Location = FVector::ZeroVector;
		FQuat   Rotation = FQuat::Identity;
		GetSocketWorldLocationAndRotation(SocketName, Location, Rotation);
		Location = GetComponentTransform().InverseTransformPosition(Location);
		Rotation = GetComponentTransform().InverseTransformRotation(Rotation);
		return FTransform(Rotation, Location);
	}

	virtual FVector GetExtent() const override
	{
		UStaticMesh* Mesh = GetStaticMesh();
		if (Mesh)
		{
			return Mesh->GetBounds().BoxExtent;
		}
		else
		{
			return INeutronMeshInterface::GetExtent();
		}
	}

	virtual bool MoveComponentImpl(const FVector& Delta, const FQuat& NewRotationQuat, bool bSweep, FHitResult* OutHit,
		EMoveComponentFlags MoveFlags, ETeleportType Teleport) override
	{
		FVector OriginalLocation = GetComponentLocation();
		bool    Moved            = Super::MoveComponentImpl(Delta, NewRotationQuat, bSweep, OutHit, MoveFlags, Teleport);

		if (Moved)
		{
			Moved = INeutronMeshInterface::MoveComponentHierarchy(this, OriginalLocation, Delta, NewRotationQuat, bSweep, OutHit, Teleport,
				FInternalSetWorldLocationAndRotation::CreateLambda(
					[&](const FVector& NewLocation, const FQuat& NewQuat, ETeleportType Teleport)
					{
						return InternalSetWorldLocationAndRotation(NewLocation, NewQuat, false, Teleport);
					}));
		}

		return Moved;
	}
};
