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

#include "WwiseBrowserDataSourceTestAccessors.h"
#include "WwiseSoundEngineTest.h"

// Clean commit, no cancellation. 
// Waapi side is seeded with a single Event root.
// The ProjectDB side is left un-constructed , so it's seeded with a properly-sized-but-empty RootItems array.
WWISE_TEST_CASE(BrowserMerge_CleanCommitNoCancellation, "Wwise::AudiokineticTools::BrowserMerge::CleanCommitNoCancellation", "[EditorContext][ProductFilter]")
{
	SECTION("Clean commit with only Waapi-side data")
	{
		FWwiseBrowserDataSourceTest DataSource;
		FWaapiDataSourceTest* Waapi = DataSource.SwapInWaapiAccessor_ForTesting();
		FWwiseProjectDatabaseDataSourceTest* ProjectDB = DataSource.SwapInProjectDBAccessor_ForTesting();

		TArray<FWwiseTreeItemPtr> PDRootItems;
		PDRootItems.SetNum(static_cast<int32>(EWwiseItemType::LastWwiseBrowserType) + 1);

		ProjectDB->SeedFixture_ForTesting(MoveTemp(PDRootItems), {});

		TArray<FWwiseTreeItemPtr> WaapiRootItems;
		WaapiRootItems.SetNum(static_cast<int32>(EWwiseItemType::LastWwiseBrowserType) + 1);
		const FGuid EventRootGuid = FGuid::NewGuid();
		WaapiRootItems[static_cast<int32>(EWwiseItemType::Event)] = MakeShared<FWwiseTreeItem>(FString(TEXT("Events")), FString(TEXT("\\Events")), nullptr, EWwiseItemType::Event, EventRootGuid);

		Waapi->SeedFixture_ForTesting(MoveTemp(WaapiRootItems), {});
		Waapi->SetConnected_ForTesting();

		int32 RefreshCount = 0;
		DataSource.WwiseBrowserDataSourceRefreshed.BindLambda([&RefreshCount]() { ++RefreshCount; });

		WWISE_CHECK(DataSource.MergeDataSources_ForTesting());
		WWISE_CHECK(RefreshCount == 1);

		FWwiseTreeItemPtr EventsRoot = DataSource.GetTreeRootForType(EWwiseItemType::Event);
		WWISE_CHECK(EventsRoot.IsValid());
		if (EventsRoot)
		{
			WWISE_CHECK(EventsRoot->DisplayName == TEXT("Events"));
			WWISE_CHECK(EventsRoot->ItemId == EventRootGuid);
			WWISE_CHECK(EventsRoot->GetChildren().Num() == 0);
		}
	}
}

// Both sides carry real content under the same Events root.
// One Event present on both sides (matching Guid/ShortId/Name)
// One Event exclusive to each side. 
WWISE_TEST_CASE(BrowserMerge_MatchedAndExclusiveChildren, "Wwise::AudiokineticTools::BrowserMerge::MatchedAndExclusiveChildren", "[EditorContext][ProductFilter]")
{
	SECTION("Merge unifies a matched child and keeps each side's exclusive child")
	{
		FWwiseBrowserDataSourceTest DataSource;
		FWaapiDataSourceTest* Waapi = DataSource.SwapInWaapiAccessor_ForTesting();
		FWwiseProjectDatabaseDataSourceTest* ProjectDB = DataSource.SwapInProjectDBAccessor_ForTesting();

		const FGuid EventsRootGuid = FGuid::NewGuid();
		const FGuid SharedEventGuid = FGuid::NewGuid();
		constexpr uint32 SharedEventShortId = 1001;
		const FGuid PDOnlyEventGuid = FGuid::NewGuid();
		constexpr uint32 PDOnlyEventShortId = 1002;
		const FGuid WaapiOnlyEventGuid = FGuid::NewGuid();
		constexpr uint32 WaapiOnlyEventShortId = 1003;

		// ProjectDB side: Events root with a shared child and a ProjectDB-exclusive child.
		TArray<FWwiseTreeItemPtr> PDRootItems;
		PDRootItems.SetNum(static_cast<int32>(EWwiseItemType::LastWwiseBrowserType) + 1);
		FWwiseTreeItemPtr PDEventsRoot = MakeShared<FWwiseTreeItem>(FString(TEXT("Events")), FString(TEXT("\\Events")), nullptr, EWwiseItemType::Event, EventsRootGuid);
		FWwiseTreeItemPtr PDSharedEvent = MakeShared<FWwiseTreeItem>(FString(TEXT("Play_Footstep")), FString(TEXT("\\Events\\Play_Footstep")), nullptr, EWwiseItemType::Event, SharedEventGuid, SharedEventShortId);
		FWwiseTreeItemPtr PDOnlyEvent = MakeShared<FWwiseTreeItem>(FString(TEXT("Play_Jump")), FString(TEXT("\\Events\\Play_Jump")), nullptr, EWwiseItemType::Event, PDOnlyEventGuid, PDOnlyEventShortId);
		PDEventsRoot->AddChild(PDSharedEvent);
		PDEventsRoot->AddChild(PDOnlyEvent);
		PDRootItems[static_cast<int32>(EWwiseItemType::Event)] = PDEventsRoot;

		ProjectDB->SeedFixture_ForTesting(MoveTemp(PDRootItems), {});

		// Waapi side: Events root with the same shared child (matching Guid/ShortId/Name) and a Waapi-exclusive child.
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

		WWISE_CHECK(DataSource.MergeDataSources_ForTesting());

		FWwiseTreeItemPtr EventsRoot = DataSource.GetTreeRootForType(EWwiseItemType::Event);
		WWISE_CHECK(EventsRoot.IsValid());
		if (!EventsRoot)
		{
			return;
		}

		// Exactly three children: the merged Event and one exclusive per side
		WWISE_CHECK(EventsRoot->GetChildren().Num() == 3);

		FWwiseTreeItemPtr MergedEvent = EventsRoot->GetChild(SharedEventGuid, SharedEventShortId, TEXT("Play_Footstep"));
		WWISE_CHECK(MergedEvent.IsValid());
		if (MergedEvent)
		{
			WWISE_CHECK(MergedEvent->ItemId == SharedEventGuid);
			WWISE_CHECK(MergedEvent->bWaapiRefExists);
			WWISE_CHECK(MergedEvent->WaapiName == TEXT("Play_Footstep"));
		}
		
		FWwiseTreeItemPtr PDExclusiveEvent = EventsRoot->GetChild(FString(TEXT("Play_Jump")));
		WWISE_CHECK(PDExclusiveEvent.IsValid());
		if (PDExclusiveEvent)
		{
			WWISE_CHECK(!PDExclusiveEvent->bWaapiRefExists);
		}

		FWwiseTreeItemPtr WaapiExclusiveEvent = EventsRoot->GetChild(FString(TEXT("Play_Land")));
		WWISE_CHECK(WaapiExclusiveEvent.IsValid());
		if (WaapiExclusiveEvent)
		{
			WWISE_CHECK(WaapiExclusiveEvent->bWaapiRefExists);
			WWISE_CHECK(WaapiExclusiveEvent->WaapiName == TEXT("Play_Land"));
		}
	}
}

