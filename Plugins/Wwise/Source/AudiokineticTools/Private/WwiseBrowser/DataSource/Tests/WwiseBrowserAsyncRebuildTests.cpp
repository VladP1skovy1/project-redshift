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

#include "Async/TaskGraphInterfaces.h"
#include "HAL/PlatformProcess.h"
#include "HAL/PlatformTime.h"
#include "WwiseBrowserDataSourceTestAccessors.h"

// Pumps the GameThread task queue until the real async rebuild chain
// commits and fires WwiseBrowserDataSourceRefreshed, or TimeoutSeconds elapses.
inline bool WaitForRebuildToFinish(FWwiseBrowserDataSourceTest& DataSource, double TimeoutSeconds = 5.0, TFunction<void()> OnRefreshed = nullptr)
{
	bool bFired = false;
	DataSource.WwiseBrowserDataSourceRefreshed.BindLambda([&bFired, OnRefreshed]()
	{
		bFired = true;
		if (OnRefreshed)
		{
			OnRefreshed();
		}
	});

	const double Deadline = FPlatformTime::Seconds() + TimeoutSeconds;
	while (!bFired && FPlatformTime::Seconds() < Deadline)
	{
		FTaskGraphInterface::Get().ProcessThreadUntilIdle(ENamedThreads::GameThread);
		FPlatformProcess::Sleep(0.005f);
	}

	DataSource.WwiseBrowserDataSourceRefreshed.Unbind();
	return bFired;
}

// These tests drive the real async rebuild chain : RequestRebuild -> LaunchRebuildChain -> FinishOrRelaunch
// through RequestRebuild_ForTesting

// Smoke test: confirms WaitForRebuildToFinish's ProcessThreadUntilIdle(GameThread) poll loop actually observes a real async commit.
WWISE_TEST_CASE(BrowserAsync_SmokeTestPumpObservesRealAsyncCommit, "Wwise::AudiokineticTools::BrowserAsync::SmokeTestPumpObservesRealAsyncCommit", "[EditorContext][ProductFilter]")
{
	FWwiseBrowserDataSourceTest DataSource;
	FWaapiDataSourceTest* Waapi = DataSource.SwapInWaapiAccessor_ForTesting();
	FWwiseProjectDatabaseDataSourceTest* ProjectDB = DataSource.SwapInProjectDBAccessor_ForTesting();
	DataSource.SwapInUAssetAccessor_ForTesting();

	TArray<FWwiseTreeItemPtr> PDRootItems;
	PDRootItems.SetNum(static_cast<int32>(EWwiseItemType::LastWwiseBrowserType) + 1);
	ProjectDB->SeedFixture_ForTesting(MoveTemp(PDRootItems), {});

	TArray<FWwiseTreeItemPtr> WaapiRootItems;
	WaapiRootItems.SetNum(static_cast<int32>(EWwiseItemType::LastWwiseBrowserType) + 1);
	Waapi->SeedFixture_ForTesting(MoveTemp(WaapiRootItems), {});
	Waapi->SetConnected_ForTesting();

	DataSource.RequestRebuild_ForTesting(true, true);
	WWISE_CHECK(WaitForRebuildToFinish(DataSource));
}

