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
#include "Wwise/WwiseTreeItem.h"
#include "Wwise/Info/WwiseObjectInfo.h"
#include "WwiseBrowser/WwiseBrowserForwards.h"

namespace WwiseBrowserFilterTestUtils
{
	void CheckTypeFlagMapsOnlyTo(EWwiseTypeFilter Flag, EWwiseItemType ExpectedType)
	{
		FWwiseTypeFilter Filter;
		Filter.bFilters[Flag] = true;
		for (EWwiseItemType Type : WwiseBrowserTestFixtures::AllWwiseTypes)
		{
			FWwiseTreeItemPtr Item = WwiseBrowserTestFixtures::MakeTestItem(TEXT("Item"), Type);
			WWISE_CHECK(Filter.IsKeptInBrowser(Item) == (Type == ExpectedType));
		}
	}
}

#if WITH_EDITORONLY_DATA
WWISE_TEST_CASE(TreeFilter_SoundBankStatusFilter_NewInWwise, "Wwise::AudiokineticTools::TreeFilter::SoundBankStatusFilter_NewInWwise", "[EditorContext][ProductFilter]")
{
	SECTION("NewInWwise flag keeps only items that are new in Wwise")
	{
		FSoundBankStatusFilter Filter;
		Filter.bFilters[NewInWwise] = true;

		FWwiseTreeItemPtr NewItem = WwiseBrowserTestFixtures::MakeTestItem(TEXT("New"), EWwiseItemType::Event);
		NewItem->bWaapiRefExists = true;

		FWwiseTreeItemPtr UpToDateItem = WwiseBrowserTestFixtures::MakeTestItem(TEXT("UpToDate"), EWwiseItemType::Event);
		WwiseBrowserTestFixtures::GiveBankRef(UpToDateItem);
		UpToDateItem->bWaapiRefExists = true;
		UpToDateItem->WaapiName = UpToDateItem->DisplayName;

		WWISE_CHECK(Filter.IsKeptInBrowser(NewItem));
		WWISE_CHECK_FALSE(Filter.IsKeptInBrowser(UpToDateItem));
	}
}

WWISE_TEST_CASE(TreeFilter_SoundBankStatusFilter_DeletedInWwise, "Wwise::AudiokineticTools::TreeFilter::SoundBankStatusFilter_DeletedInWwise", "[EditorContext][ProductFilter]")
{
	SECTION("DeletedInWwise flag keeps only items deleted in Wwise")
	{
		FSoundBankStatusFilter Filter;
		Filter.bFilters[DeletedInWwise] = true;

		FWwiseTreeItemPtr DeletedItem = WwiseBrowserTestFixtures::MakeTestItem(TEXT("Deleted"), EWwiseItemType::Event);
		WwiseBrowserTestFixtures::GiveBankRef(DeletedItem);

		FWwiseTreeItemPtr UpToDateItem = WwiseBrowserTestFixtures::MakeTestItem(TEXT("UpToDate"), EWwiseItemType::Event);
		WwiseBrowserTestFixtures::GiveBankRef(UpToDateItem);
		UpToDateItem->bWaapiRefExists = true;
		UpToDateItem->WaapiName = UpToDateItem->DisplayName;

		WWISE_CHECK(Filter.IsKeptInBrowser(DeletedItem));
		WWISE_CHECK_FALSE(Filter.IsKeptInBrowser(UpToDateItem));
	}
}

WWISE_TEST_CASE(TreeFilter_SoundBankStatusFilter_NotInWwise, "Wwise::AudiokineticTools::TreeFilter::SoundBankStatusFilter_NotInWwise", "[EditorContext][ProductFilter]")
{
	SECTION("NotInWwise flag keeps only items with neither a Waapi nor a bank ref")
	{
		FSoundBankStatusFilter Filter;
		Filter.bFilters[NotInWwise] = true;

		FWwiseTreeItemPtr NotInWwiseItem = WwiseBrowserTestFixtures::MakeTestItem(TEXT("Neither"), EWwiseItemType::Event);

		FWwiseTreeItemPtr UpToDateItem = WwiseBrowserTestFixtures::MakeTestItem(TEXT("UpToDate"), EWwiseItemType::Event);
		WwiseBrowserTestFixtures::GiveBankRef(UpToDateItem);
		UpToDateItem->bWaapiRefExists = true;
		UpToDateItem->WaapiName = UpToDateItem->DisplayName;

		WWISE_CHECK(Filter.IsKeptInBrowser(NotInWwiseItem));
		WWISE_CHECK_FALSE(Filter.IsKeptInBrowser(UpToDateItem));
	}
}