// A WAAPI-exclusive folder is created via CreateUnifiedTree's which never sets bWaapiRefExists, unlike every other WAAPI-exclusive path
WWISE_TEST_CASE(BrowserMerge_WaapiExclusiveFolderSkipsWaapiRefFlag, "Wwise::AudiokineticTools::BrowserMerge::WaapiExclusiveFolderSkipsWaapiRefFlag", "[EditorContext][ProductFilter]")
{
	FWwiseBrowserDataSourceTest DataSource;
	FWaapiDataSourceTest* Waapi = DataSource.SwapInWaapiAccessor_ForTesting();
	FWwiseProjectDatabaseDataSourceTest* ProjectDB = DataSource.SwapInProjectDBAccessor_ForTesting();

	const FGuid EventsRootGuid = FGuid::NewGuid();
	const FGuid WaapiFolderGuid = FGuid::NewGuid();

	// ProjectDB side: Events root with no children at all.
	TArray<FWwiseTreeItemPtr> PDRootItems;
	PDRootItems.SetNum(static_cast<int32>(EWwiseItemType::LastWwiseBrowserType) + 1);
	PDRootItems[static_cast<int32>(EWwiseItemType::Event)] = MakeShared<FWwiseTreeItem>(FString(TEXT("Events")), FString(TEXT("\\Events")), nullptr, EWwiseItemType::Event, EventsRootGuid);
	ProjectDB->SeedFixture_ForTesting(MoveTemp(PDRootItems), {});

	// Waapi side: Events root with a Folder child that has no ProjectDB counterpart.
	TArray<FWwiseTreeItemPtr> WaapiRootItems;
	WaapiRootItems.SetNum(static_cast<int32>(EWwiseItemType::LastWwiseBrowserType) + 1);
	FWwiseTreeItemPtr WaapiEventsRoot = MakeShared<FWwiseTreeItem>(FString(TEXT("Events")), FString(TEXT("\\Events")), nullptr, EWwiseItemType::Event, EventsRootGuid);
	FWwiseTreeItemPtr WaapiFolder = MakeShared<FWwiseTreeItem>(FString(TEXT("Ambience")), FString(TEXT("\\Events\\Ambience")), nullptr, EWwiseItemType::Folder, WaapiFolderGuid);
	WaapiEventsRoot->AddChild(WaapiFolder);
	WaapiRootItems[static_cast<int32>(EWwiseItemType::Event)] = WaapiEventsRoot;

	Waapi->SeedFixture_ForTesting(MoveTemp(WaapiRootItems), {});
	Waapi->SetConnected_ForTesting();

	WWISE_CHECK(DataSource.MergeDataSources_ForTesting());

	FWwiseTreeItemPtr EventsRoot = DataSource.GetTreeRootForType(EWwiseItemType::Event);
	WWISE_CHECK(EventsRoot.IsValid());
	if (!EventsRoot)
	{
		return;
	}

	WWISE_CHECK(EventsRoot->GetChildren().Num() == 1);

	FWwiseTreeItemPtr Folder = EventsRoot->GetChild(FString(TEXT("Ambience")));
	WWISE_CHECK(Folder.IsValid());
	if (Folder)
	{
		WWISE_CHECK(Folder->ItemType == EWwiseItemType::Folder);
		WWISE_CHECK(Folder->ItemId == WaapiFolderGuid);
		WWISE_CHECK_FALSE(Folder->bWaapiRefExists);
	}
}

