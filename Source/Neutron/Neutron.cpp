// Neutron - Gwennaël Arbona

#include "Neutron.h"
#include "UI/Style/NeutronStyleSet.h"

IMPLEMENT_MODULE(FNeutronModule, Neutron)

#define LOCTEXT_NAMESPACE "FNeutronModule"

DEFINE_LOG_CATEGORY(LogNeutron)

/*----------------------------------------------------
    Debugging tools
----------------------------------------------------*/

FString GetRoleString(const AActor* Actor)
{
	return GetEnumString(Actor->GetLocalRole());
}

FString GetRoleString(const UActorComponent* Component)
{
	return GetEnumString(Component->GetOwnerRole());
}

/*----------------------------------------------------
    Module code
----------------------------------------------------*/

void FNeutronModule::StartupModule()
{
	FNeutronStyleSet::Initialize();
}

void FNeutronModule::ShutdownModule()
{
	FNeutronStyleSet::Shutdown();
}

void FNeutronModule::DisplayLog(FString Log)
{
	GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, Log);
	UE_LOG(LogNeutron, Display, TEXT("%s"), *Log);
}

#undef LOCTEXT_NAMESPACE