WWISE_TEST_CASE(TreeFilter_SoundBankStatusFilter_RenamedInWwise, "Wwise::AudiokineticTools::TreeFilter::SoundBankStatusFilter_RenamedInWwise", "[EditorContext][ProductFilter]")
{
	SECTION("RenamedInWwise flag keeps only items renamed in Wwise")
	{
		FSoundBankStatusFilter Filter;
		Filter.bFilters[RenamedInWwise] = true;

		FWwiseTreeItemPtr RenamedItem = WwiseBrowserTestFixtures::MakeTestItem(TEXT("Renamed"), EWwiseItemType::Event);
		WwiseBrowserTestFixtures::GiveBankRef(RenamedItem);
		RenamedItem->bWaapiRefExists = true;
		RenamedItem->WaapiName = TEXT("Different");

		FWwiseTreeItemPtr UpToDateItem = WwiseBrowserTestFixtures::MakeTestItem(TEXT("UpToDate"), EWwiseItemType::Event);
		WwiseBrowserTestFixtures::GiveBankRef(UpToDateItem);
		UpToDateItem->bWaapiRefExists = true;
		UpToDateItem->WaapiName = UpToDateItem->DisplayName;

		WWISE_CHECK(Filter.IsKeptInBrowser(RenamedItem));
		WWISE_CHECK_FALSE(Filter.IsKeptInBrowser(UpToDateItem));
	}
}

WWISE_TEST_CASE(TreeFilter_SoundBankStatusFilter_MovedInWwise, "Wwise::AudiokineticTools::TreeFilter::SoundBankStatusFilter_MovedInWwise", "[EditorContext][ProductFilter]")
{
	SECTION("MovedInWwise flag keeps only items moved in Wwise")
	{
		FSoundBankStatusFilter Filter;
		Filter.bFilters[MovedInWwise] = true;

		FWwiseTreeItemPtr MovedItem = WwiseBrowserTestFixtures::MakeTestItem(TEXT("Moved"), EWwiseItemType::Event);
		WwiseBrowserTestFixtures::GiveBankRef(MovedItem);
		MovedItem->bWaapiRefExists = true;
		MovedItem->WaapiName = MovedItem->DisplayName;
		MovedItem->bSameLocation = false;

		FWwiseTreeItemPtr UpToDateItem = WwiseBrowserTestFixtures::MakeTestItem(TEXT("UpToDate"), EWwiseItemType::Event);
		WwiseBrowserTestFixtures::GiveBankRef(UpToDateItem);
		UpToDateItem->bWaapiRefExists = true;
		UpToDateItem->WaapiName = UpToDateItem->DisplayName;

		WWISE_CHECK(Filter.IsKeptInBrowser(MovedItem));
		WWISE_CHECK_FALSE(Filter.IsKeptInBrowser(UpToDateItem));
	}
}

WWISE_TEST_CASE(TreeFilter_SoundBankStatusFilter_UpToDate, "Wwise::AudiokineticTools::TreeFilter::SoundBankStatusFilter_UpToDate", "[EditorContext][ProductFilter]")
{
	SECTION("UpToDate flag keeps only items that are up to date")
	{
		FSoundBankStatusFilter Filter;
		Filter.bFilters[UpToDate] = true;

		FWwiseTreeItemPtr UpToDateItem = WwiseBrowserTestFixtures::MakeTestItem(TEXT("UpToDate"), EWwiseItemType::Event);
		WwiseBrowserTestFixtures::GiveBankRef(UpToDateItem);
		UpToDateItem->bWaapiRefExists = true;
		UpToDateItem->WaapiName = UpToDateItem->DisplayName;

		FWwiseTreeItemPtr NewItem = WwiseBrowserTestFixtures::MakeTestItem(TEXT("New"), EWwiseItemType::Event);
		NewItem->bWaapiRefExists = true;

		WWISE_CHECK(Filter.IsKeptInBrowser(UpToDateItem));
		WWISE_CHECK_FALSE(Filter.IsKeptInBrowser(NewItem));
	}
}
#endif

#if WITH_EDITORONLY_DATA
WWISE_TEST_CASE(TreeFilter_UAssetStatusFilter_UAssetMissing, "Wwise::AudiokineticTools::TreeFilter::UAssetStatusFilter_UAssetMissing", "[EditorContext][ProductFilter]")
{
	SECTION("UAssetMissing flag keeps only items missing their UAsset")
	{
		const FGuid ItemGuid = FGuid::NewGuid();
		constexpr uint32 ItemShortId = 444;
		const FString ItemName = TEXT("Play_Test");

		FUAssetStatusFilter Filter;
		Filter.bFilters[UAssetMissing] = true;

		FWwiseTreeItemPtr MissingItem = WwiseBrowserTestFixtures::MakeTestItem(ItemName, EWwiseItemType::Event, ItemGuid, ItemShortId);
		WwiseBrowserTestFixtures::GiveBankRef(MissingItem);

		FWwiseTreeItemPtr UpToDateItem = WwiseBrowserTestFixtures::MakeTestItem(ItemName, EWwiseItemType::Event, ItemGuid, ItemShortId);
		WwiseBrowserTestFixtures::GiveBankRef(UpToDateItem);
		UpToDateItem->Assets.Add(WwiseBrowserTestFixtures::MakeTaggedAsset(ItemGuid, ItemShortId, ItemName));
		UpToDateItem->UAssetName = FName(ItemName);
		UpToDateItem->bIsInWrongLocation = false;

		WWISE_CHECK(Filter.IsKeptInBrowser(MissingItem));
		WWISE_CHECK_FALSE(Filter.IsKeptInBrowser(UpToDateItem));
	}
}