WWISE_TEST_CASE(BrowserMerge_WaapiExclusiveGroupedItemKeepsExplicitGroupId, "Wwise::AudiokineticTools::BrowserMerge::WaapiExclusiveGroupedItemKeepsExplicitGroupId", "[EditorContext][ProductFilter]")
{
	FWwiseBrowserDataSourceTest DataSource;
	FWaapiDataSourceTest* Waapi = DataSource.SwapInWaapiAccessor_ForTesting();
	FWwiseProjectDatabaseDataSourceTest* ProjectDB = DataSource.SwapInProjectDBAccessor_ForTesting();

	const FGuid StatesRootGuid = FGuid::NewGuid();
	const FGuid WaapiStateGuid = FGuid::NewGuid();
	constexpr uint32 WaapiStateShortId = 2001;
	constexpr uint32 ExplicitGroupId = 42;

	TArray<FWwiseTreeItemPtr> PDRootItems;
	PDRootItems.SetNum(static_cast<int32>(EWwiseItemType::LastWwiseBrowserType) + 1);
	for (auto Type = EWwiseItemType::First; Type <= EWwiseItemType::LastWwiseBrowserType; ++Type)
	{
		PDRootItems[static_cast<int32>(Type)] = MakeShared<FWwiseTreeItem>(FString(), FString(), nullptr, Type, FGuid());
	}
	PDRootItems[static_cast<int32>(EWwiseItemType::State)] = MakeShared<FWwiseTreeItem>(FString(TEXT("States")), FString(TEXT("\\States")), nullptr, EWwiseItemType::State, StatesRootGuid);
	ProjectDB->SeedFixture_ForTesting(MoveTemp(PDRootItems), {});

	// Waapi side: States root with a grouped, exclusive State child that already carries a GroupId.
	TArray<FWwiseTreeItemPtr> WaapiRootItems;
	WaapiRootItems.SetNum(static_cast<int32>(EWwiseItemType::LastWwiseBrowserType) + 1);
	FWwiseTreeItemPtr WaapiStatesRoot = MakeShared<FWwiseTreeItem>(FString(TEXT("States")), FString(TEXT("\\States")), nullptr, EWwiseItemType::State, StatesRootGuid);
	FWwiseTreeItemPtr WaapiState = MakeShared<FWwiseTreeItem>(FString(TEXT("Happy")), FString(TEXT("\\States\\Emotion\\Happy")), nullptr, EWwiseItemType::State, WaapiStateGuid, WaapiStateShortId);
	WaapiState->GroupId = ExplicitGroupId;
	WaapiStatesRoot->AddChild(WaapiState);
	WaapiRootItems[static_cast<int32>(EWwiseItemType::State)] = WaapiStatesRoot;

	Waapi->SeedFixture_ForTesting(MoveTemp(WaapiRootItems), {});
	Waapi->SetConnected_ForTesting();

	WWISE_CHECK(DataSource.MergeDataSources_ForTesting());

	FWwiseTreeItemPtr StatesRoot = DataSource.GetTreeRootForType(EWwiseItemType::State);
	WWISE_CHECK(StatesRoot.IsValid());
	if (!StatesRoot)
	{
		return;
	}

	WWISE_CHECK(StatesRoot->GetChildren().Num() == 1);

	FWwiseTreeItemPtr State = StatesRoot->GetChild(FString(TEXT("Happy")));
	WWISE_CHECK(State.IsValid());
	if (State)
	{
		WWISE_CHECK(State->GroupId == ExplicitGroupId);
		WWISE_CHECK(State->ShortId == WaapiStateShortId);
		WWISE_CHECK(State->bWaapiRefExists);
	}
}

