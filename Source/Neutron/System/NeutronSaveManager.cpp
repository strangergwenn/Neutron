// Neutron - Gwennaël Arbona

#include "NeutronSaveManager.h"
#include "NeutronGameInstance.h"

#include "Neutron/Neutron.h"

#include "Dom/JsonObject.h"
#include "Kismet/GameplayStatics.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Async/AsyncWork.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"
#include "Policies/CondensedJsonPrintPolicy.h"

// Statics
UNeutronSaveManager* UNeutronSaveManager::Singleton = nullptr;

/*----------------------------------------------------
    Asynchronous task
----------------------------------------------------*/

class FNeutronAsyncSave : public FNonAbandonableTask
{
	friend class FAutoDeleteAsyncTask<FNeutronAsyncSave>;

public:

	FNeutronAsyncSave(
		UNeutronSaveManager* SaveSystemParam, const FString SaveNameParam, TSharedPtr<FJsonObject> JsonDataParam, bool CompressParam)
		: SaveName(SaveNameParam), JsonData(JsonDataParam), SaveSystem(SaveSystemParam), Compress(CompressParam)
	{}

protected:

	void DoWork()
	{
		NLOG("FNeutronAsyncSave::DoWork : started");

		SaveSystem->SaveGame(SaveName, JsonData, Compress);

		NLOG("FNeutronAsyncSave::DoWork : done");
	}

	FORCEINLINE TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(FNeutronAsyncSave, STATGROUP_ThreadPoolAsyncTasks);
	}

protected:

	FString                 SaveName;
	TSharedPtr<FJsonObject> JsonData;
	UNeutronSaveManager*    SaveSystem;
	bool                    Compress;
};

/*----------------------------------------------------
    Constructor
----------------------------------------------------*/

UNeutronSaveManager::UNeutronSaveManager() : Super(), CurrentSaveData(nullptr), TimeOfLastSave(0)
{}

/*----------------------------------------------------
    Interface
----------------------------------------------------*/

bool UNeutronSaveManager::DoesSaveExist(const FString SaveName)
{
	return IFileManager::Get().FileSize(*GetSaveGamePath(SaveName, true)) >= 0 ||
	       IFileManager::Get().FileSize(*GetSaveGamePath(SaveName, false)) >= 0;
}

bool UNeutronSaveManager::DeleteGame(const FString SaveName)
{
	bool Result = IFileManager::Get().Delete(*GetSaveGamePath(SaveName, false), true) ||
	              IFileManager::Get().Delete(*GetSaveGamePath(SaveName, true), true);
	return Result;
}

/*----------------------------------------------------
    Internals
----------------------------------------------------*/

void UNeutronSaveManager::SaveGameAsync(const FString SaveName, TSharedPtr<FJsonObject> JsonData, bool Compress)
{
	NCHECK(JsonData.IsValid());

	SaveListLock.Lock();

	if (SaveList.Find(JsonData) != INDEX_NONE)
	{
		NLOG("UNeutronSaveManager::SaveGameAsync : save to '%s' already in progress, aborting", *SaveName);
		SaveListLock.Unlock();
		return;
	}
	else
	{
		SaveList.Add(JsonData);
		SaveListLock.Unlock();
	}

	(new FAutoDeleteAsyncTask<FNeutronAsyncSave>(this, SaveName, JsonData, Compress))->StartBackgroundTask();
}

bool UNeutronSaveManager::SaveGame(const FString SaveName, TSharedPtr<FJsonObject> JsonData, bool Compress)
{
	NLOG("UNeutronSaveManager::SaveGame : saving to '%s'", *SaveName);

	NCHECK(JsonData.IsValid());

	SaveLock.Lock();

	// Initialize
	bool    Result       = false;
	FString FileContents = JsonToString(JsonData);

	// Serialize the JSON objects and compress
	if (Compress)
	{
		// Estimate sizes - 4 bytes to store size, 512 byes of margin for small files with higher compressed sizes
		uint32 DataSize               = FCStringAnsi::Strlen(TCHAR_TO_UTF8(*FileContents));
		int32  ContentsCompressedSize = DataSize + 512;
		int32  FullCompressedSize     = ContentsCompressedSize + 4;

		// Allocate a buffer
		uint8* CompressedData         = new uint8[FullCompressedSize];
		uint8* CompressedDataContents = CompressedData + 4;

		// Write size
		CompressedData[0] = (DataSize >> 24) & 0xFF;
		CompressedData[1] = (DataSize >> 16) & 0xFF;
		CompressedData[2] = (DataSize >> 8) & 0xFF;
		CompressedData[3] = (DataSize >> 0) & 0xFF;

		// Compress contents
		if (FCompression::CompressMemory(NAME_Zlib, CompressedDataContents, ContentsCompressedSize, TCHAR_TO_UTF8(*FileContents), DataSize))
		{
			int32 EffectiveCompressedSize = ContentsCompressedSize + 4;
			Result                        = FFileHelper::SaveArrayToFile(
									   TArrayView<const uint8>(CompressedData, EffectiveCompressedSize), *GetSaveGamePath(SaveName, true));
		}
		else
		{
			NERR("UNeutronSaveManager::SaveGame : failed to compress data");
		}

		delete[] CompressedData;
	}
	else
	{
		Result = FFileHelper::SaveStringToFile(FileContents, *GetSaveGamePath(SaveName, false));
	}

	NLOG("UNeutronSaveManager::SaveGame : done with result %d", Result);

	SaveLock.Unlock();

	// Pop the request from the list
	SaveListLock.Lock();
	SaveList.Remove(JsonData);
	SaveListLock.Unlock();

	return Result;
}

