/*******************************************************************************
The content of this file includes portions of the proprietary AUDIOKINETIC Wwise
Technology released in source code form as part of the game integration package.
The content of this file may not be used without valid licenses to the
AUDIOKINETIC Wwise Technology.
Note that the use of the game engine is subject to the Unreal(R) Engine End User
License Agreement at https://www.unrealengine.com/en-US/eula/unreal
 
License Usage
 
Licensees holding valid licenses to the AUDIOKINETIC Wwise Technology may use
this file in accordance with the end user license agreement provided with the
software or, alternatively, in accordance with the terms contained
in a written agreement between you and Audiokinetic Inc.
Copyright (c) 2026 Audiokinetic Inc.
*******************************************************************************/

#include "Wwise/WwiseProjectDatabaseImpl.h"

#include "WwiseUnrealHelper.h"
#include "Wwise/Metadata/WwiseMetadataPlatformInfo.h"
#include "Wwise/WwiseResourceLoader.h"
#include "Wwise/WwiseProjectDatabaseDelegates.h"

#include "Async/Async.h"
#include "Misc/ScopedSlowTask.h"
#include "Wwise/WwiseStringConverter.h"
#include "Wwise/Stats/ProjectDatabase.h"

#define LOCTEXT_NAMESPACE "WwiseProjectDatabase"

FWwiseProjectDatabaseImpl::FWwiseProjectDatabaseImpl() :
	ResourceLoaderOverride(nullptr),
	LockedDataStructure(new WwiseDataStructure())
{
}

FWwiseProjectDatabaseImpl::~FWwiseProjectDatabaseImpl()
{
	IsAliveRef->store(false, std::memory_order_seq_cst);
}