// An item WAAPI reports directly under a root, but that the ProjectDB tree still has
// nested under an old folder (a moved/renamed item), doesn't match via the direct-child GetChild lookup.
// It resolves via ProjectDBDataSource->FindItem plus FindItemRecursive walking the dst tree which also flips bSameLocation to false.
WWISE_TEST_CASE(BrowserMerge_MovedItemResolvesViaFindItem, "Wwise::AudiokineticTools::BrowserMerge::MovedItemResolvesViaFindItem", "[EditorContext][ProductFilter]")
{
	FWwiseBrowserDataSourceTest DataSource;
	FWaapiDataSourceTest* Waapi = DataSource.SwapInWaapiAccessor_ForTesting();
	FWwiseProjectDatabaseDataSourceTest* ProjectDB = DataSource.SwapInProjectDBAccessor_ForTesting();

	const FGuid EventsRootGuid = FGuid::NewGuid();
	const FGuid MovedEventGuid = FGuid::NewGuid();
	const uint32 MovedEventShortId = 3001;

	// ProjectDB side: Events root -> OldFolder -> Play_Relocated (nested).
	TArray<FWwiseTreeItemPtr> PDRootItems;
	PDRootItems.SetNum(static_cast<int32>(EWwiseItemType::LastWwiseBrowserType) + 1);
	FWwiseTreeItemPtr PDEventsRoot = MakeShared<FWwiseTreeItem>(FString(TEXT("Events")), FString(TEXT("\\Events")), nullptr, EWwiseItemType::Event, EventsRootGuid);
	FWwiseTreeItemPtr PDOldFolder = MakeShared<FWwiseTreeItem>(FString(TEXT("OldFolder")), FString(TEXT("\\Events\\OldFolder")), nullptr, EWwiseItemType::Folder, FGuid::NewGuid());
	FWwiseTreeItemPtr PDMovedEvent = MakeShared<FWwiseTreeItem>(FString(TEXT("Play_Relocated")), FString(TEXT("\\Events\\OldFolder\\Play_Relocated")), nullptr, EWwiseItemType::Event, MovedEventGuid, MovedEventShortId);
	PDOldFolder->AddChild(PDMovedEvent);
	PDEventsRoot->AddChild(PDOldFolder);
	PDRootItems[static_cast<int32>(EWwiseItemType::Event)] = PDEventsRoot;

	ProjectDB->SeedFixture_ForTesting(MoveTemp(PDRootItems), {});
	// FindItem only reads these maps, and only ConstructTree would normally populate them.
	TMap<FGuid, FWwiseTreeItemPtr> ByGuid;
	ByGuid.Add(MovedEventGuid, PDMovedEvent);
	ProjectDB->SeedLookupMaps_ForTesting(MoveTemp(ByGuid), {}, {});

	// Waapi side: Events root -> Play_Relocated directly (the item moved).
	TArray<FWwiseTreeItemPtr> WaapiRootItems;
	WaapiRootItems.SetNum(static_cast<int32>(EWwiseItemType::LastWwiseBrowserType) + 1);
	FWwiseTreeItemPtr WaapiEventsRoot = MakeShared<FWwiseTreeItem>(FString(TEXT("Events")), FString(TEXT("\\Events")), nullptr, EWwiseItemType::Event, EventsRootGuid);
	FWwiseTreeItemPtr WaapiMovedEvent = MakeShared<FWwiseTreeItem>(FString(TEXT("Play_Relocated")), FString(TEXT("\\Events\\Play_Relocated")), nullptr, EWwiseItemType::Event, MovedEventGuid, MovedEventShortId);
	WaapiEventsRoot->AddChild(WaapiMovedEvent);
	WaapiRootItems[static_cast<int32>(EWwiseItemType::Event)] = WaapiEventsRoot;

	Waapi->SeedFixture_ForTesting(MoveTemp(WaapiRootItems), {});
	Waapi->SetConnected_ForTesting();

	WWISE_CHECK(DataSource.MergeDataSources_ForTesting());

	FWwiseTreeItemPtr EventsRoot = DataSource.GetTreeRootForType(EWwiseItemType::Event);
	WWISE_CHECK(EventsRoot.IsValid());
	if (!EventsRoot)
	{
		return;
	}

	// The dst tree keeps the ProjectDB shape (OldFolder still there, nothing moved structurally).
	// Only the flags on the nested item change.
	WWISE_CHECK(EventsRoot->GetChildren().Num() == 1);

	FWwiseTreeItemPtr OldFolder = EventsRoot->GetChild(FString(TEXT("OldFolder")));
	WWISE_CHECK(OldFolder.IsValid());
	if (!OldFolder)
	{
		return;
	}

	FWwiseTreeItemPtr MovedEvent = OldFolder->GetChild(FString(TEXT("Play_Relocated")));
	WWISE_CHECK(MovedEvent.IsValid());
	if (MovedEvent)
	{
		WWISE_CHECK(MovedEvent->bWaapiRefExists);
		WWISE_CHECK_FALSE(MovedEvent->bSameLocation);
	}
}