WWISE_TEST_CASE(TreeFilter_UAssetStatusFilter_UAssetOrphaned, "Wwise::AudiokineticTools::TreeFilter::UAssetStatusFilter_UAssetOrphaned", "[EditorContext][ProductFilter]")
{
	SECTION("UAssetOrphaned flag keeps only items whose UAsset has no bank ref")
	{
		const FGuid ItemGuid = FGuid::NewGuid();
		constexpr uint32 ItemShortId = 444;
		const FString ItemName = TEXT("Play_Test");

		FUAssetStatusFilter Filter;
		Filter.bFilters[UAssetOrphaned] = true;

		FWwiseTreeItemPtr OrphanedItem = WwiseBrowserTestFixtures::MakeTestItem(ItemName, EWwiseItemType::Event, ItemGuid, ItemShortId);
		OrphanedItem->Assets.Add(WwiseBrowserTestFixtures::MakeTaggedAsset(ItemGuid, ItemShortId, ItemName));

		FWwiseTreeItemPtr UpToDateItem = WwiseBrowserTestFixtures::MakeTestItem(ItemName, EWwiseItemType::Event, ItemGuid, ItemShortId);
		WwiseBrowserTestFixtures::GiveBankRef(UpToDateItem);
		UpToDateItem->Assets.Add(WwiseBrowserTestFixtures::MakeTaggedAsset(ItemGuid, ItemShortId, ItemName));
		UpToDateItem->UAssetName = FName(ItemName);
		UpToDateItem->bIsInWrongLocation = false;

		WWISE_CHECK(Filter.IsKeptInBrowser(OrphanedItem));
		WWISE_CHECK_FALSE(Filter.IsKeptInBrowser(UpToDateItem));
	}
}

WWISE_TEST_CASE(TreeFilter_UAssetStatusFilter_NotInSoundBankOrUnreal, "Wwise::AudiokineticTools::TreeFilter::UAssetStatusFilter_NotInSoundBankOrUnreal", "[EditorContext][ProductFilter]")
{
	SECTION("NotInSoundBankOrUnreal flag keeps only items in neither the SoundBank nor Unreal")
	{
		const FGuid ItemGuid = FGuid::NewGuid();
		constexpr uint32 ItemShortId = 444;
		const FString ItemName = TEXT("Play_Test");

		FUAssetStatusFilter Filter;
		Filter.bFilters[NotInSoundBankOrUnreal] = true;

		FWwiseTreeItemPtr NeitherItem = WwiseBrowserTestFixtures::MakeTestItem(ItemName, EWwiseItemType::Event, ItemGuid, ItemShortId);

		FWwiseTreeItemPtr UpToDateItem = WwiseBrowserTestFixtures::MakeTestItem(ItemName, EWwiseItemType::Event, ItemGuid, ItemShortId);
		WwiseBrowserTestFixtures::GiveBankRef(UpToDateItem);
		UpToDateItem->Assets.Add(WwiseBrowserTestFixtures::MakeTaggedAsset(ItemGuid, ItemShortId, ItemName));
		UpToDateItem->UAssetName = FName(ItemName);
		UpToDateItem->bIsInWrongLocation = false;

		WWISE_CHECK(Filter.IsKeptInBrowser(NeitherItem));
		WWISE_CHECK_FALSE(Filter.IsKeptInBrowser(UpToDateItem));
	}
}