TSharedPtr<FJsonObject> UNeutronSaveManager::LoadGameInternal(const FString SaveName)
{
	FString SaveString;
	bool    SaveStringLoaded = false;

	if (DoesSaveExist(SaveName))
	{
		NLOG("UNeutronSaveManager::LoadGameInternal : loading from '%s'", *SaveName);

		TArray<uint8> Data;

		auto LoadCompressedFileToString = [&](FString& Result, const TCHAR* Filename)
		{
			TArray<uint8> CompressedData;
			if (!FFileHelper::LoadFileToArray(CompressedData, Filename))
			{
				NLOG("UNeutronSaveManager::LoadGameInternal : no compressed save file found");
				return false;
			}

			// Read uncompressed size
			int32 UncompressedSize = (CompressedData[0] << 24) + (CompressedData[1] << 16) + (CompressedData[2] << 8) + CompressedData[3];
			Data.SetNum(UncompressedSize + 1);

			// Uncompress
			if (!FCompression::UncompressMemory(
					NAME_Zlib, Data.GetData(), UncompressedSize, CompressedData.GetData() + 4, CompressedData.Num() - 4))
			{
				NERR("UNeutronSaveManager::LoadGameInternal : failed to uncompress with compressed size %d and uncompressed size %d",
					CompressedData.Num(), UncompressedSize);
				return false;
			}

			// Convert to string
			Data[UncompressedSize] = 0;
			Result                 = UTF8_TO_TCHAR(Data.GetData());
			return true;
		};

		// Check which file to load
		if (LoadCompressedFileToString(SaveString, *GetSaveGamePath(SaveName, true)))
		{
			NLOG("UNeutronSaveManager::LoadGame : read '%s'", *GetSaveGamePath(SaveName, true));
			SaveStringLoaded = true;
		}
		else if (FFileHelper::LoadFileToString(SaveString, *GetSaveGamePath(SaveName, false)))
		{
			NLOG("UNeutronSaveManager::LoadGame : read '%s'", *GetSaveGamePath(SaveName, false));
			SaveStringLoaded = true;
		}
	}

	// Get JSON data from the save string
	TSharedPtr<FJsonObject> JsonData;
	if (SaveStringLoaded)
	{
		return StringToJson(SaveString);
	}
	else
	{
		NLOG("UNeutronSaveManager::LoadGame : failed to read either '%s' or '%s'", *GetSaveGamePath(SaveName, true),
			*GetSaveGamePath(SaveName, false));

		return MakeShared<FJsonObject>();
	}
}

/*----------------------------------------------------
    Helpers
----------------------------------------------------*/

FString UNeutronSaveManager::GetSaveGamePath(const FString SaveName, bool Compressed)
{
	if (Compressed)
	{
		return FString::Printf(TEXT("%s/%s.sav"), *FPaths::ProjectSavedDir(), *SaveName);
	}
	else
	{
		return FString::Printf(TEXT("%s/%s.json"), *FPaths::ProjectSavedDir(), *SaveName);
	}
}

FString UNeutronSaveManager::JsonToString(const TSharedPtr<FJsonObject>& SaveData)
{
	FString SerializedSaveData;
	auto    JsonWriter = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&SerializedSaveData);

	if (!FJsonSerializer::Serialize(SaveData.ToSharedRef(), JsonWriter))
	{
		NCHECK(false);
	}

	JsonWriter->Close();

	return SerializedSaveData;
}

TSharedPtr<FJsonObject> UNeutronSaveManager::StringToJson(const FString& SerializedSaveData)
{
	TSharedPtr<FJsonObject>   JsonData;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(SerializedSaveData);

	if (!FJsonSerializer::Deserialize(Reader, JsonData) || !JsonData.IsValid())
	{
		NCHECK(false);
	}

	return JsonData;
}

FGuid UNeutronSaveManager::DeserializeGuid(const TSharedPtr<FJsonObject>& SaveData, const FString& FieldName)
{
	FGuid Identifier;

	if (SaveData->HasField(FieldName))
	{
		FGuid::Parse(SaveData->GetStringField(FieldName), Identifier);
	}

	return Identifier;
}