// When a direct-child match resolves to a folder-type dst node, its ItemId gets overwritten with the WAAPI-reported Guid even though the
// match itself was made by DisplayName, not Guid.
// ProjectDB's synthetic folder Guid and WAAPI's real project Guid for the same folder can differ, and this is the one place that reconciles them.
WWISE_TEST_CASE(BrowserMerge_MatchedFolderAdoptsWaapiItemId, "Wwise::AudiokineticTools::BrowserMerge::MatchedFolderAdoptsWaapiItemId", "[EditorContext][ProductFilter]")
{
	FWwiseBrowserDataSourceTest DataSource;
	FWaapiDataSourceTest* Waapi = DataSource.SwapInWaapiAccessor_ForTesting();
	FWwiseProjectDatabaseDataSourceTest* ProjectDB = DataSource.SwapInProjectDBAccessor_ForTesting();

	const FGuid EventsRootGuid = FGuid::NewGuid();
	const FGuid PDFolderGuid = FGuid::NewGuid();
	const FGuid WaapiFolderGuid = FGuid::NewGuid();

	// ProjectDB side: Events root -> Ambience folder, with a synthetic (PD-only) Guid.
	TArray<FWwiseTreeItemPtr> PDRootItems;
	PDRootItems.SetNum(static_cast<int32>(EWwiseItemType::LastWwiseBrowserType) + 1);
	FWwiseTreeItemPtr PDEventsRoot = MakeShared<FWwiseTreeItem>(FString(TEXT("Events")), FString(TEXT("\\Events")), nullptr, EWwiseItemType::Event, EventsRootGuid);
	FWwiseTreeItemPtr PDFolder = MakeShared<FWwiseTreeItem>(FString(TEXT("Ambience")), FString(TEXT("\\Events\\Ambience")), nullptr, EWwiseItemType::Folder, PDFolderGuid);
	PDEventsRoot->AddChild(PDFolder);
	PDRootItems[static_cast<int32>(EWwiseItemType::Event)] = PDEventsRoot;

	ProjectDB->SeedFixture_ForTesting(MoveTemp(PDRootItems), {});

	// Waapi side: Events root -> Ambience folder (same name, different Guid - the real project Guid).
	TArray<FWwiseTreeItemPtr> WaapiRootItems;
	WaapiRootItems.SetNum(static_cast<int32>(EWwiseItemType::LastWwiseBrowserType) + 1);
	FWwiseTreeItemPtr WaapiEventsRoot = MakeShared<FWwiseTreeItem>(FString(TEXT("Events")), FString(TEXT("\\Events")), nullptr, EWwiseItemType::Event, EventsRootGuid);
	FWwiseTreeItemPtr WaapiFolder = MakeShared<FWwiseTreeItem>(FString(TEXT("Ambience")), FString(TEXT("\\Events\\Ambience")), nullptr, EWwiseItemType::Folder, WaapiFolderGuid);
	WaapiEventsRoot->AddChild(WaapiFolder);
	WaapiRootItems[static_cast<int32>(EWwiseItemType::Event)] = WaapiEventsRoot;

	Waapi->SeedFixture_ForTesting(MoveTemp(WaapiRootItems), {});
	Waapi->SetConnected_ForTesting();

	WWISE_CHECK(DataSource.MergeDataSources_ForTesting());

	FWwiseTreeItemPtr EventsRoot = DataSource.GetTreeRootForType(EWwiseItemType::Event);
	WWISE_CHECK(EventsRoot.IsValid());
	if (!EventsRoot)
	{
		return;
	}

	WWISE_CHECK(EventsRoot->GetChildren().Num() == 1);

	FWwiseTreeItemPtr Folder = EventsRoot->GetChild(FString(TEXT("Ambience")));
	WWISE_CHECK(Folder.IsValid());
	if (Folder)
	{
		WWISE_CHECK(Folder->bWaapiRefExists);
		WWISE_CHECK(Folder->ItemId == WaapiFolderGuid);
		WWISE_CHECK(Folder->ItemId != PDFolderGuid);
	}
}