// A full run through the real async chain, over a fixture mixing a matched child, each side's exclusive child, and an orphan UAsset produces the correct output.
// Also ensure that WwiseBrowserDataSourceRefreshed is fired exactly once via the AsyncTask-guarded branch.
WWISE_TEST_CASE(BrowserAsync_FullRunProducesExpectedOutputAndFiresRefreshedOnce, "Wwise::AudiokineticTools::BrowserAsync::FullRunProducesExpectedOutputAndFiresRefreshedOnce", "[EditorContext][ProductFilter]")
{
	FWwiseBrowserDataSourceTest DataSource;
	FWaapiDataSourceTest* Waapi = DataSource.SwapInWaapiAccessor_ForTesting();
	FWwiseProjectDatabaseDataSourceTest* ProjectDB = DataSource.SwapInProjectDBAccessor_ForTesting();
	FUAssetDataSourceTest* UAsset = DataSource.SwapInUAssetAccessor_ForTesting();
	const FGuid EventsRootGuid = FGuid::NewGuid();
	const FGuid SharedEventGuid = FGuid::NewGuid();
	constexpr uint32 SharedEventShortId = 2001;
	const FGuid PDOnlyEventGuid = FGuid::NewGuid();
	constexpr uint32 PDOnlyEventShortId = 2002;
	const FGuid WaapiOnlyEventGuid = FGuid::NewGuid();
	constexpr uint32 WaapiOnlyEventShortId = 2003;

	TArray<FWwiseTreeItemPtr> PDRootItems;
	PDRootItems.SetNum(static_cast<int32>(EWwiseItemType::LastWwiseBrowserType) + 1);
	FWwiseTreeItemPtr PDEventsRoot = MakeShared<FWwiseTreeItem>(FString(TEXT("Events")), FString(TEXT("\\Events")), nullptr, EWwiseItemType::Event, EventsRootGuid);
	FWwiseTreeItemPtr PDSharedEvent = MakeShared<FWwiseTreeItem>(FString(TEXT("Play_Footstep")), FString(TEXT("\\Events\\Play_Footstep")), nullptr, EWwiseItemType::Event, SharedEventGuid, SharedEventShortId);
	FWwiseTreeItemPtr PDOnlyEvent = MakeShared<FWwiseTreeItem>(FString(TEXT("Play_Jump")), FString(TEXT("\\Events\\Play_Jump")), nullptr, EWwiseItemType::Event, PDOnlyEventGuid, PDOnlyEventShortId);
	PDEventsRoot->AddChild(PDSharedEvent);
	PDEventsRoot->AddChild(PDOnlyEvent);
	PDRootItems[static_cast<int32>(EWwiseItemType::Event)] = PDEventsRoot;
	ProjectDB->SeedFixture_ForTesting(MoveTemp(PDRootItems), {});

	TArray<FWwiseTreeItemPtr> WaapiRootItems;
	WaapiRootItems.SetNum(static_cast<int32>(EWwiseItemType::LastWwiseBrowserType) + 1);
	FWwiseTreeItemPtr WaapiEventsRoot = MakeShared<FWwiseTreeItem>(FString(TEXT("Events")), FString(TEXT("\\Events")), nullptr, EWwiseItemType::Event, EventsRootGuid);
	FWwiseTreeItemPtr WaapiSharedEvent = MakeShared<FWwiseTreeItem>(FString(TEXT("Play_Footstep")), FString(TEXT("\\Events\\Play_Footstep")), nullptr, EWwiseItemType::Event, SharedEventGuid, SharedEventShortId);
	FWwiseTreeItemPtr WaapiOnlyEvent = MakeShared<FWwiseTreeItem>(FString(TEXT("Play_Land")), FString(TEXT("\\Events\\Play_Land")), nullptr, EWwiseItemType::Event, WaapiOnlyEventGuid, WaapiOnlyEventShortId);
	WaapiEventsRoot->AddChild(WaapiSharedEvent);
	WaapiEventsRoot->AddChild(WaapiOnlyEvent);
	WaapiRootItems[static_cast<int32>(EWwiseItemType::Event)] = WaapiEventsRoot;
	Waapi->SeedFixture_ForTesting(MoveTemp(WaapiRootItems), {});
	Waapi->SetConnected_ForTesting();

	UAssetDataSourceInformation Orphan;
	Orphan.Type = EWwiseItemType::Event;
	Orphan.Id.Name = FName(TEXT("OrphanEvent"));
	Orphan.Id.ShortId = 54321;
	Orphan.AssetName = Orphan.Id.Name;
	Orphan.AssetsData.Add(FAssetData());
	UAsset->SeedOrphans_ForTesting({}, { { Orphan.Id.Name, Orphan } });

	DataSource.RequestRebuild_ForTesting(true, true);

	int32 RefreshCount = 0;
	WWISE_CHECK(WaitForRebuildToFinish(DataSource, 5.0, [&RefreshCount]() { ++RefreshCount; }));
	WWISE_CHECK(RefreshCount == 1);

	FWwiseTreeItemPtr EventsRoot = DataSource.GetTreeRootForType(EWwiseItemType::Event);
	WWISE_CHECK(EventsRoot.IsValid());
	if (EventsRoot)
	{
		WWISE_CHECK(EventsRoot->GetChildren().Num() == 3);
	}

	TArray<FWwiseTreeItemPtr>& AllRoots = DataSource.GetRootItemsUnfiltered_ForTesting();
	WWISE_CHECK(AllRoots.Num() > 0);
	if (AllRoots.Num() > 0)
	{
		FWwiseTreeItemPtr FoundOnlyInUnreal = AllRoots.Last();
		WWISE_CHECK(FoundOnlyInUnreal.IsValid());
		if (FoundOnlyInUnreal)
		{
			WWISE_CHECK(FoundOnlyInUnreal->GetChildren().Num() == 1);
		}
	}
}

