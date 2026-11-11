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

#pragma once

#include <atomic>

#include "Wwise/WwiseProjectDatabase.h"

class FWwiseResourceLoader;
class FWwiseProjectDatabase;
using FSharedWwiseDataStructure = TSharedRef<WwiseDataStructure, ESPMode::ThreadSafe>;

class WWISEPROJECTDATABASE_API FWwiseProjectDatabaseImpl : public FWwiseProjectDatabase
{
public:
	FWwiseProjectDatabaseImpl();
	~FWwiseProjectDatabaseImpl() override;

	FWwiseResourceLoaderPtr ResourceLoaderOverride;

	void UpdateDataStructure(const WwiseDBGuid* InBasePlatformGuid = &BasePlatformGuid, FWwiseProjectDatabaseCancellationToken InToken = {}) override;

	uint32 BumpUpdateGeneration() override
	{
		return UpdateGeneration.fetch_add(1, std::memory_order_relaxed) + 1;
	}
	const std::atomic<uint32>* GetUpdateGenerationToken() const override
	{
		return &UpdateGeneration;
	}

	void PrepareProjectDatabaseForPlatform(FWwiseResourceLoaderPtr&& InResourceLoader) override;
	FWwiseResourceLoaderPtr GetResourceLoader() override;
	const FWwiseResourceLoaderPtr GetResourceLoader() const override;

protected:
	std::atomic<bool> bShouldBroadcast{ true };

	std::atomic<uint32> UpdateGeneration{ 0 };

	TSharedRef<std::atomic<bool>, ESPMode::ThreadSafe> IsAliveRef{ new std::atomic<bool>{ true } };

	FSharedWwiseDataStructure LockedDataStructure;

	/**
	 * @brief Location where the Wwise Generated SoundBanks is found on disk relative to the project
	 */
	const WwiseDBString* GeneratedSoundBanksPath;


	FSharedWwiseDataStructure& GetLockedDataStructure() override { return LockedDataStructure; }
	const FSharedWwiseDataStructure& GetLockedDataStructure() const override { return LockedDataStructure; }
};