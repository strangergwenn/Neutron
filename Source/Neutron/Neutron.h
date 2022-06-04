// Neutron - Gwennaël Arbona

#pragma once

#include "CoreMinimal.h"
#include "Runtime/Engine/Classes/Engine/EngineBaseTypes.h"
#include "Runtime/Engine/Classes/Engine/EngineTypes.h"
#include "Modules/ModuleManager.h"
#include "Logging/LogMacros.h"

/*----------------------------------------------------
    Debugging tools
----------------------------------------------------*/

NEUTRON_API DECLARE_LOG_CATEGORY_EXTERN(LogNeutron, Log, All);

#define NDIS(Format, ...) FNeutronModule::DisplayLog(FString::Printf(TEXT(Format), ##__VA_ARGS__))
#define NLOG(Format, ...) UE_LOG(LogNeutron, Display, TEXT(Format), ##__VA_ARGS__)
#define NERR(Format, ...) UE_LOG(LogNeutron, Error, TEXT(Format), ##__VA_ARGS__)

template <typename T>
FString GetEnumString(T Value)
{
	return UEnum::GetDisplayValueAsText(Value).ToString();
}

FString NEUTRON_API GetRoleString(const AActor* Actor);

FString NEUTRON_API GetRoleString(const UActorComponent* Component);

/** Ensure an expression is true */
#define NCHECK(Expression) verify(Expression)

/*----------------------------------------------------
    Game module definition
----------------------------------------------------*/

class FNeutronModule : public IModuleInterface
{
public:

	virtual void StartupModule() override;

	virtual void ShutdownModule() override;

	/** Display a log on the screen */
	static void DisplayLog(FString Log);
};