WWISE_TEST_CASE(TreeFilter_UAssetStatusFilter_RenamedInSoundBank, "Wwise::AudiokineticTools::TreeFilter::UAssetStatusFilter_RenamedInSoundBank", "[EditorContext][ProductFilter]")
{
	SECTION("RenamedInSoundBank flag keeps only items renamed in the SoundBank")
	{
		const FGuid ItemGuid = FGuid::NewGuid();
		constexpr uint32 ItemShortId = 444;
		const FString ItemName = TEXT("Play_Test");

		FUAssetStatusFilter Filter;
		Filter.bFilters[RenamedInSoundBank] = true;

		FWwiseTreeItemPtr RenamedItem = WwiseBrowserTestFixtures::MakeTestItem(ItemName, EWwiseItemType::Event, ItemGuid, ItemShortId);
		WwiseBrowserTestFixtures::GiveBankRef(RenamedItem);
		RenamedItem->Assets.Add(WwiseBrowserTestFixtures::MakeTaggedAsset(ItemGuid, ItemShortId, ItemName));
		RenamedItem->UAssetName = FName(TEXT("Old_Name"));

		FWwiseTreeItemPtr UpToDateItem = WwiseBrowserTestFixtures::MakeTestItem(ItemName, EWwiseItemType::Event, ItemGuid, ItemShortId);
		WwiseBrowserTestFixtures::GiveBankRef(UpToDateItem);
		UpToDateItem->Assets.Add(WwiseBrowserTestFixtures::MakeTaggedAsset(ItemGuid, ItemShortId, ItemName));
		UpToDateItem->UAssetName = FName(ItemName);
		UpToDateItem->bIsInWrongLocation = false;

		WWISE_CHECK(Filter.IsKeptInBrowser(RenamedItem));
		WWISE_CHECK_FALSE(Filter.IsKeptInBrowser(UpToDateItem));
	}
}

WWISE_TEST_CASE(TreeFilter_UAssetStatusFilter_MultipleUAssets, "Wwise::AudiokineticTools::TreeFilter::UAssetStatusFilter_MultipleUAssets", "[EditorContext][ProductFilter]")
{
	SECTION("MultipleUAssets flag keeps only items with more than one UAsset")
	{
		const FGuid ItemGuid = FGuid::NewGuid();
		constexpr uint32 ItemShortId = 444;
		const FString ItemName = TEXT("Play_Test");

		FUAssetStatusFilter Filter;
		Filter.bFilters[MultipleUAssets] = true;

		FWwiseTreeItemPtr MultipleItem = WwiseBrowserTestFixtures::MakeTestItem(ItemName, EWwiseItemType::Event, ItemGuid, ItemShortId);
		WwiseBrowserTestFixtures::GiveBankRef(MultipleItem);
		MultipleItem->Assets.Add(WwiseBrowserTestFixtures::MakeTaggedAsset(ItemGuid, ItemShortId, ItemName));
		MultipleItem->Assets.Add(WwiseBrowserTestFixtures::MakeTaggedAsset(ItemGuid, ItemShortId, ItemName));

		FWwiseTreeItemPtr UpToDateItem = WwiseBrowserTestFixtures::MakeTestItem(ItemName, EWwiseItemType::Event, ItemGuid, ItemShortId);
		WwiseBrowserTestFixtures::GiveBankRef(UpToDateItem);
		UpToDateItem->Assets.Add(WwiseBrowserTestFixtures::MakeTaggedAsset(ItemGuid, ItemShortId, ItemName));
		UpToDateItem->UAssetName = FName(ItemName);
		UpToDateItem->bIsInWrongLocation = false;

		WWISE_CHECK(Filter.IsKeptInBrowser(MultipleItem));
		WWISE_CHECK_FALSE(Filter.IsKeptInBrowser(UpToDateItem));
	}
}

WWISE_TEST_CASE(TreeFilter_UAssetStatusFilter_UAssetNeedsUpdate, "Wwise::AudiokineticTools::TreeFilter::UAssetStatusFilter_UAssetNeedsUpdate", "[EditorContext][ProductFilter]")
{
	SECTION("UAssetNeedsUpdate flag keeps only items whose UAsset is out of date")
	{
		const FGuid ItemGuid = FGuid::NewGuid();
		constexpr uint32 ItemShortId = 444;
		const FString ItemName = TEXT("Play_Test");

		FUAssetStatusFilter Filter;
		Filter.bFilters[UAssetNeedsUpdate] = true;

		FWwiseTreeItemPtr OutOfDateItem = WwiseBrowserTestFixtures::MakeTestItem(ItemName, EWwiseItemType::Event, ItemGuid, ItemShortId);
		WwiseBrowserTestFixtures::GiveBankRef(OutOfDateItem);
		OutOfDateItem->Assets.Add(WwiseBrowserTestFixtures::MakeTaggedAsset(FGuid::NewGuid(), ItemShortId, ItemName));
		OutOfDateItem->UAssetName = FName(ItemName);

		FWwiseTreeItemPtr UpToDateItem = WwiseBrowserTestFixtures::MakeTestItem(ItemName, EWwiseItemType::Event, ItemGuid, ItemShortId);
		WwiseBrowserTestFixtures::GiveBankRef(UpToDateItem);
		UpToDateItem->Assets.Add(WwiseBrowserTestFixtures::MakeTaggedAsset(ItemGuid, ItemShortId, ItemName));
		UpToDateItem->UAssetName = FName(ItemName);
		UpToDateItem->bIsInWrongLocation = false;

		WWISE_CHECK(Filter.IsKeptInBrowser(OutOfDateItem));
		WWISE_CHECK_FALSE(Filter.IsKeptInBrowser(UpToDateItem));
	}
}