// Ensure that a generation mismatch triggered from the background TaskGraph worker thread cancel the current run
// and that the coordinator's relaunched run commits cleanly and fires the delegate exactly once (for the relaunch's commit only).
WWISE_TEST_CASE(BrowserAsync_CancellationMidLoopDiscardsThenRelaunchesCleanly, "Wwise::AudiokineticTools::BrowserAsync::CancellationMidLoopDiscardsThenRelaunchesCleanly", "[EditorContext][ProductFilter]")
{
	const FGuid SharedEventGuid = FGuid::NewGuid();
	constexpr uint32 SharedEventShortId = 2001;
	FWwiseBrowserDataSourceTest DataSource;
	FWaapiDataSourceTest* Waapi = DataSource.SwapInWaapiAccessor_ForTesting();
	FWwiseProjectDatabaseDataSourceTest* ProjectDB = DataSource.SwapInProjectDBAccessor_ForTesting();
	DataSource.SwapInUAssetAccessor_ForTesting();

	const FGuid EventsRootGuid = FGuid::NewGuid();
	const FGuid AuxBusRootGuid = FGuid::NewGuid();

	TArray<FWwiseTreeItemPtr> PDRootItems;
	PDRootItems.SetNum(static_cast<int32>(EWwiseItemType::LastWwiseBrowserType) + 1);
	//Event Root + 1 children
	FWwiseTreeItemPtr PDEventsRoot = PDRootItems[static_cast<int32>(EWwiseItemType::Event)] = MakeShared<FWwiseTreeItem>(FString(TEXT("Events")), FString(TEXT("\\Events")), nullptr, EWwiseItemType::Event, EventsRootGuid);
	FWwiseTreeItemPtr PDOnlyEvent = MakeShared<FWwiseTreeItem>(FString(TEXT("Play_Footstep")), FString(TEXT("\\Events\\Play_Footstep")), nullptr, EWwiseItemType::Event, SharedEventGuid, SharedEventShortId);
	PDEventsRoot->AddChild(PDOnlyEvent);
	//Bus Root
	PDRootItems[static_cast<int32>(EWwiseItemType::AuxBus)] = MakeShared<FWwiseTreeItem>(FString(TEXT("Busses")), FString(TEXT("\\Busses")), nullptr, EWwiseItemType::AuxBus, AuxBusRootGuid);
	ProjectDB->SeedFixture_ForTesting(MoveTemp(PDRootItems), {});

	TArray<FWwiseTreeItemPtr> WaapiRootItems;
	WaapiRootItems.SetNum(static_cast<int32>(EWwiseItemType::LastWwiseBrowserType) + 1);
	Waapi->SeedFixture_ForTesting(MoveTemp(WaapiRootItems), {});
	Waapi->SetConnected_ForTesting();

	// Cancel right after Event is processed, on the first merge pass only.
	// The second pass (the relaunch triggered by this same cancellation) must be allowed to commit cleanly.
	bool bHasCancelledOnce = false;
	DataSource.OnMergeTypeProcessed_ForTesting = [&DataSource, &bHasCancelledOnce](EWwiseItemType Type)
	{
		if (Type == EWwiseItemType::Event && !bHasCancelledOnce)
		{
			bHasCancelledOnce = true;
			DataSource.CancelCurrentRebuild();
		}
	};

	DataSource.RequestRebuild_ForTesting(true, true);

	int32 RefreshCount = 0;
	WWISE_CHECK(WaitForRebuildToFinish(DataSource, 5.0, [&RefreshCount]() { ++RefreshCount; }));
	WWISE_CHECK(bHasCancelledOnce);
	WWISE_CHECK(RefreshCount == 1);

	FWwiseTreeItemPtr EventsRoot = DataSource.GetTreeRootForType(EWwiseItemType::Event);
	FWwiseTreeItemPtr AuxBusRoot = DataSource.GetTreeRootForType(EWwiseItemType::AuxBus);
	WWISE_CHECK(EventsRoot.IsValid());
	if (EventsRoot)
	{
		WWISE_CHECK(EventsRoot->GetChildren().Num() == 1);
	}
	WWISE_CHECK(AuxBusRoot.IsValid());
}

