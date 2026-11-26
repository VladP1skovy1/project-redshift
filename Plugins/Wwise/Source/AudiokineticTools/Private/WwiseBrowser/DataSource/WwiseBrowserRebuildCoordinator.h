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

#include "CoreMinimal.h"

class FWwiseBrowserRebuildCoordinator
{
public:
	bool RequestRebuild(bool bRebuildProjectDB, bool bRebuildWaapi)
	{
		bPendingProjectDB |= bRebuildProjectDB;
		bPendingWaapi |= bRebuildWaapi;

		Generation.fetch_add(1, std::memory_order_relaxed);

		bool bExpected = false;
		return bInFlight.compare_exchange_strong(bExpected, true, std::memory_order_relaxed);
	}

	struct FBrowserRebuildOperation
	{
		uint32 Generation = 0;
		bool bProjectDB = false;
		bool bWaapi = false;
	};

	FBrowserRebuildOperation BeginChain()
	{
		FBrowserRebuildOperation Out;
		Out.Generation = Generation.load(std::memory_order_relaxed);
		Out.bProjectDB = bPendingProjectDB;
		Out.bWaapi = bPendingWaapi;
		bPendingProjectDB = false;
		bPendingWaapi = false;
		return Out;
	}

	bool IsStale(uint32 Gen) const
	{
		return Generation.load(std::memory_order_relaxed) != Gen;
	}

	bool RequireRelaunch(uint32 FinishedGeneration)
	{
		if (IsStale(FinishedGeneration))
		{
			return true;
		}
		bInFlight.store(false, std::memory_order_relaxed);
		return false;
	}

	uint32 CurrentGeneration() const { return Generation.load(std::memory_order_relaxed); }
	bool IsInFlight() const { return bInFlight.load(std::memory_order_relaxed); }

	const std::atomic<uint32>& GenerationRef() const { return Generation; }

	void CancelCurrentRebuild() { Generation.fetch_add(1, std::memory_order_relaxed); }

private:
	std::atomic<uint32> Generation{ 0 };
	std::atomic<bool> bInFlight{ false };
	bool bPendingProjectDB = false;
	bool bPendingWaapi = false;
};