WWISE_TEST_CASE(TreeFilter_UAssetStatusFilter_UAssetUpToDate, "Wwise::AudiokineticTools::TreeFilter::UAssetStatusFilter_UAssetUpToDate", "[EditorContext][ProductFilter]")
{
	SECTION("UAssetUpToDate flag keeps only items that are up to date")
	{
		const FGuid ItemGuid = FGuid::NewGuid();
		constexpr uint32 ItemShortId = 444;
		const FString ItemName = TEXT("Play_Test");

		FUAssetStatusFilter Filter;
		Filter.bFilters[UAssetUpToDate] = true;

		FWwiseTreeItemPtr UpToDateItem = WwiseBrowserTestFixtures::MakeTestItem(ItemName, EWwiseItemType::Event, ItemGuid, ItemShortId);
		WwiseBrowserTestFixtures::GiveBankRef(UpToDateItem);
		UpToDateItem->Assets.Add(WwiseBrowserTestFixtures::MakeTaggedAsset(ItemGuid, ItemShortId, ItemName));
		UpToDateItem->UAssetName = FName(ItemName);
		UpToDateItem->bIsInWrongLocation = false;

		FWwiseTreeItemPtr MissingItem = WwiseBrowserTestFixtures::MakeTestItem(ItemName, EWwiseItemType::Event, ItemGuid, ItemShortId);
		WwiseBrowserTestFixtures::GiveBankRef(MissingItem);

		WWISE_CHECK(Filter.IsKeptInBrowser(UpToDateItem));
		WWISE_CHECK_FALSE(Filter.IsKeptInBrowser(MissingItem));
	}
}
#endif

WWISE_TEST_CASE(TreeFilter_WwiseTypeFilter_AcousticTexture, "Wwise::AudiokineticTools::TreeFilter::WwiseTypeFilter_AcousticTexture", "[EditorContext][ProductFilter]")
{
	SECTION("AcousticTexture")
	{
		WwiseBrowserFilterTestUtils::CheckTypeFlagMapsOnlyTo(AcousticTexture, EWwiseItemType::AcousticTexture);
	}
}

WWISE_TEST_CASE(TreeFilter_WwiseTypeFilter_AudioDeviceShareSet, "Wwise::AudiokineticTools::TreeFilter::WwiseTypeFilter_AudioDeviceShareSet", "[EditorContext][ProductFilter]")
{
	SECTION("AudioDeviceShareSet")
	{
		WwiseBrowserFilterTestUtils::CheckTypeFlagMapsOnlyTo(AudioDeviceShareSet, EWwiseItemType::AudioDeviceShareSet);
	}
}

WWISE_TEST_CASE(TreeFilter_WwiseTypeFilter_DialogueEvents, "Wwise::AudiokineticTools::TreeFilter::WwiseTypeFilter_DialogueEvents", "[EditorContext][SmokeFilter]")
{
	SECTION("DialogueEvents")
	{
		WwiseBrowserFilterTestUtils::CheckTypeFlagMapsOnlyTo(DialogueEvents, EWwiseItemType::DialogueEvent);
	}
}

WWISE_TEST_CASE(TreeFilter_WwiseTypeFilter_Effects, "Wwise::AudiokineticTools::TreeFilter::WwiseTypeFilter_Effects", "[EditorContext][ProductFilter]")
{
	SECTION("Effects")
	{
		WwiseBrowserFilterTestUtils::CheckTypeFlagMapsOnlyTo(Effects, EWwiseItemType::EffectShareSet);
	}
}

WWISE_TEST_CASE(TreeFilter_WwiseTypeFilter_Events, "Wwise::AudiokineticTools::TreeFilter::WwiseTypeFilter_Events", "[EditorContext][ProductFilter]")
{
	SECTION("Events")
	{
		WwiseBrowserFilterTestUtils::CheckTypeFlagMapsOnlyTo(Events, EWwiseItemType::Event);
	}
}

WWISE_TEST_CASE(TreeFilter_WwiseTypeFilter_GameParameters, "Wwise::AudiokineticTools::TreeFilter::WwiseTypeFilter_GameParameters", "[EditorContext][ProductFilter]")
{
	SECTION("GameParameters")
	{
		WwiseBrowserFilterTestUtils::CheckTypeFlagMapsOnlyTo(GameParameters, EWwiseItemType::GameParameter);
	}
}

