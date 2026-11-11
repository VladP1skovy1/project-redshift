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

#include "Wwise/WwiseUnitTests.h"

#if WWISE_UNIT_TESTS

#include "WwiseBrowser/DataSource/WaapiDataSource.h"
#include "WwiseBrowser/DataSource/WwiseBrowserDataSource.h"
#include "WwiseBrowser/DataSource/WwiseProjectDatabaseSource.h"
#include "Wwise/WwiseTreeItem.h"
#include "Wwise/Info/WwiseObjectInfo.h"
#include "AssetRegistry/AssetData.h"

// Shared fixture builders for FWwiseTreeItem/FAssetData test doubles, used by both
// WwiseBrowserFilterTests.cpp and WwiseTreeItemStatusTests.cpp.
namespace WwiseBrowserTestFixtures
{
	inline const TArray<EWwiseItemType> AllWwiseTypes = {
		EWwiseItemType::Event,
		EWwiseItemType::AuxBus,
		EWwiseItemType::Switch,
		EWwiseItemType::State,
		EWwiseItemType::GameParameter,
		EWwiseItemType::DialogueEvent,
		EWwiseItemType::EffectShareSet,
		EWwiseItemType::Trigger,
		EWwiseItemType::AcousticTexture,
		EWwiseItemType::AudioDeviceShareSet,
		EWwiseItemType::ActorMixer,
		EWwiseItemType::Bus,
		EWwiseItemType::Project,
		EWwiseItemType::StandaloneWorkUnit,
		EWwiseItemType::NestedWorkUnit,
		EWwiseItemType::PhysicalFolder,
		EWwiseItemType::Folder,
		EWwiseItemType::Sound,
		EWwiseItemType::SwitchContainer,
		EWwiseItemType::RandomSequenceContainer,
		EWwiseItemType::BlendContainer,
		EWwiseItemType::MotionBus,
		EWwiseItemType::StateGroup,
		EWwiseItemType::SwitchGroup,
		EWwiseItemType::InitBank,
		EWwiseItemType::AudioNode
	};

	inline const TArray<EWwiseItemType> FolderTypes = {
		EWwiseItemType::StandaloneWorkUnit,
		EWwiseItemType::NestedWorkUnit,
		EWwiseItemType::Folder,
		EWwiseItemType::Bus,
		EWwiseItemType::MotionBus,
		EWwiseItemType::PhysicalFolder,
		EWwiseItemType::SwitchContainer,
		EWwiseItemType::SwitchGroup,
		EWwiseItemType::StateGroup,
		EWwiseItemType::RandomSequenceContainer
	};

	inline const TArray<EWwiseItemType> GroupTypes = { 
		EWwiseItemType::Switch,
		EWwiseItemType::State 
	};

	inline FWwiseTreeItemPtr MakeTestItem(const FString& Name, EWwiseItemType Type, const FGuid& Guid = FGuid::NewGuid(), uint32 ShortId = 0)
	{
		return MakeShared<FWwiseTreeItem>(Name, FString(TEXT("\\")) + Name, nullptr, Type, Guid, ShortId);
	}

#if WITH_EDITORONLY_DATA
	// A freshly MakeShared'd WwiseMetadataBasicReference with a real FGuid satisfies WwiseBankRefExists().
	inline void GiveBankRef(const FWwiseTreeItemPtr& Item)
	{
		Item->WwiseItemRef = MakeShared<WwiseMetadataBasicReference>(0, TEXT("RefName"), TEXT("\\RefPath"), WwiseDBGuid(FGuid::NewGuid()));
	}
#endif

	// Builds an FAssetData carrying the exact tag keys/values UAkAudioType::GetAssetRegistryTags writes in
	// production, so IsUAssetOutOfDate() can be made to match or mismatch on demand.
	inline FAssetData MakeTaggedAsset(const FGuid& Guid, uint32 ShortId, const FString& Name)
	{
		FAssetDataTagMap Tags;
		Tags.Add(GET_MEMBER_NAME_CHECKED(FWwiseObjectInfo, WwiseGuid), Guid.ToString());
		Tags.Add(GET_MEMBER_NAME_CHECKED(FWwiseObjectInfo, WwiseShortId), FString::FromInt(static_cast<int32>(ShortId)));
		Tags.Add(GET_MEMBER_NAME_CHECKED(FWwiseObjectInfo, WwiseName), Name);
		return FAssetData(FName(TEXT("/Game/Test/TaggedAsset")), FName(TEXT("/Game/Test")), FName(TEXT("TaggedAsset")), UObject::StaticClass()->GetClassPathName(), Tags);
	}

