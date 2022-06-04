// Neutron - Gwennaël Arbona

#pragma once

#include "EngineMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/StreamableManager.h"
#include "NeutronAssetManager.generated.h"

/*----------------------------------------------------
    Description base classes
----------------------------------------------------*/

/** Interface for objects that will provide a description interface */
class NEUTRON_API INeutronDescriptibleInterface
{
public:

	/** Return the full description of this asset as inline text */
	FText GetInlineDescription() const
	{
		return GetFormattedDescription("  ");
	}

	/** Return the full description of this asset as paragraph text */
	FText GetParagraphDescription() const
	{
		return GetFormattedDescription("\n");
	}

	/** Return the formatted description of this asset */
	FText GetFormattedDescription(FString Delimiter) const;

	/** Return details on this asset */
	virtual TArray<FText> GetDescription() const
	{
		return TArray<FText>();
	}
};

/** Asset description */
UCLASS(ClassGroup = (Neutron))
class NEUTRON_API UNeutronAssetDescription
	: public UDataAsset
	, public INeutronDescriptibleInterface
{
	GENERATED_BODY()

public:

	/** Procedurally generate a screenshot of this asset */
	UFUNCTION(Category = Neutron, BlueprintCallable, CallInEditor)
	void UpdateAssetRender();

	// Write an asset description to JSON
	static void SaveAsset(TSharedPtr<class FJsonObject> Save, FString AssetName, const UNeutronAssetDescription* Asset);

	// Get an asset description from JSON
	static const UNeutronAssetDescription* LoadAsset(TSharedPtr<class FJsonObject> Save, FString AssetName);

	template <typename T>
	static const T* LoadAsset(TSharedPtr<class FJsonObject> Save, FString AssetName)
	{
		return Cast<T>(LoadAsset(Save, AssetName));
	}

	/** Get a list of assets to load before use*/
	virtual TArray<FSoftObjectPath> GetAsyncAssets() const
	{
		TArray<FSoftObjectPath> Result;

		for (TFieldIterator<FSoftObjectProperty> PropIt(GetClass()); PropIt; ++PropIt)
		{
			FSoftObjectProperty* Property = *PropIt;
			FSoftObjectPtr       Ptr      = Property->GetPropertyValue(Property->ContainerPtrToValuePtr<int32>(this));
			if (!Ptr.IsNull())
			{
				Result.AddUnique(Ptr.ToSoftObjectPath());
			}
		}

		return Result;
	}

	/** Get the desired display settings when taking shots of this asset */
	virtual struct FNeutronAssetPreviewSettings GetPreviewSettings() const;

	/** Configure the target actor for taking shots of this asset */
	virtual void ConfigurePreviewActor(class AActor* Actor) const
	{}

public:

	// Identifier
	UPROPERTY(Category = Neutron, EditDefaultsOnly)
	FGuid Identifier;

	// Display name
	UPROPERTY(Category = Neutron, EditDefaultsOnly)
	FText Name;

	// Whether this asset is a special hidden one
	UPROPERTY(Category = Neutron, EditDefaultsOnly)
	bool Hidden;

	// Whether this asset is a special default asset
	UPROPERTY(Category = Neutron, EditDefaultsOnly)
	bool Default;

	// Generated texture file
	UPROPERTY()
	FSlateBrush AssetRender;
};

/*----------------------------------------------------
    Asset manager
----------------------------------------------------*/

/** Catalog of dynamic assets to load in game */
UCLASS(ClassGroup = (Neutron))
class NEUTRON_API UNeutronAssetManager : public UObject
{
	GENERATED_BODY()

public:

	UNeutronAssetManager();

	/*----------------------------------------------------
	    Public methods
	----------------------------------------------------*/

	/** Get the singleton instance */
	static UNeutronAssetManager* Get()
	{
		return Singleton;
	}

	/** Initialize this class */
	void Initialize(class UNeutronGameInstance* GameInstance);

	/** Find the component with the GUID that matches Identifier */
	const UNeutronAssetDescription* GetAsset(FGuid Identifier) const
	{
		const UNeutronAssetDescription* const* Entry = Catalog.Find(Identifier);

		return Entry ? *Entry : nullptr;
	}

	/** Find the component with the GUID that matches Identifier */
	template <typename T>
	const T* GetAsset(FGuid Identifier) const
	{
		return Cast<T>(GetAsset(Identifier));
	}

	/** Find all assets of a class */
	template <typename T>
	TArray<const T*> GetAssets() const
	{
		TArray<const T*> Result;

		for (const auto& Entry : Catalog)
		{
			if (Entry.Value->IsA<T>() && !Entry.Value->Hidden)
			{
				Result.Add(Cast<T>(Entry.Value));
			}
		}

		return Result;
	}

	/** Find all assets of a class and sort */
	template <typename T>
	TArray<const T*> GetSortedAssets() const
	{
		TArray<const T*> Result = GetAssets<T>();

		Result.Sort();

		return Result;
	}

	/** Find the default asset of a class */
	template <typename T>
	const T* GetDefaultAsset() const
	{
		const auto Entry = DefaultAssets.Find(T::StaticClass());
		if (Entry)
		{
			return Cast<T>(*Entry);
		}
		else
		{
			return nullptr;
		}
	}

	/** Load an asset asynchronously */
	void LoadAsset(FSoftObjectPath Entry, FStreamableDelegate Callback);

	/** Load a collection of assets synchronously */
	void LoadAssets(TArray<FSoftObjectPath> Assets);

	/** Load a collection of assets asynchronously */
	void LoadAssets(TArray<FSoftObjectPath> Assets, FStreamableDelegate Callback);

	/** Unload an asset asynchronously */
	void UnloadAsset(FSoftObjectPath Asset);

	/*----------------------------------------------------
	    Public data
	----------------------------------------------------*/

	// Singleton pointer
	static UNeutronAssetManager* Singleton;

	// All assets
	UPROPERTY()
	TMap<FGuid, const UNeutronAssetDescription*> Catalog;

	// Default assets
	UPROPERTY()
	TMap<TSubclassOf<UNeutronAssetDescription>, const UNeutronAssetDescription*> DefaultAssets;

	// Asynchronous asset loader
	FStreamableManager StreamableManager;
};