WWISE_TEST_CASE(TreeFilter_WwiseTypeFilter_MasterMixerHierarchy, "Wwise::AudiokineticTools::TreeFilter::WwiseTypeFilter_MasterMixerHierarchy", "[EditorContext][ProductFilter]")
{
	SECTION("MasterMixerHierarchy maps to AuxBus, not Bus")
	{
		WwiseBrowserFilterTestUtils::CheckTypeFlagMapsOnlyTo(MasterMixerHierarchy, EWwiseItemType::AuxBus);
	}
}

WWISE_TEST_CASE(TreeFilter_WwiseTypeFilter_State, "Wwise::AudiokineticTools::TreeFilter::WwiseTypeFilter_State", "[EditorContext][ProductFilter]")
{
	SECTION("State")
	{
		WwiseBrowserFilterTestUtils::CheckTypeFlagMapsOnlyTo(State, EWwiseItemType::State);
	}
}

WWISE_TEST_CASE(TreeFilter_WwiseTypeFilter_Switch, "Wwise::AudiokineticTools::TreeFilter::WwiseTypeFilter_Switch", "[EditorContext][ProductFilter]")
{
	SECTION("Switch")
	{
		WwiseBrowserFilterTestUtils::CheckTypeFlagMapsOnlyTo(Switch, EWwiseItemType::Switch);
	}
}

WWISE_TEST_CASE(TreeFilter_WwiseTypeFilter_Trigger, "Wwise::AudiokineticTools::TreeFilter::WwiseTypeFilter_Trigger", "[EditorContext][ProductFilter]")
{
	SECTION("Trigger")
	{
		WwiseBrowserFilterTestUtils::CheckTypeFlagMapsOnlyTo(Trigger, EWwiseItemType::Trigger);
	}
}

WWISE_TEST_CASE(TreeFilter_AreFiltersOff_SoundBankStatusFilter, "Wwise::AudiokineticTools::TreeFilter::AreFiltersOff_SoundBankStatusFilter", "[EditorContext][ProductFilter]")
{
	SECTION("FSoundBankStatusFilter")
	{
		FSoundBankStatusFilter Filter;
		WWISE_CHECK(Filter.AreFiltersOff());
		Filter.bFilters[NewInWwise] = true;
		WWISE_CHECK_FALSE(Filter.AreFiltersOff());
	}
}

WWISE_TEST_CASE(TreeFilter_AreFiltersOff_UAssetStatusFilter, "Wwise::AudiokineticTools::TreeFilter::AreFiltersOff_UAssetStatusFilter", "[EditorContext][ProductFilter]")
{
	SECTION("FUAssetStatusFilter")
	{
		FUAssetStatusFilter Filter;
		WWISE_CHECK(Filter.AreFiltersOff());
		Filter.bFilters[UAssetMissing] = true;
		WWISE_CHECK_FALSE(Filter.AreFiltersOff());
	}
}

WWISE_TEST_CASE(TreeFilter_AreFiltersOff_WwiseTypeFilter, "Wwise::AudiokineticTools::TreeFilter::AreFiltersOff_WwiseTypeFilter", "[EditorContext][ProductFilter]")
{
	SECTION("FWwiseTypeFilter")
	{
		FWwiseTypeFilter Filter;
		WWISE_CHECK(Filter.AreFiltersOff());
		Filter.bFilters[Events] = true;
		WWISE_CHECK_FALSE(Filter.AreFiltersOff());
	}
}

WWISE_TEST_CASE(TreeFilter_AreFiltersOff_BrowserDataSourceAggregatesAllThree, "Wwise::AudiokineticTools::TreeFilter::AreFiltersOff_BrowserDataSourceAggregatesAllThree", "[EditorContext][ProductFilter]")
{
	SECTION("FWwiseBrowserDataSource::AreFiltersOff aggregates all three filters")
	{
		FWwiseBrowserDataSourceTest DataSource;
		WWISE_CHECK(DataSource.AreFiltersOff());

		FSoundBankStatusFilter SoundBankFilter;
		SoundBankFilter.bFilters[UpToDate] = true;
		DataSource.ApplyFilter(SoundBankFilter, FUAssetStatusFilter(), FWwiseTypeFilter());

		WWISE_CHECK_FALSE(DataSource.AreFiltersOff());
	}
}