// A second RequestRebuild_ForTesting call issued while the first is still in flight.
// Ensure that the resulting DataSource reads the latest fixture data and that the result is only broadcasted once.
WWISE_TEST_CASE(BrowserAsync_CoalescedSecondRequestSettlesOnLatestData, "Wwise::AudiokineticTools::BrowserAsync::CoalescedSecondRequestSettlesOnLatestData", "[EditorContext][ProductFilter]")
{
	FWwiseBrowserDataSourceTest DataSource;
	FWaapiDataSourceTest* Waapi = DataSource.SwapInWaapiAccessor_ForTesting();
	FWwiseProjectDatabaseDataSourceTest* ProjectDB = DataSource.SwapInProjectDBAccessor_ForTesting();
	DataSource.SwapInUAssetAccessor_ForTesting();

	const FGuid EventsRootGuidA = FGuid::NewGuid();
	TArray<FWwiseTreeItemPtr> PDRootItemsA;
	PDRootItemsA.SetNum(static_cast<int32>(EWwiseItemType::LastWwiseBrowserType) + 1);
	PDRootItemsA[static_cast<int32>(EWwiseItemType::Event)] = MakeShared<FWwiseTreeItem>(FString(TEXT("EventsA")), FString(TEXT("\\EventsA")), nullptr, EWwiseItemType::Event, EventsRootGuidA);
	ProjectDB->SeedFixture_ForTesting(MoveTemp(PDRootItemsA), {});

	TArray<FWwiseTreeItemPtr> WaapiRootItemsA;
	WaapiRootItemsA.SetNum(static_cast<int32>(EWwiseItemType::LastWwiseBrowserType) + 1);
	Waapi->SeedFixture_ForTesting(MoveTemp(WaapiRootItemsA), {});
	Waapi->SetConnected_ForTesting();

	DataSource.RequestRebuild_ForTesting(true, true);

	// Reseed with fixture B and issue a second request before waiting/pumping at all
	const FGuid EventsRootGuidB = FGuid::NewGuid();
	TArray<FWwiseTreeItemPtr> PDRootItemsB;
	PDRootItemsB.SetNum(static_cast<int32>(EWwiseItemType::LastWwiseBrowserType) + 1);
	PDRootItemsB[static_cast<int32>(EWwiseItemType::Event)] = MakeShared<FWwiseTreeItem>(FString(TEXT("EventsB")), FString(TEXT("\\EventsB")), nullptr, EWwiseItemType::Event, EventsRootGuidB);
	ProjectDB->SeedFixture_ForTesting(MoveTemp(PDRootItemsB), {});

	TArray<FWwiseTreeItemPtr> WaapiRootItemsB;
	WaapiRootItemsB.SetNum(static_cast<int32>(EWwiseItemType::LastWwiseBrowserType) + 1);
	Waapi->SeedFixture_ForTesting(MoveTemp(WaapiRootItemsB), {});

	DataSource.RequestRebuild_ForTesting(true, true);

	int32 RefreshCount = 0;
	WWISE_CHECK(WaitForRebuildToFinish(DataSource, 5.0, [&RefreshCount]() { ++RefreshCount; }));
	WWISE_CHECK(RefreshCount == 1);

	FWwiseTreeItemPtr EventsRoot = DataSource.GetTreeRootForType(EWwiseItemType::Event);
	WWISE_CHECK(EventsRoot.IsValid());
	if (EventsRoot)
	{
		WWISE_CHECK(EventsRoot->DisplayName == TEXT("EventsB"));
		WWISE_CHECK(EventsRoot->ItemId == EventsRootGuidB);
	}
}