	// A placeholder asset for predicates that only look at Assets.Num() (UEAssetExists, HasUniqueUAsset/HasMultipleUAssets) and never read TagsAndValues.
	inline FAssetData MakePlainAsset(const FString& AssetName)
	{
		return FAssetData(FName(*(FString(TEXT("/Game/Test/")) + AssetName)), FName(TEXT("/Game/Test")), FName(*AssetName), UObject::StaticClass()->GetClassPathName());
	}
}

class FWaapiDataSourceTest : public FWaapiDataSource
{
public:
	void SeedFixture_ForTesting(TArray<FWwiseTreeItemPtr> InRootItems, TMap<FString, FWwiseTreeItemPtr> InNodesByPath)
	{
		FScopeLock Lock(&WaapiRootItemsLock);
		RootItems = MoveTemp(InRootItems);
		NodesByPath = MoveTemp(InNodesByPath);
	}

	void SetConnected_ForTesting(EWwiseConnectionStatus InStatus = EWwiseConnectionStatus::Connected)
	{
		CachedConnectionStatus = InStatus;
	}

	// Overrides the real network-calling ConstructTree so executing the async rebuild chain via
	// RequestRebuild_ForTesting doesn't clobber seeded fixture data with a live WAAPI query
	virtual bool ConstructTree(bool bShouldRefresh) override
	{
		return true;
	}
};

class FWwiseProjectDatabaseDataSourceTest : public FWwiseProjectDatabaseDataSource
{
public:
	void SeedFixture_ForTesting(TArray<FWwiseTreeItemPtr> InRootItems, TMap<FString, FWwiseTreeItemPtr> InNodesByPath)
	{
		FScopeLock Lock(&TreeDataLock);
		RootItems = MoveTemp(InRootItems);
		NodesByPath = MoveTemp(InNodesByPath);
	}

	// Only needed by tests exercising FindItem's moved/renamed resolution
	void SeedLookupMaps_ForTesting(TMap<FGuid, FWwiseTreeItemPtr> InByGuid, TMultiMap<uint32, FWwiseTreeItemPtr> InByShortId, TMultiMap<FString, FWwiseTreeItemPtr> InByName)
	{
		FScopeLock Lock(&TreeDataLock);
		AllValidTreeItemsByGuid = MoveTemp(InByGuid);
		AllValidTreeItemsByShortId = MoveTemp(InByShortId);
		AllValidTreeItemsByName = MoveTemp(InByName);
	}

	// Builds a FWwiseTreeBuildContext bound to this accessor's own protected containers, for use
	// with the Build*_ForTesting wrappers below.
	FWwiseTreeBuildContext MakeTreeBuildContext_ForTesting()
	{
		RootItems.SetNum(static_cast<int32>(EWwiseItemType::LastWwiseBrowserType) + 1);
		return FWwiseTreeBuildContext{ RootItems, AllValidTreeItemsByGuid, AllValidTreeItemsByShortId, AllValidTreeItemsByName, NodesByPath };
	}

	void BuildEvents_ForTesting(const WwiseEventGlobalIdsMap& Events, FWwiseTreeBuildContext& Ctx) { BuildEvents(Events, Ctx); }
	void BuildDialogueEvents_ForTesting(const WwiseDialogueEventGlobalIdsMap& DialogueEvents, FWwiseTreeBuildContext& Ctx) { BuildDialogueEvents(DialogueEvents, Ctx); }
	void BuildBusses_ForTesting(const WwiseBusGlobalIdsMap& Busses, FWwiseTreeBuildContext& Ctx) { BuildBusses(Busses, Ctx); }
	void BuildAuxBusses_ForTesting(const WwiseAuxBusGlobalIdsMap& AuxBusses, FWwiseTreeBuildContext& Ctx) { BuildAuxBusses(AuxBusses, Ctx); }
	void BuildAcousticTextures_ForTesting(const WwiseAcousticTextureGlobalIdsMap& AcousticTextures, FWwiseTreeBuildContext& Ctx) { BuildAcousticTextures(AcousticTextures, Ctx); }
	void BuildAudioDevices_ForTesting(const WwiseAudioDeviceGlobalIdsMap& AudioDevices, FWwiseTreeBuildContext& Ctx) { BuildAudioDevices(AudioDevices, Ctx); }
	void BuildStateGroups_ForTesting(const WwiseStateGroupGlobalIdsMap& StateGroups, FWwiseTreeBuildContext& Ctx) { BuildStateGroups(StateGroups, Ctx); }
	void BuildStates_ForTesting(const WwiseStateGlobalIdsMap& States, FWwiseTreeBuildContext& Ctx) { BuildStates(States, Ctx); }
	void BuildSwitchGroups_ForTesting(const WwiseSwitchGroupGlobalIdsMap& SwitchGroups, FWwiseTreeBuildContext& Ctx) { BuildSwitchGroups(SwitchGroups, Ctx); }
	void BuildSwitches_ForTesting(const WwiseSwitchGlobalIdsMap& Switches, FWwiseTreeBuildContext& Ctx) { BuildSwitches(Switches, Ctx); }
	void BuildGameParameters_ForTesting(const WwiseGameParameterGlobalIdsMap& GameParameters, FWwiseTreeBuildContext& Ctx) { BuildGameParameters(GameParameters, Ctx); }
	void BuildTriggers_ForTesting(const WwiseTriggerGlobalIdsMap& Triggers, FWwiseTreeBuildContext& Ctx) { BuildTriggers(Triggers, Ctx); }
	void BuildEffectShareSets_ForTesting(const WwisePluginShareSetGlobalIdsMap& EffectShareSets, FWwiseTreeBuildContext& Ctx) { BuildEffectShareSets(EffectShareSets, Ctx); }