WWISE_TEST_CASE(TreeFilter_BrowserDataSourceFolderRecursion_SurvivingChild, "Wwise::AudiokineticTools::TreeFilter::BrowserDataSourceFolderRecursion_SurvivingChild", "[EditorContext][ProductFilter]")
{
	SECTION("Folder with one surviving and one non-surviving child is kept")
	{
		FWwiseBrowserDataSourceTest DataSource;
		FWwiseTypeFilter TypeFilter;
		TypeFilter.bFilters[Events] = true;
		DataSource.ApplyFilter(FSoundBankStatusFilter(), FUAssetStatusFilter(), TypeFilter);

		FWwiseTreeItemPtr Folder = WwiseBrowserTestFixtures::MakeTestItem(TEXT("Folder"), EWwiseItemType::Folder);
		FWwiseTreeItemPtr SurvivingChild = WwiseBrowserTestFixtures::MakeTestItem(TEXT("Survivor"), EWwiseItemType::Event);
		FWwiseTreeItemPtr NonSurvivingChild = WwiseBrowserTestFixtures::MakeTestItem(TEXT("NonSurvivor"), EWwiseItemType::Switch);
		Folder->AddChild(SurvivingChild);
		Folder->AddChild(NonSurvivingChild);

		WWISE_CHECK(DataSource.IsKeptInBrowser(Folder));
	}
}

WWISE_TEST_CASE(TreeFilter_BrowserDataSourceFolderRecursion_NoSurvivingChild, "Wwise::AudiokineticTools::TreeFilter::BrowserDataSourceFolderRecursion_NoSurvivingChild", "[EditorContext][ProductFilter]")
{
	SECTION("Folder with no surviving children is dropped")
	{
		FWwiseBrowserDataSourceTest DataSource;
		FWwiseTypeFilter TypeFilter;
		TypeFilter.bFilters[Events] = true;
		DataSource.ApplyFilter(FSoundBankStatusFilter(), FUAssetStatusFilter(), TypeFilter);

		FWwiseTreeItemPtr Folder = WwiseBrowserTestFixtures::MakeTestItem(TEXT("Folder"), EWwiseItemType::Folder);
		FWwiseTreeItemPtr NonSurvivingChildA = WwiseBrowserTestFixtures::MakeTestItem(TEXT("NonSurvivorA"), EWwiseItemType::Switch);
		FWwiseTreeItemPtr NonSurvivingChildB = WwiseBrowserTestFixtures::MakeTestItem(TEXT("NonSurvivorB"), EWwiseItemType::State);
		Folder->AddChild(NonSurvivingChildA);
		Folder->AddChild(NonSurvivingChildB);

		WWISE_CHECK_FALSE(DataSource.IsKeptInBrowser(Folder));
	}
}

WWISE_TEST_CASE(TreeFilter_SoundBankFilterSkipped_WaapiNotConnected, "Wwise::AudiokineticTools::TreeFilter::SoundBankFilterSkipped_WaapiNotConnected", "[EditorContext][ProductFilter]")
{
	SECTION("Item failing the SoundBank filter is still kept when WAAPI is not connected")
	{
		FWwiseBrowserDataSourceTest DataSource;
		FWaapiDataSourceTest* WaapiAccessor = DataSource.SwapInWaapiAccessor_ForTesting();
		WaapiAccessor->SetConnected_ForTesting(WwiseNotOpen);

		FSoundBankStatusFilter SoundBankFilter;
		SoundBankFilter.bFilters[UpToDate] = true;
		DataSource.ApplyFilter(SoundBankFilter, FUAssetStatusFilter(), FWwiseTypeFilter());

		FWwiseTreeItemPtr Item = WwiseBrowserTestFixtures::MakeTestItem(TEXT("NotUpToDate"), EWwiseItemType::Event);

		WWISE_CHECK(DataSource.IsKeptInBrowser(Item));
	}
}

WWISE_TEST_CASE(TreeFilter_SoundBankFilterSkipped_WaapiConnected, "Wwise::AudiokineticTools::TreeFilter::SoundBankFilterSkipped_WaapiConnected", "[EditorContext][ProductFilter]")
{
	SECTION("The same item is filtered out once WAAPI is connected")
	{
		FWwiseBrowserDataSourceTest DataSource;
		FWaapiDataSourceTest* WaapiAccessor = DataSource.SwapInWaapiAccessor_ForTesting();
		WaapiAccessor->SetConnected_ForTesting(Connected);

		FSoundBankStatusFilter SoundBankFilter;
		SoundBankFilter.bFilters[UpToDate] = true;
		DataSource.ApplyFilter(SoundBankFilter, FUAssetStatusFilter(), FWwiseTypeFilter());

		FWwiseTreeItemPtr Item = WwiseBrowserTestFixtures::MakeTestItem(TEXT("NotUpToDate"), EWwiseItemType::Event);

		WWISE_CHECK_FALSE(DataSource.IsKeptInBrowser(Item));
	}
}

