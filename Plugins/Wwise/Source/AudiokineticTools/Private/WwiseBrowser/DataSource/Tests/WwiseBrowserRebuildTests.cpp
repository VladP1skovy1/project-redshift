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

#include "Wwise/WwiseUnitTests.h"

#if WWISE_UNIT_TESTS
#include "WwiseBrowser/DataSource/WwiseBrowserRebuildCoordinator.h"
#include "WwiseBrowser/DataSource/WwiseRebuildCancellationToken.h"
#include <atomic>
#include <thread>

WWISE_TEST_CASE(BrowserRebuild_CoordinatorOnlyFirstRequestLaunches, "Wwise::AudiokineticTools::BrowserRebuild_Coordinator::OnlyFirstRequestLaunches", "[EditorContext][ProductFilter]")
{
    SECTION("Only first request launches")
    {
        FWwiseBrowserRebuildCoordinator Coordinator;
        WWISE_CHECK(Coordinator.RequestRebuild(true, false));
        //Other will have to wait until the relaunch
        WWISE_CHECK_FALSE(Coordinator.RequestRebuild(true, false));
    }
}

WWISE_TEST_CASE(BrowserRebuild_CoordinatorGenerationAlwaysIncrement, "Wwise::AudiokineticTools::BrowserRebuild_Coordinator::GenerationAlwaysIncrement", "[EditorContext][ProductFilter]")
{
    SECTION("Generation count always go upward")
    {
        FWwiseBrowserRebuildCoordinator Coordinator;
        const uint32 Generation0 = Coordinator.CurrentGeneration();
        Coordinator.RequestRebuild(false, false);
        Coordinator.RequestRebuild(false, false);
        WWISE_CHECK(Coordinator.CurrentGeneration() == Generation0 + 2);
    }
}

WWISE_TEST_CASE(BrowserRebuild_CoordinatorOldBuildBecomeStale, "Wwise::AudiokineticTools::BrowserRebuild_Coordinator::OldBuildBecomeStale", "[EditorContext][ProductFilter]")
{
    SECTION("Old build become stale on new build launch")
    {
        FWwiseBrowserRebuildCoordinator Coordinator;
        Coordinator.RequestRebuild(true, false);
        const auto RebuildOperation = Coordinator.BeginChain();
        WWISE_CHECK_FALSE(Coordinator.IsStale(RebuildOperation.Generation));
        Coordinator.RequestRebuild(false, true);
        WWISE_CHECK(Coordinator.IsStale(RebuildOperation.Generation));
    }
}

WWISE_TEST_CASE(BrowserRebuild_CoordinatorRelaunchCarryMidFlightSpecs, "Wwise::AudiokineticTools::BrowserRebuild_Coordinator::RelaunchCarryMidFlightSpecs", "[EditorContext][ProductFilter]")
{
    SECTION("Relaunches carries the specs of a request sent mid-flight")
    {
        FWwiseBrowserRebuildCoordinator Coordinator;
        Coordinator.RequestRebuild(true, false);
        auto RebuildOperation = Coordinator.BeginChain();
        Coordinator.RequestRebuild(false, true);
        WWISE_CHECK(Coordinator.RequireRelaunch(RebuildOperation.Generation));
        auto RebuiltOperation2 = Coordinator.BeginChain();
        WWISE_CHECK(!RebuiltOperation2.bProjectDB);
        WWISE_CHECK(RebuiltOperation2.bWaapi);
    }
}

WWISE_TEST_CASE(BrowserRebuild_CoordinatorLaunchingWorksAfterACleanFinish, "Wwise::AudiokineticTools::BrowserRebuild_Coordinator::LaunchingWorksAfterACleanFinish", "[EditorContext][ProductFilter]")
{
    SECTION("Launching works after a clean finish")
    {
        FWwiseBrowserRebuildCoordinator Coordinator;
        Coordinator.RequestRebuild(true, false);
        auto RebuildOperation = Coordinator.BeginChain();
        WWISE_CHECK_FALSE(Coordinator.RequireRelaunch(RebuildOperation.Generation));
        WWISE_CHECK_FALSE(Coordinator.IsInFlight());
        WWISE_CHECK(Coordinator.RequestRebuild(true, false));
    }
}

WWISE_TEST_CASE(BrowserRebuild_CancellationTripsOnGenerationAdvance, "Wwise::AudiokineticTools::BrowserRebuild_Cancellation::TripsOnGenerationAdvance", "[EditorContext][ProductFilter]")
{
    SECTION("Trips when live generation advances")
    {
        std::atomic<uint32> Generation { 7 };
        FWwiseRebuildCancellationToken Token { &Generation, 7 };
        WWISE_CHECK_FALSE(Token.IsCancelled());
        Generation.fetch_add(1, std::memory_order_relaxed);
        WWISE_CHECK(Token.IsCancelled());
    }
}

WWISE_TEST_CASE(BrowserRebuild_CancellationShouldCancelAtInterval, "Wwise::AudiokineticTools::BrowserRebuild_Cancellation::ShouldCancelAtInterval", "[EditorContext][ProductFilter]")
{
    SECTION("ShouldCancelAtInterval check")
    {
        std::atomic<uint32> Generation { 1 };
        FWwiseRebuildCancellationToken Token { &Generation, 1 };
        int32 Counter = 0;
        constexpr int32 Interval = 100;
        // untripped: will never bail
        for (int i = 0; i < 200; ++i)
        {
            WWISE_CHECK_FALSE(Token.ShouldCancelAtInterval(Counter, Interval));
        }
        
        //Increment generation
        Generation.fetch_add(1, std::memory_order_relaxed);
        bool hasTripped = false;
        for (int i = 0; i < 100; ++i)
        {
            if (Token.ShouldCancelAtInterval(Counter, Interval))
            {
                hasTripped = true;
                WWISE_CHECK(Counter == 300); //Tripped on iteration 300 (next interval)
            }
        }
        WWISE_CHECK(hasTripped) //Should've tripped.
    }
}
#endif // WWISE_UNIT_TESTS