// A ProjectDB-only tree (no Waapi root at all for this). Dst mirrors every src field, including at the root.
// What's asserted here is the field-copy behavior and that a folder specifically ends up with the
// same "nothing populated" shape due to ShouldDisplayInfo() returning false for folders.
WWISE_TEST_CASE(BrowserMerge_CreateProjectDBItemMirrorsFieldsAndSkipsFolderInfo, "Wwise::AudiokineticTools::BrowserMerge::CreateProjectDBItemMirrorsFieldsAndSkipsFolderInfo", "[EditorContext][ProductFilter]")
{
	FWwiseBrowserDataSourceTest DataSource;
	FWaapiDataSourceTest* Waapi = DataSource.SwapInWaapiAccessor_ForTesting();
	FWwiseProjectDatabaseDataSourceTest* ProjectDB = DataSource.SwapInProjectDBAccessor_ForTesting();

	const FGuid EventsRootGuid = FGuid::NewGuid();
	const FGuid FolderGuid = FGuid::NewGuid();
	const FGuid LeafEventGuid = FGuid::NewGuid();
	const uint32 LeafEventShortId = 5001;

	TArray<FWwiseTreeItemPtr> PDRootItems;
	PDRootItems.SetNum(static_cast<int32>(EWwiseItemType::LastWwiseBrowserType) + 1);
	FWwiseTreeItemPtr PDEventsRoot = MakeShared<FWwiseTreeItem>(FString(TEXT("Events")), FString(TEXT("\\Events")), nullptr, EWwiseItemType::Event, EventsRootGuid);
	FWwiseTreeItemPtr PDFolder = MakeShared<FWwiseTreeItem>(FString(TEXT("Ambience")), FString(TEXT("\\Events\\Ambience")), nullptr, EWwiseItemType::Folder, FolderGuid);
	FWwiseTreeItemPtr PDLeafEvent = MakeShared<FWwiseTreeItem>(FString(TEXT("Play_Wind")), FString(TEXT("\\Events\\Ambience\\Play_Wind")), nullptr, EWwiseItemType::Event, LeafEventGuid, LeafEventShortId);
	PDFolder->AddChild(PDLeafEvent);
	PDEventsRoot->AddChild(PDFolder);
	PDRootItems[static_cast<int32>(EWwiseItemType::Event)] = PDEventsRoot;
	ProjectDB->SeedFixture_ForTesting(MoveTemp(PDRootItems), {});

	TArray<FWwiseTreeItemPtr> WaapiRootItems;
	WaapiRootItems.SetNum(static_cast<int32>(EWwiseItemType::LastWwiseBrowserType) + 1);
	Waapi->SeedFixture_ForTesting(MoveTemp(WaapiRootItems), {});
	Waapi->SetConnected_ForTesting();

	WWISE_CHECK(DataSource.MergeDataSources_ForTesting());

	FWwiseTreeItemPtr EventsRoot = DataSource.GetTreeRootForType(EWwiseItemType::Event);
	WWISE_CHECK(EventsRoot.IsValid());
	if (!EventsRoot)
	{
		return;
	}

	WWISE_CHECK(EventsRoot->DisplayName == TEXT("Events"));
	WWISE_CHECK(EventsRoot->FolderPath == TEXT("\\Events"));
	WWISE_CHECK(EventsRoot->ItemType == EWwiseItemType::Event);
	WWISE_CHECK(EventsRoot->ItemId == EventsRootGuid);

	FWwiseTreeItemPtr Folder = EventsRoot->GetChild(FString(TEXT("Ambience")));
	WWISE_CHECK(Folder.IsValid());
	if (!Folder)
	{
		return;
	}
	WWISE_CHECK(Folder->DisplayName == TEXT("Ambience"));
	WWISE_CHECK(Folder->FolderPath == TEXT("\\Events\\Ambience"));
	WWISE_CHECK(Folder->ItemType == EWwiseItemType::Folder);
	WWISE_CHECK(Folder->ItemId == FolderGuid);
	// ShouldDisplayInfo()==false for a Folder, so CreateProjectDBItem never calls UAssetDataSource->GetAssetsInfo/IsMoved for it.
	// These stay at their freshly-constructed defaults.
	WWISE_CHECK(Folder->Assets.Num() == 0);
	WWISE_CHECK(Folder->UAssetName == NAME_None);
	WWISE_CHECK_FALSE(Folder->bIsInWrongLocation);

	FWwiseTreeItemPtr LeafEvent = Folder->GetChild(FString(TEXT("Play_Wind")));
	WWISE_CHECK(LeafEvent.IsValid());
	if (LeafEvent)
	{
		WWISE_CHECK(LeafEvent->DisplayName == TEXT("Play_Wind"));
		WWISE_CHECK(LeafEvent->FolderPath == TEXT("\\Events\\Ambience\\Play_Wind"));
		WWISE_CHECK(LeafEvent->ItemType == EWwiseItemType::Event);
		WWISE_CHECK(LeafEvent->ItemId == LeafEventGuid);
		WWISE_CHECK(LeafEvent->ShortId == LeafEventShortId);
	}
}

WWISE_TEST_CASE(BrowserMerge_ProjectDBNoneStateForcedWaapiRef, "Wwise::AudiokineticTools::BrowserMerge::ProjectDBNoneStateForcedWaapiRef", "[EditorContext][ProductFilter]")
{
	FWwiseBrowserDataSourceTest DataSource;
	FWaapiDataSourceTest* Waapi = DataSource.SwapInWaapiAccessor_ForTesting();
	FWwiseProjectDatabaseDataSourceTest* ProjectDB = DataSource.SwapInProjectDBAccessor_ForTesting();

	const FGuid StatesRootGuid = FGuid::NewGuid();
	const FGuid StateGroupGuid = FGuid::NewGuid();
	const FGuid NoneStateGuid = FGuid::NewGuid();

	TArray<FWwiseTreeItemPtr> PDRootItems;
	PDRootItems.SetNum(static_cast<int32>(EWwiseItemType::LastWwiseBrowserType) + 1);
	FWwiseTreeItemPtr PDStatesRoot = MakeShared<FWwiseTreeItem>(FString(TEXT("States")), FString(TEXT("\\States")), nullptr, EWwiseItemType::State, StatesRootGuid);
	FWwiseTreeItemPtr PDStateGroup = MakeShared<FWwiseTreeItem>(FString(TEXT("Emotion")), FString(TEXT("\\States\\Emotion")), nullptr, EWwiseItemType::StateGroup, StateGroupGuid);
	FWwiseTreeItemPtr PDNoneState = MakeShared<FWwiseTreeItem>(FString(TEXT("None")), FString(TEXT("\\States\\Emotion\\None")), nullptr, EWwiseItemType::State, NoneStateGuid);
	PDStateGroup->AddChild(PDNoneState);
	PDStatesRoot->AddChild(PDStateGroup);
	PDRootItems[static_cast<int32>(EWwiseItemType::State)] = PDStatesRoot;
	ProjectDB->SeedFixture_ForTesting(MoveTemp(PDRootItems), {});

	TArray<FWwiseTreeItemPtr> WaapiRootItems;
	WaapiRootItems.SetNum(static_cast<int32>(EWwiseItemType::LastWwiseBrowserType) + 1);
	Waapi->SeedFixture_ForTesting(MoveTemp(WaapiRootItems), {});
	Waapi->SetConnected_ForTesting();

	WWISE_CHECK(DataSource.MergeDataSources_ForTesting());

	FWwiseTreeItemPtr StatesRoot = DataSource.GetTreeRootForType(EWwiseItemType::State);
	WWISE_CHECK(StatesRoot.IsValid());
	if (!StatesRoot)
	{
		return;
	}

	FWwiseTreeItemPtr StateGroup = StatesRoot->GetChild(FString(TEXT("Emotion")));
	WWISE_CHECK(StateGroup.IsValid());
	if (!StateGroup)
	{
		return;
	}

	FWwiseTreeItemPtr NoneState = StateGroup->GetChild(FString(TEXT("None")));
	WWISE_CHECK(NoneState.IsValid());
	if (NoneState)
	{
		WWISE_CHECK(NoneState->bWaapiRefExists);
		WWISE_CHECK(NoneState->WaapiName == TEXT("None"));
	}
}

