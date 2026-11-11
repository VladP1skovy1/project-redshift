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

#include "Wwise/WwiseProjectDatabase.h"
#include <atomic>

struct WWISEPACKAGINGEDITOR_API FWwiseAssetLibraryFilteringSharedData
{
	FWwiseAssetLibraryFilteringSharedData(const FWwiseProjectDatabase& ProjectDatabase) :
		Db{ ProjectDatabase }
	{}
	virtual ~FWwiseAssetLibraryFilteringSharedData() {}

	/**
	 * All the database operations are done with the Database locked in read.
	 */
	WwiseDataStructureScopeLock Db;

	/**
	 * Cancellation token for aborting long-running filter operations.
	 * When non-null, filters should periodically check if the token value has changed.
	 */
	const std::atomic<uint32>* CancellationToken{ nullptr };

	/**
	 * The expected value of the cancellation token when this operation started.
	 * If CancellationToken->load() != ExpectedTokenValue, the operation should abort.
	 */
	uint32 ExpectedTokenValue{ 0 };

	/**
	 * Helper method to check if the current operation has been cancelled.
	 * @return true if cancellation has been requested, false otherwise.
	 */
	bool IsCancelled() const
	{
		return CancellationToken && CancellationToken->load(std::memory_order_relaxed) != ExpectedTokenValue;
	}

	/**
	 * Entire list of applicable sources.
	 */
	TArray<WwiseAnyRef> Sources;

	/**
	 * List of assets positions in Sources that are still available to be filtered.
	 */
	TSparseArray<int32> Remaining;

	/**
	 * Filtered assets, calculated from the current filters.
	 *
	 * This is what got calculated in the previous Filter action. Depending on context, this might be store in the asset or not.
	 */
	TArray<FWwiseAssetLibraryRef> FilteredAssets;

	/**
	 * Assets considered to be filtered
	 */
	TArray<FAssetData> AssetsData;

	/**
	 * Number of assets that should not be filtered due to being in a different package
	 */
	int32 SkippedAssetsCount;

	bool bConsiderAssetsData = true;

};
