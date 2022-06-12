// Neutron - Gwennaël Arbona

#pragma once

#include "CoreMinimal.h"
#include "Tickable.h"
#include "Dom/JsonObject.h"

#include "NeutronContractManager.generated.h"

/*----------------------------------------------------
    Supporting types
----------------------------------------------------*/

/** Contracts presentation details */
struct FNeutronContractDetails
{
	FText Title;
	FText Description;
	float Progress;
};

/** Contract types */
enum class ENeutronContractType : uint8
{
	Tutorial
};

/** Contract event type */
enum ENeutronContratEventType
{
	Tick
};

/** Contract event data */
struct FNeutronContractEvent
{
	FNeutronContractEvent(ENeutronContratEventType T) : Type(T)
	{}

	ENeutronContratEventType Type;
};

/** Save data */
USTRUCT()
struct FNeutronContractManagerSave
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FString> ContractSaveData;

	UPROPERTY()
	int32 CurrentTrackedContract = INDEX_NONE;
};

// Contract creation delegate
DECLARE_DELEGATE_RetVal_TwoParams(TSharedPtr<class FNeutronContract>, FNeutronContractCreationCallback, ENeutronContractType Type,
	class UNeutronGameInstance* CurrentGameInstance);

/*----------------------------------------------------
    Base contract definitions
----------------------------------------------------*/

/** Contracts internal*/
class NEUTRON_API FNeutronContract : public TSharedFromThis<FNeutronContract>
{
public:

	FNeutronContract() : GameInstance(nullptr)
	{}

	virtual ~FNeutronContract()
	{}

	/** Create the contract */
	virtual void Initialize(class UNeutronGameInstance* CurrentGameInstance);

	/** Save this object to save data */
	virtual TSharedRef<FJsonObject> Save() const;

	/** Load this object from save data */
	virtual void Load(const TSharedPtr<FJsonObject>& Data);

	/** Get a numerical type identifier for this contract */
	virtual ENeutronContractType GetType() const
	{
		return Type;
	}

	/** Get the display text, progress, etc for this contract */
	FNeutronContractDetails GetDisplayDetails() const
	{
		return Details;
	}

	/** Update this contract */
	virtual void OnEvent(const FNeutronContractEvent& Event){};

protected:

	// Local state
	ENeutronContractType        Type;
	FNeutronContractDetails     Details;
	class UNeutronGameInstance* GameInstance;
};

/*----------------------------------------------------
    Contract manager
----------------------------------------------------*/

/** Contract manager to interact with contract objects */
UCLASS(ClassGroup = (Neutron))
class NEUTRON_API UNeutronContractManager
	: public UObject
	, public FTickableGameObject
{
	GENERATED_BODY()

public:

	UNeutronContractManager();

	/*----------------------------------------------------
	    Loading & saving
	----------------------------------------------------*/

	FNeutronContractManagerSave Save() const;

	void Load(const FNeutronContractManagerSave& SaveData);

	/*----------------------------------------------------
	    System interface
	----------------------------------------------------*/

	/** Get the singleton instance */
	static UNeutronContractManager* Get()
	{
		return Singleton;
	}

	/** Initialize this class */
	void Initialize(class UNeutronGameInstance* Instance);

	/** Start playing on a new level */
	void BeginPlay(class ANeutronPlayerController* PC, FNeutronContractCreationCallback CreationCallback);

	/** Update all contracts */
	void OnEvent(FNeutronContractEvent Event);

	/*----------------------------------------------------
	    Game interface
	----------------------------------------------------*/

	/** Generate a new contract that will sit in temporary memory until it is accepted or declined */
	FNeutronContractDetails GenerateNewContract(ENeutronContractType Type);

	/** Accept the newly generated contract and save it to persistent storage */
	void AcceptContract();

	/** Decline the newly generated contract and destroy it */
	void DeclineContract();

	/** Update a contract */
	void ProgressContract(TSharedPtr<class FNeutronContract> Contract);

	/** Complete a contract and destroy it */
	void CompleteContract(TSharedPtr<class FNeutronContract> Contract);

	/** Fetch the number of active contracts */
	uint32 GetContractCount() const;

	/** Get the details text of a contract based on its index */
	FNeutronContractDetails GetContractDetails(int32 Index) const;

	/** Set a particular contract as the tracked one, pass INDEX_NONE to untrack */
	void SetTrackedContract(int32 Index);

	/** Abandon a contract */
	void AbandonContract(int32 Index);

	/** Get the tracked contract, or INDEX_NONE if none */
	int32 GetTrackedContract();

	/*----------------------------------------------------
	    Tick
	----------------------------------------------------*/

	virtual void              Tick(float DeltaTime) override;
	virtual ETickableTickType GetTickableTickType() const override
	{
		return ETickableTickType::Always;
	}
	virtual TStatId GetStatId() const override
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(UNeutronSoundManager, STATGROUP_Tickables);
	}
	virtual bool IsTickableWhenPaused() const
	{
		return true;
	}
	virtual bool IsTickableInEditor() const
	{
		return false;
	}

	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

protected:

	// Singleton pointer
	static UNeutronContractManager* Singleton;

	// Player owner
	UPROPERTY()
	class ANeutronPlayerController* PlayerController;

	// Game instance pointer
	UPROPERTY()
	class UNeutronGameInstance* GameInstance;

	// State
	bool                                       ShouldStartTutorial;
	FNeutronContractCreationCallback           ContractGenerator;
	TSharedPtr<class FNeutronContract>         GeneratedContract;
	TArray<TSharedPtr<class FNeutronContract>> CurrentContracts;
	int32                                      CurrentTrackedContract;
};