// Tripping the cancellation token before the per-type loop starts. 
WWISE_TEST_CASE(BrowserMerge_CancelledBeforeAnyTypeDiscardsPreviousSnapshot, "Wwise::AudiokineticTools::BrowserMerge::CancelledBeforeAnyTypeDiscardsPreviousSnapshot", "[EditorContext][ProductFilter]")
{
	FWwiseBrowserDataSourceTest DataSource;
	FWaapiDataSourceTest* Waapi = DataSource.SwapInWaapiAccessor_ForTesting();
	FWwiseProjectDatabaseDataSourceTest* ProjectDB = DataSource.SwapInProjectDBAccessor_ForTesting();

	const FGuid EventsRootGuid = FGuid::NewGuid();

	// Baseline: one clean, uncancelled commit, so RootItemsUnfiltered holds a known, non-empty snapshot to verify stays untouched below.
	TArray<FWwiseTreeItemPtr> PDRootItems;
	PDRootItems.SetNum(static_cast<int32>(EWwiseItemType::LastWwiseBrowserType) + 1);
	PDRootItems[static_cast<int32>(EWwiseItemType::Event)] = MakeShared<FWwiseTreeItem>(FString(TEXT("Events")), FString(TEXT("\\Events")), nullptr, EWwiseItemType::Event, EventsRootGuid);
	ProjectDB->SeedFixture_ForTesting(MoveTemp(PDRootItems), {});

	TArray<FWwiseTreeItemPtr> WaapiRootItems;
	WaapiRootItems.SetNum(static_cast<int32>(EWwiseItemType::LastWwiseBrowserType) + 1);
	Waapi->SeedFixture_ForTesting(MoveTemp(WaapiRootItems), {});
	Waapi->SetConnected_ForTesting();

	WWISE_CHECK(DataSource.MergeDataSources_ForTesting());

	FWwiseTreeItemPtr BaselineEventsRoot = DataSource.GetTreeRootForType(EWwiseItemType::Event);
	WWISE_CHECK(BaselineEventsRoot.IsValid());

	int32 RefreshCount = 0;
	DataSource.WwiseBrowserDataSourceRefreshed.BindLambda([&RefreshCount]() { ++RefreshCount; });

	WWISE_CHECK_FALSE(DataSource.MergeDataSources_ForTesting(true, 1));
	WWISE_CHECK(RefreshCount == 0);

	FWwiseTreeItemPtr EventsRootAfterCancel = DataSource.GetTreeRootForType(EWwiseItemType::Event);
	WWISE_CHECK(EventsRootAfterCancel == BaselineEventsRoot);
}

WWISE_TEST_CASE(BrowserMerge_OrphanUAssetsBuildFoundOnlyInUnrealRoot, "Wwise::AudiokineticTools::BrowserMerge::OrphanUAssetsBuildFoundOnlyInUnrealRoot", "[EditorContext][ProductFilter]")
{
	FWwiseBrowserDataSourceTest DataSource;
	FWaapiDataSourceTest* Waapi = DataSource.SwapInWaapiAccessor_ForTesting();
	FWwiseProjectDatabaseDataSourceTest* ProjectDB = DataSource.SwapInProjectDBAccessor_ForTesting();
	FUAssetDataSourceTest* UAsset = DataSource.SwapInUAssetAccessor_ForTesting();

	TArray<FWwiseTreeItemPtr> PDRootItems;
	PDRootItems.SetNum(static_cast<int32>(EWwiseItemType::LastWwiseBrowserType) + 1);
	ProjectDB->SeedFixture_ForTesting(MoveTemp(PDRootItems), {});

	TArray<FWwiseTreeItemPtr> WaapiRootItems;
	WaapiRootItems.SetNum(static_cast<int32>(EWwiseItemType::LastWwiseBrowserType) + 1);
	Waapi->SeedFixture_ForTesting(MoveTemp(WaapiRootItems), {});
	Waapi->SetConnected_ForTesting();

	UAssetDataSourceInformation Orphan;
	Orphan.Type = EWwiseItemType::Event;
	Orphan.Id.Name = FName(TEXT("OrphanEvent"));
	Orphan.Id.ShortId = 12345;
	Orphan.AssetName = Orphan.Id.Name;
	Orphan.AssetsData.Add(FAssetData());
	UAsset->SeedOrphans_ForTesting({}, { { Orphan.Id.Name, Orphan } });

	WWISE_CHECK(DataSource.MergeDataSources_ForTesting());

	TArray<FWwiseTreeItemPtr>& AllRoots = DataSource.GetRootItemsUnfiltered_ForTesting();
	WWISE_CHECK(AllRoots.Num() > 0);
	if (AllRoots.Num() > 0)
	{
		FWwiseTreeItemPtr FoundOnlyInUnreal = AllRoots.Last();
		WWISE_CHECK(FoundOnlyInUnreal.IsValid());
		if (FoundOnlyInUnreal)
		{
			WWISE_CHECK(FoundOnlyInUnreal->DisplayName == TEXT("Found only in Unreal"));
			WWISE_CHECK(FoundOnlyInUnreal->GetChildren().Num() == 1);
			if (FoundOnlyInUnreal->GetChildren().Num() == 1)
			{
				const FWwiseTreeItemPtr Child = FoundOnlyInUnreal->GetChildren()[0];
				WWISE_CHECK(Child->UAssetName == Orphan.Id.Name);
				WWISE_CHECK(Child->ShortId == Orphan.Id.ShortId);
				WWISE_CHECK(Child->Assets.Num() == 1);
			}
		}
	}
}

