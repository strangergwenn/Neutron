// Neutron - Gwennaël Arbona

#include "NeutronAssetManager.h"

#include "Neutron/Actor/NeutronCaptureActor.h"
#include "Neutron/Neutron.h"

#include "AssetRegistryModule.h"
#include "Dom/JsonObject.h"

// Statics
UNeutronAssetManager* UNeutronAssetManager::Singleton = nullptr;

/*----------------------------------------------------
    General purpose types
----------------------------------------------------*/

FText INeutronDescriptibleInterface::GetFormattedDescription(FString Delimiter) const
{
	FString Result;

	for (const FText& Text : GetDescription())
	{
		if (Result.Len() > 0)
		{
			Result += Delimiter;
		}

		Result += Text.ToString();
	}

	return FText::FromString(Result);
}

/*----------------------------------------------------
    Asset description
----------------------------------------------------*/

void UNeutronAssetDescription::UpdateAssetRender()
{
#if WITH_EDITOR

	// Find a capture actor
	TArray<AActor*> CaptureActors;
	for (const FWorldContext& World : GEngine->GetWorldContexts())
	{
		UGameplayStatics::GetAllActorsOfClass(World.World(), ANeutronCaptureActor::StaticClass(), CaptureActors);
		if (CaptureActors.Num())
		{
			break;
		}
	}

	// Capture
	if (CaptureActors.Num() > 0)
	{
		ANeutronCaptureActor* CaptureActor = Cast<ANeutronCaptureActor>(CaptureActors[0]);
		NCHECK(CaptureActor);

		CaptureActor->RenderAsset(this, AssetRender);
	}

#endif    // WITH_EDITOR
}

void UNeutronAssetDescription::SaveAsset(TSharedPtr<FJsonObject> Save, FString AssetName, const UNeutronAssetDescription* Asset)
{
	if (Asset)
	{
		Save->SetStringField(AssetName, Asset->Identifier.ToString(EGuidFormats::Short));
	}
};

const UNeutronAssetDescription* UNeutronAssetDescription::LoadAsset(TSharedPtr<FJsonObject> Save, FString AssetName)
{
	const UNeutronAssetDescription* Asset = nullptr;

	FString IdentifierString;
	if (Save->TryGetStringField(AssetName, IdentifierString))
	{
		FGuid AssetIdentifier;
		if (FGuid::Parse(IdentifierString, AssetIdentifier))
		{
			Asset = UNeutronAssetManager::Get()->GetAsset(AssetIdentifier);
		}
	}

	return Asset;
};

struct FNeutronAssetPreviewSettings UNeutronAssetDescription::GetPreviewSettings() const
{
	return FNeutronAssetPreviewSettings();
}

/*----------------------------------------------------
    Constructor
----------------------------------------------------*/

UNeutronAssetManager::UNeutronAssetManager() : Super()
{}

/*----------------------------------------------------
    Public methods
----------------------------------------------------*/

void UNeutronAssetManager::Initialize(class UNeutronGameInstance* GameInstance)
{
	Singleton = this;
	Catalog.Empty();

	IAssetRegistry& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();

#if WITH_EDITOR
	Registry.SearchAllAssets(true);
#endif

	// Get assets
	TArray<FAssetData> AssetList;
	Registry.GetAssetsByClass(UNeutronAssetDescription::StaticClass()->GetFName(), AssetList, true);
	for (FAssetData Asset : AssetList)
	{
		const UNeutronAssetDescription* Entry = Cast<UNeutronAssetDescription>(Asset.GetAsset());
		NCHECK(Entry);

		Catalog.Add(TPair<FGuid, const class UNeutronAssetDescription*>(Entry->Identifier, Entry));

		if (Entry->Default)
		{
			DefaultAssets.Add(
				TPair<TSubclassOf<UNeutronAssetDescription>, const class UNeutronAssetDescription*>(Entry->GetClass(), Entry));
		}
	}
}

void UNeutronAssetManager::LoadAsset(FSoftObjectPath Asset, FStreamableDelegate Callback)
{
	TArray<FSoftObjectPath> Assets;
	Assets.Add(Asset);

	StreamableManager.RequestAsyncLoad(Assets, Callback);
}

void UNeutronAssetManager::LoadAssets(TArray<FSoftObjectPath> Assets)
{
	for (FSoftObjectPath Asset : Assets)
	{
		StreamableManager.LoadSynchronous(Asset);
	}
}

void UNeutronAssetManager::LoadAssets(TArray<FSoftObjectPath> Assets, FStreamableDelegate Callback)
{
	StreamableManager.RequestAsyncLoad(Assets, Callback);
}

void UNeutronAssetManager::UnloadAsset(FSoftObjectPath Asset)
{
	StreamableManager.Unload(Asset);
}
