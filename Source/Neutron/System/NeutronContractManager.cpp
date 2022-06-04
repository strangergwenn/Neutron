// Neutron - Gwennaël Arbona

#include "NeutronContractManager.h"
#include "NeutronGameInstance.h"
#include "NeutronSaveManager.h"

#include "Neutron/Player/NeutronPlayerController.h"
#include "Neutron/Neutron.h"

#define LOCTEXT_NAMESPACE "UNeutronContractManager"

// Statics
UNeutronContractManager* UNeutronContractManager::Singleton = nullptr;

/*----------------------------------------------------
    Base contract class
----------------------------------------------------*/

void FNeutronContract::Initialize(UNeutronGameInstance* CurrentGameInstance)
{
	GameInstance = CurrentGameInstance;
}

TSharedRef<FJsonObject> FNeutronContract::Save() const
{
	TSharedRef<FJsonObject> Data = MakeShared<FJsonObject>();

	Data->SetNumberField("Type", static_cast<int>(Type));

	return Data;
}

void FNeutronContract::Load(const TSharedPtr<FJsonObject>& Data)
{
	NCHECK(Data.IsValid());

	Type = static_cast<ENeutronContractType>(Data->GetNumberField("Type"));
}

/*----------------------------------------------------
    Constructor
----------------------------------------------------*/

UNeutronContractManager::UNeutronContractManager() : ShouldStartTutorial(false)
{}

/*----------------------------------------------------
    Loading & saving
----------------------------------------------------*/

FNeutronContractManagerSave UNeutronContractManager::Save() const
{
	FNeutronContractManagerSave SaveData;

	// Save contracts
	for (TSharedPtr<FNeutronContract> Contract : CurrentContracts)
	{
		SaveData.ContractSaveData.Add(UNeutronSaveManager::JsonToString(Contract->Save()));
	}

	// Save the tracked contract
	SaveData.CurrentTrackedContract = CurrentTrackedContract;

	return SaveData;
}

void UNeutronContractManager::Load(const FNeutronContractManagerSave& SaveData)
{
	// Reset the state from a potential previous session
	CurrentContracts.Empty();
	GeneratedContract.Reset();

	NCHECK(ContractGenerator.IsBound());

	// Load contracts
	if (SaveData.ContractSaveData.Num())
	{
		for (const FString& SerializedContract : SaveData.ContractSaveData)
		{
			TSharedPtr<FJsonObject> ContractData = UNeutronSaveManager::StringToJson(SerializedContract);

			FNeutronContract TestContract;
			TestContract.Load(ContractData);

			TSharedPtr<FNeutronContract> Contract = ContractGenerator.Execute(TestContract.GetType(), GameInstance);
			Contract->Load(ContractData);

			CurrentContracts.Add(Contract);
		}

		// Get the tracked contract
		CurrentTrackedContract = SaveData.CurrentTrackedContract;
	}

	// No contract structure was found, so it's a new game
	else
	{
		ShouldStartTutorial = true;

		CurrentTrackedContract = INDEX_NONE;
	}
}

/*----------------------------------------------------
    System interface
----------------------------------------------------*/

void UNeutronContractManager::Initialize(UNeutronGameInstance* Instance)
{
	Singleton    = this;
	GameInstance = Instance;
}

void UNeutronContractManager::BeginPlay(ANeutronPlayerController* PC, FNeutronContractCreationCallback CreationCallback)
{
	PlayerController  = PC;
	ContractGenerator = CreationCallback;

	if (ShouldStartTutorial)
	{
		NLOG("UNeutronContractManager::Load : adding tutorial contract");

		// Add a tutorial contract and track it
		TSharedPtr<FNeutronContract> Tutorial = ContractGenerator.Execute(ENeutronContractType::Tutorial, GameInstance);
		CurrentContracts.Add(Tutorial);

		ShouldStartTutorial = false;
	}
}

void UNeutronContractManager::OnEvent(FNeutronContractEvent Event)
{
	TArray<TSharedPtr<FNeutronContract>> SafeCurrentContracts = CurrentContracts;
	for (TSharedPtr<FNeutronContract> Contract : SafeCurrentContracts)
	{
		Contract->OnEvent(Event);
	}
}

/*----------------------------------------------------
    Game interface
----------------------------------------------------*/

FNeutronContractDetails UNeutronContractManager::GenerateNewContract(ENeutronContractType Type)
{
	NLOG("UNeutronContractManager::GenerateNewContract");

	GeneratedContract = ContractGenerator.Execute(Type, GameInstance);

	return GeneratedContract->GetDisplayDetails();
}

void UNeutronContractManager::AcceptContract()
{
	NLOG("UNeutronContractManager::AcceptContract");

	CurrentContracts.Add(GeneratedContract);
	GeneratedContract.Reset();

	PlayerController->Notify(LOCTEXT("ContractAccepted", "Contract accepted"), FText(), ENeutronNotificationType::Info);
}

void UNeutronContractManager::DeclineContract()
{
	NLOG("UNeutronContractManager::DeclineContract");

	GeneratedContract.Reset();
}

void UNeutronContractManager::ProgressContract(TSharedPtr<class FNeutronContract> Contract)
{
	NLOG("UNeutronContractManager::ProgressContract");

	PlayerController->Notify(LOCTEXT("ContractUpdated", "Contract updated"), FText(), ENeutronNotificationType::Info);
}

void UNeutronContractManager::CompleteContract(TSharedPtr<class FNeutronContract> Contract)
{
	NLOG("UNeutronContractManager::CompleteContract");

	CurrentContracts.Remove(Contract);

	PlayerController->Notify(LOCTEXT("ContractComplete", "Contract complete"), FText(), ENeutronNotificationType::Info);
}

uint32 UNeutronContractManager::GetContractCount() const
{
	return CurrentContracts.Num();
}

FNeutronContractDetails UNeutronContractManager::GetContractDetails(int32 Index) const
{
	NCHECK(Index >= 0 && Index < CurrentContracts.Num());

	return CurrentContracts[Index]->GetDisplayDetails();
}

void UNeutronContractManager::SetTrackedContract(int32 Index)
{
	NLOG("UNeutronContractManager::SetTrackedContract %d", Index);

	CurrentTrackedContract = Index;

	if (Index >= 0)
	{
		PlayerController->Notify(LOCTEXT("ContractTracked", "Contract tracked"), FText(), ENeutronNotificationType::Info);
	}
	else
	{
		PlayerController->Notify(LOCTEXT("ContractUntracked", "Contract untracked"), FText(), ENeutronNotificationType::Info);
	}
}

void UNeutronContractManager::AbandonContract(int32 Index)
{
	NLOG("UNeutronContractManager::AbandonContract %d", Index);

	NCHECK(Index >= 0 && Index < CurrentContracts.Num());

	CurrentContracts.RemoveAt(Index);
	if (CurrentTrackedContract == Index)
	{
		CurrentTrackedContract = INDEX_NONE;
	}

	PlayerController->Notify(LOCTEXT("ContractAbandoned", "Contract abandoned"), FText(), ENeutronNotificationType::Info);
}

int32 UNeutronContractManager::GetTrackedContract()
{
	return CurrentContracts.Num() > 0 ? CurrentTrackedContract : INDEX_NONE;
}

/*----------------------------------------------------
    Tick
----------------------------------------------------*/

void UNeutronContractManager::Tick(float DeltaTime)
{
	OnEvent(ENeutronContratEventType::Tick);
}

#undef LOCTEXT_NAMESPACE