// Partial-scope requests (ProjectDB-only / WAAPI-only) still merge correctly through the real chain 
WWISE_TEST_CASE(BrowserAsync_PartialScopeRequestsStillMergeCorrectlyProjectDBOnly, "Wwise::AudiokineticTools::BrowserAsync::PartialScopeRequestsStillMergeCorrectlyProjectDBOnly", "[EditorContext][ProductFilter]")
{
	SECTION("ProjectDB request")
	{
		FWwiseBrowserDataSourceTest DataSource;
		FWaapiDataSourceTest* Waapi = DataSource.SwapInWaapiAccessor_ForTesting();
		FWwiseProjectDatabaseDataSourceTest* ProjectDB = DataSource.SwapInProjectDBAccessor_ForTesting();
		DataSource.SwapInUAssetAccessor_ForTesting();

		const FGuid EventsRootGuid = FGuid::NewGuid();
		TArray<FWwiseTreeItemPtr> PDRootItems;
		PDRootItems.SetNum(static_cast<int32>(EWwiseItemType::LastWwiseBrowserType) + 1);
		PDRootItems[static_cast<int32>(EWwiseItemType::Event)] = MakeShared<FWwiseTreeItem>(FString(TEXT("Events")), FString(TEXT("\\Events")), nullptr, EWwiseItemType::Event, EventsRootGuid);
		ProjectDB->SeedFixture_ForTesting(MoveTemp(PDRootItems), {});

		TArray<FWwiseTreeItemPtr> WaapiRootItems;
		WaapiRootItems.SetNum(static_cast<int32>(EWwiseItemType::LastWwiseBrowserType) + 1);
		Waapi->SeedFixture_ForTesting(MoveTemp(WaapiRootItems), {});
		Waapi->SetConnected_ForTesting();

		DataSource.RequestRebuild_ForTesting(true, false);
		WWISE_CHECK(WaitForRebuildToFinish(DataSource));

		FWwiseTreeItemPtr EventsRoot = DataSource.GetTreeRootForType(EWwiseItemType::Event);
		WWISE_CHECK(EventsRoot.IsValid());
		if (EventsRoot)
		{
			WWISE_CHECK(EventsRoot->ItemId == EventsRootGuid);
		}
	}
}