WWISE_TEST_CASE(TreeFilter_ApplyFilterDeepCopiesSurvivorsWithoutMutatingInput, "Wwise::AudiokineticTools::TreeFilter::ApplyFilterDeepCopiesSurvivorsWithoutMutatingInput", "[EditorContext][ProductFilter]")
{
	SECTION("Mixed folder/leaf survivors are copied; the input tree is left untouched")
	{
		FWwiseBrowserDataSourceTest DataSource;
		FWwiseTypeFilter TypeFilter;
		TypeFilter.bFilters[Events] = true;
		DataSource.ApplyFilter(FSoundBankStatusFilter(), FUAssetStatusFilter(), TypeFilter);

		FWwiseTreeItemPtr Root = WwiseBrowserTestFixtures::MakeTestItem(TEXT("Root"), EWwiseItemType::Folder);

		FWwiseTreeItemPtr SurvivingLeaf = WwiseBrowserTestFixtures::MakeTestItem(TEXT("SurvivingLeaf"), EWwiseItemType::Event);
		FWwiseTreeItemPtr DroppedLeaf = WwiseBrowserTestFixtures::MakeTestItem(TEXT("DroppedLeaf"), EWwiseItemType::Switch);

		FWwiseTreeItemPtr SubFolder = WwiseBrowserTestFixtures::MakeTestItem(TEXT("SubFolder"), EWwiseItemType::Folder);
		FWwiseTreeItemPtr NestedSurvivor = WwiseBrowserTestFixtures::MakeTestItem(TEXT("NestedSurvivor"), EWwiseItemType::Event);
		SubFolder->AddChild(NestedSurvivor);

		FWwiseTreeItemPtr LeafWithGrandchild = WwiseBrowserTestFixtures::MakeTestItem(TEXT("LeafWithGrandchild"), EWwiseItemType::Event);
		FWwiseTreeItemPtr Grandchild = WwiseBrowserTestFixtures::MakeTestItem(TEXT("Grandchild"), EWwiseItemType::Event);
		LeafWithGrandchild->AddChild(Grandchild);

		Root->AddChild(SurvivingLeaf);
		Root->AddChild(DroppedLeaf);
		Root->AddChild(SubFolder);
		Root->AddChild(LeafWithGrandchild);

		FWwiseTreeItemPtr OutItem;
		DataSource.ApplyFilter(Root, OutItem);

		WWISE_CHECK(OutItem.IsValid());
		WWISE_CHECK(OutItem != Root);
		WWISE_CHECK(OutItem->GetChildren().Num() == 3);
		if (OutItem->GetChildren().Num() == 3)
		{
			FWwiseTreeItemPtr OutSurvivingLeaf = OutItem->GetChildren()[0];
			FWwiseTreeItemPtr OutSubFolder = OutItem->GetChildren()[1];
			FWwiseTreeItemPtr OutLeafWithGrandchild = OutItem->GetChildren()[2];

			WWISE_CHECK(OutSurvivingLeaf->DisplayName == TEXT("SurvivingLeaf"));
			WWISE_CHECK(OutSurvivingLeaf != SurvivingLeaf);

			WWISE_CHECK(OutSubFolder->DisplayName == TEXT("SubFolder"));
			WWISE_CHECK(OutSubFolder != SubFolder);
			WWISE_CHECK(OutSubFolder->GetChildren().Num() == 1);
			if (OutSubFolder->GetChildren().Num() == 1)
			{
				WWISE_CHECK(OutSubFolder->GetChildren()[0]->DisplayName == TEXT("NestedSurvivor"));
				WWISE_CHECK(OutSubFolder->GetChildren()[0] != NestedSurvivor);
			}

			WWISE_CHECK(OutLeafWithGrandchild->DisplayName == TEXT("LeafWithGrandchild"));
			WWISE_CHECK(OutLeafWithGrandchild != LeafWithGrandchild);
			WWISE_CHECK(OutLeafWithGrandchild->GetChildren().Num() == 1);
			if (OutLeafWithGrandchild->GetChildren().Num() == 1)
			{
				WWISE_CHECK(OutLeafWithGrandchild->GetChildren()[0] == Grandchild);
			}
		}

		WWISE_CHECK(Root->GetChildren().Num() == 4);
		if (Root->GetChildren().Num() == 4)
		{
			WWISE_CHECK(Root->GetChildren()[0] == SurvivingLeaf);
			WWISE_CHECK(Root->GetChildren()[1] == DroppedLeaf);
			WWISE_CHECK(Root->GetChildren()[2] == SubFolder);
			WWISE_CHECK(Root->GetChildren()[3] == LeafWithGrandchild);
		}
		WWISE_CHECK(SubFolder->GetChildren().Num() == 1);
		if (SubFolder->GetChildren().Num() == 1)
		{
			WWISE_CHECK(SubFolder->GetChildren()[0] == NestedSurvivor);
		}
		WWISE_CHECK(LeafWithGrandchild->GetChildren().Num() == 1);
		if (LeafWithGrandchild->GetChildren().Num() == 1)
		{
			WWISE_CHECK(LeafWithGrandchild->GetChildren()[0] == Grandchild);
		}
	}
}

#endif // WWISE_UNIT_TESTS