void FWwiseProjectDatabaseImpl::UpdateDataStructure(const WwiseDBGuid* InBasePlatformGuid, FWwiseProjectDatabaseCancellationToken InToken)
{
	SCOPED_WWISEPROJECTDATABASE_EVENT_2(TEXT("FWwiseProjectDatabaseImpl::UpdateDataStructure"));

	if (!InToken.GenerationPtr)
	{
		// Untokened (synchronous) callers are authoritative and always commit. Bump the
		// generation so any concurrently in-flight tokened (async) build detects it has been cancelled
		// and discards its own result instead of racing to overwrite this one.
		BumpUpdateGeneration();
	}

	const auto IsCancelled = [InToken]() -> bool
	{
		return IsEngineExitRequested() || InToken.IsCancelled();
	};

	WwiseDBSharedPlatformId Platform;
	FDirectoryPath SourcePath;
	{
		FWwiseResourceLoaderPtr ResourceLoader = GetResourceLoader();
		if (UNLIKELY(!ResourceLoader))
		{
			return;
		}
		auto SharedPlatform = ResourceLoader->GetCurrentPlatform();
		Platform = WwiseDBSharedPlatformId(WwiseDBGuid(
			SharedPlatform.Platform->PlatformGuid.A, SharedPlatform.Platform->PlatformGuid.B,
			SharedPlatform.Platform->PlatformGuid.C, SharedPlatform.Platform->PlatformGuid.D),
			FWwiseStringConverter::ToWwiseDBString(*SharedPlatform.Platform->PlatformName.ToString()),
			FWwiseStringConverter::ToWwiseDBString(*SharedPlatform.Platform->PathRelativeToGeneratedSoundBanks.ToString()));
		Platform.Platform->ExternalSourceRootPath = FWwiseStringConverter::ToWwiseDBString(SharedPlatform.Platform->ExternalSourceRootPath.ToString());
		SourcePath = GetGeneratedSoundBanksPath();
		if (SourcePath.Path.IsEmpty())
		{
			const auto SoundBankDirectory = WwiseUnrealHelper::GetSoundBankDirectory();
			SetGeneratedSoundBanksPath(FDirectoryPath{ SoundBankDirectory });
			SourcePath = FDirectoryPath{ SoundBankDirectory };
		}
	}

	if (IsCancelled())
	{
		return;
	}

	WwiseDataStructure UpdatedDataStructure;
	FWwiseSharedPlatformId SharedPlatformToSet;
	bool bHasPlatformToSet = false;

	if (DisableDefaultPlatforms())
	{
		UE_LOG(LogWwiseProjectDatabase, Log, TEXT("UpdateDataStructure: Retrieving root data structure in (%s)"), *SourcePath.Path);
		UpdatedDataStructure = WwiseDataStructure(FWwiseStringConverter::ToWwiseDBString(SourcePath.Path), nullptr, InToken);
	}
	else
	{
		UE_LOG(LogWwiseProjectDatabase, Log, TEXT("UpdateDataStructure: Retrieving data structure for %s (Base: %s) in (%s)"), *FWwiseStringConverter::ToFString(Platform.GetPlatformName()), InBasePlatformGuid ? *FWwiseStringConverter::ToFString(InBasePlatformGuid->ToString()) : TEXT("null"), *SourcePath.Path);

		WwiseDBString PlatformName = WwiseDBString(*Platform.GetPlatformName());
		UE_LOG(LogWwiseProjectDatabase, Display, TEXT("Starting project DB Update data structure"));
		UpdatedDataStructure = WwiseDataStructure(FWwiseStringConverter::ToWwiseDBString(SourcePath.Path), &PlatformName, InToken);
		
		if (IsCancelled())
		{
			return;
		}

		WwiseDBSharedPlatformId FoundSimilarPlatform = Platform;
		for (const auto& LoadedPlatform : UpdatedDataStructure.Platforms)
		{
			WwiseDBPair<const WwiseDBSharedPlatformId, WwisePlatformDataStructure> LoadedPlatformPair(LoadedPlatform);
			FoundSimilarPlatform = LoadedPlatformPair.GetFirst();
			if (FoundSimilarPlatform == Platform)
			{
				break;
			}
		}

		if (UpdatedDataStructure.Platforms.Contains(FoundSimilarPlatform))
		{
			const WwisePlatformDataStructure& PlatformEntry = UpdatedDataStructure.Platforms.FindRef(FoundSimilarPlatform);
			if (auto PlatformInfo = PlatformEntry.PlatformRef.GetPlatformInfo())
			{
				FoundSimilarPlatform.Platform->ExternalSourceRootPath = PlatformInfo->RootPaths.ExternalSourcesOutputRoot;
			}
			else
			{
				UE_LOG(LogWwiseProjectDatabase, Warning, TEXT("Cannot set ExternalSourceRootPath for platform %s, no PlatformInfo found."), *FoundSimilarPlatform.GetPlatformName());
			}
		}

		int A, B, C, D;
		FoundSimilarPlatform.Platform->PlatformGuid.GetGuidValues(A, B, C, D);
		SharedPlatformToSet = FWwiseSharedPlatformId(FGuid(A, B, C, D),
			*FoundSimilarPlatform.Platform->PlatformName, *FoundSimilarPlatform.Platform->PathRelativeToGeneratedSoundBanks);
		SharedPlatformToSet.Platform->ExternalSourceRootPath = FName(*FoundSimilarPlatform.Platform->ExternalSourceRootPath);
		bHasPlatformToSet = true;

		if (UNLIKELY(UpdatedDataStructure.Platforms.Size() == 0))
		{
			if (!InBasePlatformGuid)
			{
				WWISE_DB_LOG(Error, "JSON metadata files are not generated. Make sure the SoundBanks are generated and that the \"Generate JSON Metadata\" setting is enabled in your Wwise Project Settings, under the SoundBanks tab.");
				return;
			}
			UE_LOG(LogWwiseProjectDatabase, Error, TEXT("UpdateDataStructure: Could not find suitable platform for %s (Base: %s) in (%s)"),
				*FWwiseStringConverter::ToFString(Platform.GetPlatformName()), InBasePlatformGuid ? *FWwiseStringConverter::ToFString(InBasePlatformGuid->ToString()) : TEXT("null"), *SourcePath.Path);
			return;
		}
	}

	if (IsCancelled())
	{
		return;
	}

	{
		std::scoped_lock WLock(LockedDataStructure->Lock);
		if (IsCancelled())
		{
			return;
		}
		LockedDataStructure.Get() = std::move(UpdatedDataStructure);
		bIsDatabaseParsed.store(true, std::memory_order_relaxed);
	}
	WWISE_DB_LOG(Log, "UpdateDataStructure: Done.");

	if (bHasPlatformToSet)
	{
		FWwiseResourceLoaderPtr ResourceLoader = GetResourceLoader();
		if (UNLIKELY(!ResourceLoader))
		{
			return;
		}
		ResourceLoader->SetPlatform(SharedPlatformToSet);
	}

	if (Get() == this && bShouldBroadcast.exchange(false, std::memory_order_acq_rel))
	{
		if (IsInGameThread())
		{
			if (FWwiseProjectDatabaseDelegates* Delegates = FWwiseProjectDatabaseDelegates::Get())
			{
				Delegates->GetOnDatabaseUpdateCompletedDelegate().Broadcast();
			}
			bShouldBroadcast.store(true, std::memory_order_release);
		}
		else
		{
			TSharedRef<std::atomic<bool>, ESPMode::ThreadSafe> AliveRef = IsAliveRef;
			AsyncTask(ENamedThreads::GameThread, [this, AliveRef]()
			{
				if (!AliveRef->load(std::memory_order_seq_cst))
				{
					return;
				}
				if (!IsEngineExitRequested())
				{
					if (FWwiseProjectDatabaseDelegates* Delegates = FWwiseProjectDatabaseDelegates::Get())
					{
						Delegates->GetOnDatabaseUpdateCompletedDelegate().Broadcast();
					}
				}
				bShouldBroadcast.store(true, std::memory_order_release);
			});
		}
	}
}

void FWwiseProjectDatabaseImpl::PrepareProjectDatabaseForPlatform(FWwiseResourceLoaderPtr&& InResourceLoader)
{
	ResourceLoaderOverride = InResourceLoader;
}

FWwiseResourceLoaderPtr FWwiseProjectDatabaseImpl::GetResourceLoader()
{
	if (ResourceLoaderOverride.IsValid())
	{
		return ResourceLoaderOverride;
	}
	else
	{
		return FWwiseResourceLoader::Get();
	}
}

const FWwiseResourceLoaderPtr FWwiseProjectDatabaseImpl::GetResourceLoader() const
{
	if (ResourceLoaderOverride.IsValid())
	{
		return ResourceLoaderOverride;
	}
	else
	{
		return FWwiseResourceLoader::Get();
	}
}

#undef LOCTEXT_NAMESPACE