	// Keeps RequestRebuild_ForTesting's real async chain from clobbering seeded fixture data with a real FWwiseProjectDatabase::Get() read.
	virtual bool ConstructTree(bool bShouldRefresh) override
	{
		return true;
	}
};

class FUAssetDataSourceTest : public FUAssetDataSource
{
public:
	void SeedOrphans_ForTesting(TMap<TPair<uint32, uint32>, UAssetDataSourceInformation> InUAssetWithoutGuid, TMap<FName, UAssetDataSourceInformation> InUAssetWithoutShortId)
	{
		UAssetWithoutGuid = MoveTemp(InUAssetWithoutGuid);
		UAssetWithoutShortId = MoveTemp(InUAssetWithoutShortId);
	}

	// Overrides the real Asset Registry scan so RequestRebuild_ForTesting's real async chain doesn't clobber
	// SeedOrphans_ForTesting's seeded maps with whatever real assets exist in this process's registry.
	virtual void ConstructItems() override {}
};

class FWwiseBrowserDataSourceTest : public FWwiseBrowserDataSource
{
public:
	FWwiseBrowserDataSourceTest() : FWwiseBrowserDataSource(EForTesting::SkipInit) {}

	// Swaps the real WaapiDataSource for FWaapiDataSourceTest. Returns the derived-typed pointer to seed
	FWaapiDataSourceTest* SwapInWaapiAccessor_ForTesting()
	{
		auto Accessor = MakeUnique<FWaapiDataSourceTest>();
		FWaapiDataSourceTest* Ptr = Accessor.Get();
		WaapiDataSource = MoveTemp(Accessor);
		return Ptr;
	}

	// Swaps the real ProjectDatabaseSource for FProjectDatabaseSourceTest. Returns the derived-typed pointer to seed
	FWwiseProjectDatabaseDataSourceTest* SwapInProjectDBAccessor_ForTesting()
	{
		auto Accessor = MakeUnique<FWwiseProjectDatabaseDataSourceTest>();
		FWwiseProjectDatabaseDataSourceTest* Ptr = Accessor.Get();
		ProjectDBDataSource = MoveTemp(Accessor);
		return Ptr;
	}

	// Swaps the real UAssetDataSource for FUAssetDataSourceTest. Returns the derived-typed pointer to seed
	FUAssetDataSourceTest* SwapInUAssetAccessor_ForTesting()
	{
		auto Accessor = MakeUnique<FUAssetDataSourceTest>();
		FUAssetDataSourceTest* Ptr = Accessor.Get();
		UAssetDataSource = MoveTemp(Accessor);
		return Ptr;
	}

	bool MergeDataSources_ForTesting(bool bCheckCancellation = false, uint32 Generation = 0)
	{
		return MergeDataSources(bCheckCancellation, Generation);
	}

	// Drives the real async rebuild chain (RequestRebuild -> LaunchRebuildChain ->
	// FinishOrRelaunch), the same entry point production code calls.
	void RequestRebuild_ForTesting(bool bRebuildProjectDB, bool bRebuildWaapi)
	{
		RequestRebuild(bRebuildProjectDB, bRebuildWaapi);
	}

	// The "Found only in Unreal" orphan root has no EWwiseItemType slot, so GetTreeRootForType
	// can't retrieve it
	// I's always appended after the per-type loop in MergeDataSources, landing at the last index of RootItemsUnfiltered every merge.
	TArray<FWwiseTreeItemPtr>& GetRootItemsUnfiltered_ForTesting()
	{
		return RootItemsUnfiltered;
	}
};

#endif // WWISE_UNIT_TESTS