// Cancellation landing mid-loop, after Event has already been merged into the loop's local NewRootItems but before Bus is processed and before the final
// swap into RootItemsUnfiltered. 
// OnMergeTypeProcessed_ForTesting lets the test bump RebuildCoordinator's generation deterministically between iterations.
WWISE_TEST_CASE(BrowserMerge_CancelledMidLoopDiscardsPartialSnapshot, "Wwise::AudiokineticTools::BrowserMerge::CancelledMidLoopDiscardsPartialSnapshot", "[EditorContext][ProductFilter]")
{
	FWwiseBrowserDataSourceTest DataSource;
	FWaapiDataSourceTest* Waapi = DataSource.SwapInWaapiAccessor_ForTesting();
	FWwiseProjectDatabaseDataSourceTest* ProjectDB = DataSource.SwapInProjectDBAccessor_ForTesting();

	const FGuid EventsRootGuid = FGuid::NewGuid();
	const FGuid AuxBusRootGuid = FGuid::NewGuid();

	// Baseline: one clean, uncancelled commit with two non-empty types, so RootItemsUnfiltered holds
	// a known snapshot to verify stays untouched below.
	TArray<FWwiseTreeItemPtr> PDRootItems;
	PDRootItems.SetNum(static_cast<int32>(EWwiseItemType::LastWwiseBrowserType) + 1);
	PDRootItems[static_cast<int32>(EWwiseItemType::Event)] = MakeShared<FWwiseTreeItem>(FString(TEXT("Events")), FString(TEXT("\\Events")), nullptr, EWwiseItemType::Event, EventsRootGuid);
	PDRootItems[static_cast<int32>(EWwiseItemType::AuxBus)] = MakeShared<FWwiseTreeItem>(FString(TEXT("Busses")), FString(TEXT("\\Busses")), nullptr, EWwiseItemType::AuxBus, AuxBusRootGuid);
	ProjectDB->SeedFixture_ForTesting(MoveTemp(PDRootItems), {});

	TArray<FWwiseTreeItemPtr> WaapiRootItems;
	WaapiRootItems.SetNum(static_cast<int32>(EWwiseItemType::LastWwiseBrowserType) + 1);
	Waapi->SeedFixture_ForTesting(MoveTemp(WaapiRootItems), {});
	Waapi->SetConnected_ForTesting();

	WWISE_CHECK(DataSource.MergeDataSources_ForTesting());

	FWwiseTreeItemPtr BaselineEventsRoot = DataSource.GetTreeRootForType(EWwiseItemType::Event);
	FWwiseTreeItemPtr BaselineAuxBusRoot = DataSource.GetTreeRootForType(EWwiseItemType::AuxBus);
	WWISE_CHECK(BaselineEventsRoot.IsValid());
	WWISE_CHECK(BaselineAuxBusRoot.IsValid());

	// Cancel right after Event is processed, before AuxBus's iteration runs.
	DataSource.OnMergeTypeProcessed_ForTesting = [&DataSource](EWwiseItemType Type)
	{
		if (Type == EWwiseItemType::Event)
		{
			DataSource.CancelCurrentRebuild();
		}
	};

	WWISE_CHECK_FALSE(DataSource.MergeDataSources_ForTesting(true, 0));

	FWwiseTreeItemPtr EventsRootAfterCancel = DataSource.GetTreeRootForType(EWwiseItemType::Event);
	FWwiseTreeItemPtr AuxBusRootAfterCancel = DataSource.GetTreeRootForType(EWwiseItemType::AuxBus);
	WWISE_CHECK(EventsRootAfterCancel == BaselineEventsRoot);
	WWISE_CHECK(AuxBusRootAfterCancel == BaselineAuxBusRoot);
}

#endif // WWISE_UNIT_TESTS