WWISE_TEST_CASE(BrowserAsync_PartialScopeRequestsStillMergeCorrectlyWaapiOnly, "Wwise::AudiokineticTools::BrowserAsync::PartialScopeRequestsStillMergeCorrectlyWaapiOnly", "[EditorContext][ProductFilter]")
{
	SECTION("Waapi request")
	{
		FWwiseBrowserDataSourceTest DataSource;
		FWaapiDataSourceTest* Waapi = DataSource.SwapInWaapiAccessor_ForTesting();
		FWwiseProjectDatabaseDataSourceTest* ProjectDB = DataSource.SwapInProjectDBAccessor_ForTesting();
		DataSource.SwapInUAssetAccessor_ForTesting();

		TArray<FWwiseTreeItemPtr> PDRootItems;
		PDRootItems.SetNum(static_cast<int32>(EWwiseItemType::LastWwiseBrowserType) + 1);
		ProjectDB->SeedFixture_ForTesting(MoveTemp(PDRootItems), {});

		const FGuid EventsRootGuid = FGuid::NewGuid();
		TArray<FWwiseTreeItemPtr> WaapiRootItems;
		WaapiRootItems.SetNum(static_cast<int32>(EWwiseItemType::LastWwiseBrowserType) + 1);
		WaapiRootItems[static_cast<int32>(EWwiseItemType::Event)] = MakeShared<FWwiseTreeItem>(FString(TEXT("Events")), FString(TEXT("\\Events")), nullptr, EWwiseItemType::Event, EventsRootGuid);
		Waapi->SeedFixture_ForTesting(MoveTemp(WaapiRootItems), {});
		Waapi->SetConnected_ForTesting();

		DataSource.RequestRebuild_ForTesting(false, true);
		WWISE_CHECK(WaitForRebuildToFinish(DataSource));

		FWwiseTreeItemPtr EventsRoot = DataSource.GetTreeRootForType(EWwiseItemType::Event);
		WWISE_CHECK(EventsRoot.IsValid());
		if (EventsRoot)
		{
			WWISE_CHECK(EventsRoot->ItemId == EventsRootGuid);
		}
	}
}

// Destroying FWwiseBrowserDataSource while a rebuild is in flight exercises the IsAliveRef/OutstandingWorkerTasks teardown spin-wait
WWISE_TEST_CASE(BrowserAsync_TeardownDuringInFlightRebuildDoesNotHang, "Wwise::AudiokineticTools::BrowserAsync::TeardownDuringInFlightRebuildDoesNotHang", "[EditorContext][ProductFilter]")
{
	TUniquePtr<FWwiseBrowserDataSourceTest> DataSource = MakeUnique<FWwiseBrowserDataSourceTest>();
	FWaapiDataSourceTest* Waapi = DataSource->SwapInWaapiAccessor_ForTesting();
	FWwiseProjectDatabaseDataSourceTest* ProjectDB = DataSource->SwapInProjectDBAccessor_ForTesting();
	DataSource->SwapInUAssetAccessor_ForTesting();

	TArray<FWwiseTreeItemPtr> PDRootItems;
	PDRootItems.SetNum(static_cast<int32>(EWwiseItemType::LastWwiseBrowserType) + 1);
	ProjectDB->SeedFixture_ForTesting(MoveTemp(PDRootItems), {});

	TArray<FWwiseTreeItemPtr> WaapiRootItems;
	WaapiRootItems.SetNum(static_cast<int32>(EWwiseItemType::LastWwiseBrowserType) + 1);
	Waapi->SeedFixture_ForTesting(MoveTemp(WaapiRootItems), {});
	Waapi->SetConnected_ForTesting();

	DataSource->RequestRebuild_ForTesting(true, true);

	const uint32 GenerationBeforeTeardown = DataSource->GetCurrentGeneration();

	bool bTeardownDrained = false;
	uint32 GenerationAtTeardown = 0;
	bool CurrentWorkerFutureIsReady = true;
	DataSource->OnTeardownDrained = [&bTeardownDrained, &GenerationAtTeardown, &CurrentWorkerFutureIsReady](uint32 InGeneration, bool InCurrentWorkerFutureIsReady)
	{
		bTeardownDrained = true;
		GenerationAtTeardown = InGeneration;
		CurrentWorkerFutureIsReady = InCurrentWorkerFutureIsReady;
	};

	const double StartTime = FPlatformTime::Seconds();
	DataSource.Reset();
	const double ElapsedSeconds = FPlatformTime::Seconds() - StartTime;

	WWISE_CHECK(ElapsedSeconds < 5.0);

	WWISE_CHECK(bTeardownDrained);
	WWISE_CHECK(GenerationAtTeardown == GenerationBeforeTeardown+1);
	WWISE_CHECK(CurrentWorkerFutureIsReady == true);
}

#endif // WWISE_UNIT_TESTS
