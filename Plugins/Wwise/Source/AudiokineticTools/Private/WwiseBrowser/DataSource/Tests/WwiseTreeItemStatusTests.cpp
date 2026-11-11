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

#include "Wwise/WwiseTreeItem.h"
#include "Wwise/Info/WwiseObjectInfo.h"
#include "WwiseBrowser/WwiseBrowserForwards.h"
#include "WwiseBrowserDataSourceTestAccessors.h"

#if WITH_EDITORONLY_DATA
WWISE_TEST_CASE(TreeItemStatus_NewDeletedMissingUpToDate_NeitherRefExists, "Wwise::AudiokineticTools::TreeItemStatus::NewDeletedMissingUpToDate_NeitherRefExists", "[EditorContext][ProductFilter]")
{
	SECTION("Neither Waapi ref nor bank ref exists")
	{
		FWwiseTreeItemPtr Item = WwiseBrowserTestFixtures::MakeTestItem(TEXT("Item"), EWwiseItemType::Event);

		WWISE_CHECK(Item->IsNotInWwiseOrSoundBank());
		WWISE_CHECK_FALSE(Item->IsNewInWwise());
		WWISE_CHECK_FALSE(Item->IsDeletedInWwise());
		WWISE_CHECK_FALSE(Item->IsSoundBankUpToDate());
	}
}

WWISE_TEST_CASE(TreeItemStatus_NewDeletedMissingUpToDate_WaapiRefOnly, "Wwise::AudiokineticTools::TreeItemStatus::NewDeletedMissingUpToDate_WaapiRefOnly", "[EditorContext][ProductFilter]")
{
	SECTION("Waapi ref exists, no bank ref")
	{
		FWwiseTreeItemPtr Item = WwiseBrowserTestFixtures::MakeTestItem(TEXT("Item"), EWwiseItemType::Event);
		Item->bWaapiRefExists = true;

		WWISE_CHECK(Item->IsNewInWwise());
		WWISE_CHECK_FALSE(Item->IsDeletedInWwise());
		WWISE_CHECK_FALSE(Item->IsNotInWwiseOrSoundBank());
		WWISE_CHECK_FALSE(Item->IsSoundBankUpToDate());
	}
}

WWISE_TEST_CASE(TreeItemStatus_NewDeletedMissingUpToDate_BankRefOnly, "Wwise::AudiokineticTools::TreeItemStatus::NewDeletedMissingUpToDate_BankRefOnly", "[EditorContext][ProductFilter]")
{
	SECTION("Bank ref exists, no Waapi ref")
	{
		FWwiseTreeItemPtr Item = WwiseBrowserTestFixtures::MakeTestItem(TEXT("Item"), EWwiseItemType::Event);
		WwiseBrowserTestFixtures::GiveBankRef(Item);

		WWISE_CHECK(Item->IsDeletedInWwise());
		WWISE_CHECK_FALSE(Item->IsNewInWwise());
		WWISE_CHECK_FALSE(Item->IsNotInWwiseOrSoundBank());
		WWISE_CHECK_FALSE(Item->IsSoundBankUpToDate());
	}
}

WWISE_TEST_CASE(TreeItemStatus_NewDeletedMissingUpToDate_BothRefsExist, "Wwise::AudiokineticTools::TreeItemStatus::NewDeletedMissingUpToDate_BothRefsExist", "[EditorContext][ProductFilter]")
{
	SECTION("Both refs exist, same name and location")
	{
		FWwiseTreeItemPtr Item = WwiseBrowserTestFixtures::MakeTestItem(TEXT("Item"), EWwiseItemType::Event);
		WwiseBrowserTestFixtures::GiveBankRef(Item);
		Item->bWaapiRefExists = true;
		Item->WaapiName = Item->DisplayName;
		Item->bSameLocation = true;

		WWISE_CHECK(Item->IsSoundBankUpToDate());
		WWISE_CHECK_FALSE(Item->IsNewInWwise());
		WWISE_CHECK_FALSE(Item->IsDeletedInWwise());
		WWISE_CHECK_FALSE(Item->IsNotInWwiseOrSoundBank());
	}
}
#endif

#if WITH_EDITORONLY_DATA
WWISE_TEST_CASE(TreeItemStatus_RenamedInWwise_MatchingNames, "Wwise::AudiokineticTools::TreeItemStatus::RenamedInWwise_MatchingNames", "[EditorContext][ProductFilter]")
{
	SECTION("DisplayName matches WaapiName")
	{
		FWwiseTreeItemPtr Item = WwiseBrowserTestFixtures::MakeTestItem(TEXT("Play_Footstep"), EWwiseItemType::Event);
		WwiseBrowserTestFixtures::GiveBankRef(Item);
		Item->bWaapiRefExists = true;
		Item->WaapiName = Item->DisplayName;

		WWISE_CHECK_FALSE(Item->IsRenamedInWwise());
	}
}

WWISE_TEST_CASE(TreeItemStatus_RenamedInWwise_DifferingNames, "Wwise::AudiokineticTools::TreeItemStatus::RenamedInWwise_DifferingNames", "[EditorContext][ProductFilter]")
{
	SECTION("DisplayName differs from WaapiName")
	{
		FWwiseTreeItemPtr Item = WwiseBrowserTestFixtures::MakeTestItem(TEXT("Play_Footstep"), EWwiseItemType::Event);
		WwiseBrowserTestFixtures::GiveBankRef(Item);
		Item->bWaapiRefExists = true;
		Item->WaapiName = TEXT("Play_Jump");

		WWISE_CHECK(Item->IsRenamedInWwise());
	}
}
#endif

#if WITH_EDITORONLY_DATA
WWISE_TEST_CASE(TreeItemStatus_MovedInWwise_SameLocationNotRenamed, "Wwise::AudiokineticTools::TreeItemStatus::MovedInWwise_SameLocationNotRenamed", "[EditorContext][ProductFilter]")
{
	SECTION("Same location, not renamed")
	{
		FWwiseTreeItemPtr Item = WwiseBrowserTestFixtures::MakeTestItem(TEXT("Item"), EWwiseItemType::Event);
		WwiseBrowserTestFixtures::GiveBankRef(Item);
		Item->bWaapiRefExists = true;
		Item->WaapiName = Item->DisplayName;
		Item->bSameLocation = true;

		WWISE_CHECK_FALSE(Item->IsMovedInWwise());
	}
}

WWISE_TEST_CASE(TreeItemStatus_MovedInWwise_SameLocationRenamed, "Wwise::AudiokineticTools::TreeItemStatus::MovedInWwise_SameLocationRenamed", "[EditorContext][ProductFilter]")
{
	SECTION("Same location, renamed")
	{
		FWwiseTreeItemPtr Item = WwiseBrowserTestFixtures::MakeTestItem(TEXT("Item"), EWwiseItemType::Event);
		WwiseBrowserTestFixtures::GiveBankRef(Item);
		Item->bWaapiRefExists = true;
		Item->WaapiName = TEXT("Different");
		Item->bSameLocation = true;

		WWISE_CHECK_FALSE(Item->IsMovedInWwise());
	}
}

WWISE_TEST_CASE(TreeItemStatus_MovedInWwise_DifferentLocationRenamed, "Wwise::AudiokineticTools::TreeItemStatus::MovedInWwise_DifferentLocationRenamed", "[EditorContext][ProductFilter]")
{
	SECTION("Different location, renamed")
	{
		FWwiseTreeItemPtr Item = WwiseBrowserTestFixtures::MakeTestItem(TEXT("Item"), EWwiseItemType::Event);
		WwiseBrowserTestFixtures::GiveBankRef(Item);
		Item->bWaapiRefExists = true;
		Item->WaapiName = TEXT("Different");
		Item->bSameLocation = false;

		WWISE_CHECK_FALSE(Item->IsMovedInWwise());
	}
}

WWISE_TEST_CASE(TreeItemStatus_MovedInWwise_DifferentLocationNotRenamed, "Wwise::AudiokineticTools::TreeItemStatus::MovedInWwise_DifferentLocationNotRenamed", "[EditorContext][ProductFilter]")
{
	SECTION("Different location, not renamed")
	{
		FWwiseTreeItemPtr Item = WwiseBrowserTestFixtures::MakeTestItem(TEXT("Item"), EWwiseItemType::Event);
		WwiseBrowserTestFixtures::GiveBankRef(Item);
		Item->bWaapiRefExists = true;
		Item->WaapiName = Item->DisplayName;
		Item->bSameLocation = false;

		WWISE_CHECK(Item->IsMovedInWwise());
	}
}
#endif

#if WITH_EDITORONLY_DATA
WWISE_TEST_CASE(TreeItemStatus_MissingOrphanedNotInSoundBankOrUnreal_NoBankRefNoAsset, "Wwise::AudiokineticTools::TreeItemStatus::MissingOrphanedNotInSoundBankOrUnreal_NoBankRefNoAsset", "[EditorContext][ProductFilter]")
{
	SECTION("No bank ref, no UAsset")
	{
		FWwiseTreeItemPtr Item = WwiseBrowserTestFixtures::MakeTestItem(TEXT("Item"), EWwiseItemType::Event);

		WWISE_CHECK(Item->IsNotInSoundBankOrUnreal());
		WWISE_CHECK_FALSE(Item->IsUAssetMissing());
		WWISE_CHECK_FALSE(Item->IsUAssetOrphaned());
	}
}

WWISE_TEST_CASE(TreeItemStatus_MissingOrphanedNotInSoundBankOrUnreal_NoBankRefWithAsset, "Wwise::AudiokineticTools::TreeItemStatus::MissingOrphanedNotInSoundBankOrUnreal_NoBankRefWithAsset", "[EditorContext][ProductFilter]")
{
	SECTION("No bank ref, has a UAsset : orphaned")
	{
		FWwiseTreeItemPtr Item = WwiseBrowserTestFixtures::MakeTestItem(TEXT("Item"), EWwiseItemType::Event);
		Item->Assets.Add(WwiseBrowserTestFixtures::MakePlainAsset(TEXT("Item")));

		WWISE_CHECK(Item->IsUAssetOrphaned());
		WWISE_CHECK_FALSE(Item->IsUAssetMissing());
		WWISE_CHECK_FALSE(Item->IsNotInSoundBankOrUnreal());
	}
}

WWISE_TEST_CASE(TreeItemStatus_MissingOrphanedNotInSoundBankOrUnreal_BankRefNoAsset, "Wwise::AudiokineticTools::TreeItemStatus::MissingOrphanedNotInSoundBankOrUnreal_BankRefNoAsset", "[EditorContext][ProductFilter]")
{
	SECTION("Bank ref exists, no UAsset : missing")
	{
		FWwiseTreeItemPtr Item = WwiseBrowserTestFixtures::MakeTestItem(TEXT("Item"), EWwiseItemType::Event);
		WwiseBrowserTestFixtures::GiveBankRef(Item);

		WWISE_CHECK(Item->IsUAssetMissing());
		WWISE_CHECK_FALSE(Item->IsUAssetOrphaned());
		WWISE_CHECK_FALSE(Item->IsNotInSoundBankOrUnreal());
	}
}

WWISE_TEST_CASE(TreeItemStatus_MissingOrphanedNotInSoundBankOrUnreal_BankRefWithAsset, "Wwise::AudiokineticTools::TreeItemStatus::MissingOrphanedNotInSoundBankOrUnreal_BankRefWithAsset", "[EditorContext][ProductFilter]")
{
	SECTION("Bank ref exists and has a UAsset : none of the three")
	{
		FWwiseTreeItemPtr Item = WwiseBrowserTestFixtures::MakeTestItem(TEXT("Item"), EWwiseItemType::Event);
		WwiseBrowserTestFixtures::GiveBankRef(Item);
		Item->Assets.Add(WwiseBrowserTestFixtures::MakePlainAsset(TEXT("Item")));

		WWISE_CHECK_FALSE(Item->IsUAssetMissing());
		WWISE_CHECK_FALSE(Item->IsUAssetOrphaned());
		WWISE_CHECK_FALSE(Item->IsNotInSoundBankOrUnreal());
	}
}
#endif

WWISE_TEST_CASE(TreeItemStatus_UniqueAndMultipleUAssets_ZeroAssets, "Wwise::AudiokineticTools::TreeItemStatus::UniqueAndMultipleUAssets_ZeroAssets", "[EditorContext][ProductFilter]")
{
	SECTION("Zero assets")
	{
		FWwiseTreeItemPtr Item = WwiseBrowserTestFixtures::MakeTestItem(TEXT("Item"), EWwiseItemType::Event);

		WWISE_CHECK_FALSE(Item->HasUniqueUAsset());
		WWISE_CHECK_FALSE(Item->HasMultipleUAssets());
	}
}

WWISE_TEST_CASE(TreeItemStatus_UniqueAndMultipleUAssets_OneAsset, "Wwise::AudiokineticTools::TreeItemStatus::UniqueAndMultipleUAssets_OneAsset", "[EditorContext][ProductFilter]")
{
	SECTION("One asset")
	{
		FWwiseTreeItemPtr Item = WwiseBrowserTestFixtures::MakeTestItem(TEXT("Item"), EWwiseItemType::Event);
		Item->Assets.Add(WwiseBrowserTestFixtures::MakePlainAsset(TEXT("Item")));

		WWISE_CHECK(Item->HasUniqueUAsset());
		WWISE_CHECK_FALSE(Item->HasMultipleUAssets());
	}
}

WWISE_TEST_CASE(TreeItemStatus_UniqueAndMultipleUAssets_TwoAssets, "Wwise::AudiokineticTools::TreeItemStatus::UniqueAndMultipleUAssets_TwoAssets", "[EditorContext][ProductFilter]")
{
	SECTION("Two assets")
	{
		FWwiseTreeItemPtr Item = WwiseBrowserTestFixtures::MakeTestItem(TEXT("Item"), EWwiseItemType::Event);
		Item->Assets.Add(WwiseBrowserTestFixtures::MakePlainAsset(TEXT("Item_A")));
		Item->Assets.Add(WwiseBrowserTestFixtures::MakePlainAsset(TEXT("Item_B")));

		WWISE_CHECK_FALSE(Item->HasUniqueUAsset());
		WWISE_CHECK(Item->HasMultipleUAssets());
	}
}

#if WITH_EDITORONLY_DATA
WWISE_TEST_CASE(TreeItemStatus_RenamedInSoundBank_NonGroupItem_MatchingName, "Wwise::AudiokineticTools::TreeItemStatus::RenamedInSoundBank_NonGroupItem_MatchingName", "[EditorContext][ProductFilter]")
{
	SECTION("UAssetName matches DisplayName")
	{
		FWwiseTreeItemPtr Item = WwiseBrowserTestFixtures::MakeTestItem(TEXT("Play_Footstep"), EWwiseItemType::Event);
		WwiseBrowserTestFixtures::GiveBankRef(Item);
		Item->Assets.Add(WwiseBrowserTestFixtures::MakePlainAsset(TEXT("Play_Footstep")));
		Item->UAssetName = FName(Item->DisplayName);

		WWISE_CHECK_FALSE(Item->IsInGroup());
		WWISE_CHECK_FALSE(Item->IsRenamedInSoundBank());
	}
}

WWISE_TEST_CASE(TreeItemStatus_RenamedInSoundBank_NonGroupItem_DifferingName, "Wwise::AudiokineticTools::TreeItemStatus::RenamedInSoundBank_NonGroupItem_DifferingName", "[EditorContext][ProductFilter]")
{
	SECTION("UAssetName differs from DisplayName")
	{
		FWwiseTreeItemPtr Item = WwiseBrowserTestFixtures::MakeTestItem(TEXT("Play_Footstep"), EWwiseItemType::Event);
		WwiseBrowserTestFixtures::GiveBankRef(Item);
		Item->Assets.Add(WwiseBrowserTestFixtures::MakePlainAsset(TEXT("Play_Footstep_Old")));
		Item->UAssetName = FName(TEXT("Play_Footstep_Old"));

		WWISE_CHECK_FALSE(Item->IsInGroup());
		WWISE_CHECK(Item->IsRenamedInSoundBank());
	}
}
#endif

#if WITH_EDITORONLY_DATA
WWISE_TEST_CASE(TreeItemStatus_RenamedInSoundBank_GroupedItem_MatchingName, "Wwise::AudiokineticTools::TreeItemStatus::RenamedInSoundBank_GroupedItem_MatchingName", "[EditorContext][ProductFilter]")
{
	SECTION("UAssetName matches the parent-composed switch asset name")
	{
		FWwiseTreeItemPtr SwitchGroup = WwiseBrowserTestFixtures::MakeTestItem(TEXT("Surface"), EWwiseItemType::SwitchGroup);
		FWwiseTreeItemPtr Item = WwiseBrowserTestFixtures::MakeTestItem(TEXT("Grass"), EWwiseItemType::Switch);
		SwitchGroup->AddChild(Item);
		WwiseBrowserTestFixtures::GiveBankRef(Item);
		Item->Assets.Add(WwiseBrowserTestFixtures::MakePlainAsset(TEXT("Surface-Grass")));
		Item->UAssetName = FName(Item->GetSwitchAssetName());

		WWISE_CHECK(Item->IsInGroup());
		WWISE_CHECK(Item->GetSwitchAssetName() == TEXT("Surface-Grass"));
		WWISE_CHECK_FALSE(Item->IsRenamedInSoundBank());
	}
}

WWISE_TEST_CASE(TreeItemStatus_RenamedInSoundBank_GroupedItem_DifferingName, "Wwise::AudiokineticTools::TreeItemStatus::RenamedInSoundBank_GroupedItem_DifferingName", "[EditorContext][ProductFilter]")
{
	SECTION("UAssetName differs from the parent-composed switch asset name")
	{
		FWwiseTreeItemPtr SwitchGroup = WwiseBrowserTestFixtures::MakeTestItem(TEXT("Surface"), EWwiseItemType::SwitchGroup);
		FWwiseTreeItemPtr Item = WwiseBrowserTestFixtures::MakeTestItem(TEXT("Grass"), EWwiseItemType::Switch);
		SwitchGroup->AddChild(Item);
		WwiseBrowserTestFixtures::GiveBankRef(Item);
		Item->Assets.Add(WwiseBrowserTestFixtures::MakePlainAsset(TEXT("Surface-Grass_Old")));
		Item->UAssetName = FName(TEXT("Surface-Grass_Old"));

		WWISE_CHECK(Item->IsInGroup());
		WWISE_CHECK(Item->IsRenamedInSoundBank());
	}
}
#endif

WWISE_TEST_CASE(TreeItemStatus_UAssetOutOfDate_AllMatch, "Wwise::AudiokineticTools::TreeItemStatus::UAssetOutOfDate_AllMatch", "[EditorContext][ProductFilter]")
{
	SECTION("Guid, ShortId and Name all match : up to date")
	{
		const FGuid ItemGuid = FGuid::NewGuid();
		const uint32 ItemShortId = 555;
		const FString ItemName = TEXT("Play_Footstep");

		FWwiseTreeItemPtr Item = WwiseBrowserTestFixtures::MakeTestItem(ItemName, EWwiseItemType::Event, ItemGuid, ItemShortId);
		Item->Assets.Add(WwiseBrowserTestFixtures::MakeTaggedAsset(ItemGuid, ItemShortId, ItemName));

		WWISE_CHECK_FALSE(Item->IsUAssetOutOfDate());
	}
}

WWISE_TEST_CASE(TreeItemStatus_UAssetOutOfDate_GuidMismatch, "Wwise::AudiokineticTools::TreeItemStatus::UAssetOutOfDate_GuidMismatch", "[EditorContext][ProductFilter]")
{
	SECTION("Guid mismatches : out of date")
	{
		const FGuid ItemGuid = FGuid::NewGuid();
		const uint32 ItemShortId = 555;
		const FString ItemName = TEXT("Play_Footstep");

		FWwiseTreeItemPtr Item = WwiseBrowserTestFixtures::MakeTestItem(ItemName, EWwiseItemType::Event, ItemGuid, ItemShortId);
		Item->Assets.Add(WwiseBrowserTestFixtures::MakeTaggedAsset(FGuid::NewGuid(), ItemShortId, ItemName));

		WWISE_CHECK(Item->IsUAssetOutOfDate());
	}
}

WWISE_TEST_CASE(TreeItemStatus_UAssetOutOfDate_ShortIdMismatch, "Wwise::AudiokineticTools::TreeItemStatus::UAssetOutOfDate_ShortIdMismatch", "[EditorContext][ProductFilter]")
{
	SECTION("ShortId mismatches : out of date")
	{
		const FGuid ItemGuid = FGuid::NewGuid();
		const uint32 ItemShortId = 555;
		const FString ItemName = TEXT("Play_Footstep");

		FWwiseTreeItemPtr Item = WwiseBrowserTestFixtures::MakeTestItem(ItemName, EWwiseItemType::Event, ItemGuid, ItemShortId);
		Item->Assets.Add(WwiseBrowserTestFixtures::MakeTaggedAsset(ItemGuid, ItemShortId + 1, ItemName));

		WWISE_CHECK(Item->IsUAssetOutOfDate());
	}
}

WWISE_TEST_CASE(TreeItemStatus_UAssetOutOfDate_NameMismatch, "Wwise::AudiokineticTools::TreeItemStatus::UAssetOutOfDate_NameMismatch", "[EditorContext][ProductFilter]")
{
	SECTION("Name mismatches : out of date")
	{
		const FGuid ItemGuid = FGuid::NewGuid();
		const uint32 ItemShortId = 555;
		const FString ItemName = TEXT("Play_Footstep");

		FWwiseTreeItemPtr Item = WwiseBrowserTestFixtures::MakeTestItem(ItemName, EWwiseItemType::Event, ItemGuid, ItemShortId);
		Item->Assets.Add(WwiseBrowserTestFixtures::MakeTaggedAsset(ItemGuid, ItemShortId, TEXT("Play_Footstep_Old")));

		WWISE_CHECK(Item->IsUAssetOutOfDate());
	}
}

#if WITH_EDITORONLY_DATA
WWISE_TEST_CASE(TreeItemStatus_UAssetUpToDate_AllSatisfied, "Wwise::AudiokineticTools::TreeItemStatus::UAssetUpToDate_AllSatisfied", "[EditorContext][ProductFilter]")
{
	SECTION("All conditions satisfied : up to date")
	{
		const FGuid ItemGuid = FGuid::NewGuid();
		const uint32 ItemShortId = 777;
		const FString ItemName = TEXT("Play_Land");

		FWwiseTreeItemPtr Item = WwiseBrowserTestFixtures::MakeTestItem(ItemName, EWwiseItemType::Event, ItemGuid, ItemShortId);
		WwiseBrowserTestFixtures::GiveBankRef(Item);
		Item->Assets.Add(WwiseBrowserTestFixtures::MakeTaggedAsset(ItemGuid, ItemShortId, ItemName));
		Item->UAssetName = FName(ItemName);
		Item->bIsInWrongLocation = false;

		WWISE_CHECK(Item->IsUAssetUpToDate());
	}
}

WWISE_TEST_CASE(TreeItemStatus_UAssetUpToDate_NoBankRef, "Wwise::AudiokineticTools::TreeItemStatus::UAssetUpToDate_NoBankRef", "[EditorContext][ProductFilter]")
{
	SECTION("No bank ref : not up to date")
	{
		const FGuid ItemGuid = FGuid::NewGuid();
		constexpr uint32 ItemShortId = 777;
		const FString ItemName = TEXT("Play_Land");

		FWwiseTreeItemPtr Item = WwiseBrowserTestFixtures::MakeTestItem(ItemName, EWwiseItemType::Event, ItemGuid, ItemShortId);
		Item->Assets.Add(WwiseBrowserTestFixtures::MakeTaggedAsset(ItemGuid, ItemShortId, ItemName));
		Item->UAssetName = FName(ItemName);

		WWISE_CHECK_FALSE(Item->IsUAssetUpToDate());
	}
}

WWISE_TEST_CASE(TreeItemStatus_UAssetUpToDate_NoUAsset, "Wwise::AudiokineticTools::TreeItemStatus::UAssetUpToDate_NoUAsset", "[EditorContext][ProductFilter]")
{
	SECTION("No UAsset (not unique) : not up to date")
	{
		const FGuid ItemGuid = FGuid::NewGuid();
		const uint32 ItemShortId = 777;
		const FString ItemName = TEXT("Play_Land");

		FWwiseTreeItemPtr Item = WwiseBrowserTestFixtures::MakeTestItem(ItemName, EWwiseItemType::Event, ItemGuid, ItemShortId);
		WwiseBrowserTestFixtures::GiveBankRef(Item);
		Item->UAssetName = FName(ItemName);

		WWISE_CHECK_FALSE(Item->IsUAssetUpToDate());
	}
}

WWISE_TEST_CASE(TreeItemStatus_UAssetUpToDate_MultipleUAssets, "Wwise::AudiokineticTools::TreeItemStatus::UAssetUpToDate_MultipleUAssets", "[EditorContext][ProductFilter]")
{
	SECTION("Multiple UAssets (not unique) : not up to date")
	{
		const FGuid ItemGuid = FGuid::NewGuid();
		const uint32 ItemShortId = 777;
		const FString ItemName = TEXT("Play_Land");

		FWwiseTreeItemPtr Item = WwiseBrowserTestFixtures::MakeTestItem(ItemName, EWwiseItemType::Event, ItemGuid, ItemShortId);
		WwiseBrowserTestFixtures::GiveBankRef(Item);
		Item->Assets.Add(WwiseBrowserTestFixtures::MakeTaggedAsset(ItemGuid, ItemShortId, ItemName));
		Item->UAssetName = FName(ItemName);
		Item->bIsInWrongLocation = false;

		Item->Assets.Add(WwiseBrowserTestFixtures::MakeTaggedAsset(FGuid::NewGuid(), ItemShortId + 1, TEXT("Other")));

		WWISE_CHECK_FALSE(Item->IsUAssetUpToDate());
	}
}

WWISE_TEST_CASE(TreeItemStatus_UAssetUpToDate_RenamedInSoundBank, "Wwise::AudiokineticTools::TreeItemStatus::UAssetUpToDate_RenamedInSoundBank", "[EditorContext][ProductFilter]")
{
	SECTION("Renamed in SoundBank : not up to date")
	{
		const FGuid ItemGuid = FGuid::NewGuid();
		const uint32 ItemShortId = 777;
		const FString ItemName = TEXT("Play_Land");

		FWwiseTreeItemPtr Item = WwiseBrowserTestFixtures::MakeTestItem(ItemName, EWwiseItemType::Event, ItemGuid, ItemShortId);
		WwiseBrowserTestFixtures::GiveBankRef(Item);
		Item->Assets.Add(WwiseBrowserTestFixtures::MakeTaggedAsset(ItemGuid, ItemShortId, ItemName));
		Item->UAssetName = FName(TEXT("Play_Land_Old"));
		Item->bIsInWrongLocation = false;

		WWISE_CHECK_FALSE(Item->IsUAssetUpToDate());
	}
}

WWISE_TEST_CASE(TreeItemStatus_UAssetUpToDate_UAssetOutOfDate, "Wwise::AudiokineticTools::TreeItemStatus::UAssetUpToDate_UAssetOutOfDate", "[EditorContext][ProductFilter]")
{
	SECTION("UAsset out of date : not up to date")
	{
		const FGuid ItemGuid = FGuid::NewGuid();
		const uint32 ItemShortId = 777;
		const FString ItemName = TEXT("Play_Land");

		FWwiseTreeItemPtr Item = WwiseBrowserTestFixtures::MakeTestItem(ItemName, EWwiseItemType::Event, ItemGuid, ItemShortId);
		WwiseBrowserTestFixtures::GiveBankRef(Item);
		Item->Assets.Add(WwiseBrowserTestFixtures::MakeTaggedAsset(FGuid::NewGuid(), ItemShortId, ItemName));
		Item->UAssetName = FName(ItemName);

		WWISE_CHECK_FALSE(Item->IsUAssetUpToDate());
	}
}

WWISE_TEST_CASE(TreeItemStatus_UAssetUpToDate_InWrongLocation, "Wwise::AudiokineticTools::TreeItemStatus::UAssetUpToDate_InWrongLocation", "[EditorContext][ProductFilter]")
{
	SECTION("In wrong location : not up to date")
	{
		const FGuid ItemGuid = FGuid::NewGuid();
		const uint32 ItemShortId = 777;
		const FString ItemName = TEXT("Play_Land");

		FWwiseTreeItemPtr Item = WwiseBrowserTestFixtures::MakeTestItem(ItemName, EWwiseItemType::Event, ItemGuid, ItemShortId);
		WwiseBrowserTestFixtures::GiveBankRef(Item);
		Item->Assets.Add(WwiseBrowserTestFixtures::MakeTaggedAsset(ItemGuid, ItemShortId, ItemName));
		Item->UAssetName = FName(ItemName);
		Item->bIsInWrongLocation = true;

		WWISE_CHECK_FALSE(Item->IsUAssetUpToDate());
	}
}
#endif

WWISE_TEST_CASE(TreeItemStatus_TypeClassificationForEveryItemType, "Wwise::AudiokineticTools::TreeItemStatus::TypeClassificationForEveryItemType", "[EditorContext][ProductFilter]")
{
	SECTION("Classify folder, aux bus, group, and info display across all item types")
	{
		for (EWwiseItemType Type : WwiseBrowserTestFixtures::AllWwiseTypes)
		{
			FWwiseTreeItemPtr Item = WwiseBrowserTestFixtures::MakeTestItem(TEXT("Item"), Type);

			const bool bExpectedFolder = WwiseBrowserTestFixtures::FolderTypes.Contains(Type);
			const bool bExpectedAuxBus = (Type == EWwiseItemType::AuxBus);
			const bool bExpectedInGroup = WwiseBrowserTestFixtures::GroupTypes.Contains(Type);

			WWISE_CHECK(Item->IsFolder() == bExpectedFolder);
			WWISE_CHECK(Item->IsAuxBus() == bExpectedAuxBus);
			WWISE_CHECK(Item->IsInGroup() == bExpectedInGroup);
			WWISE_CHECK(Item->ShouldDisplayInfo() == !bExpectedFolder);
		}
	}
}

WWISE_TEST_CASE(TreeItemStatus_SortChildrenOrdering, "Wwise::AudiokineticTools::TreeItemStatus::SortChildrenOrdering", "[EditorContext][ProductFilter]")
{
	SECTION("Sort children with type hierarchy and natural string comparison")
	{
		FWwiseTreeItemPtr Root = WwiseBrowserTestFixtures::MakeTestItem(TEXT("Root"), EWwiseItemType::Event);

		FWwiseTreeItemPtr Item10 = WwiseBrowserTestFixtures::MakeTestItem(TEXT("Item10"), EWwiseItemType::Event);
		FWwiseTreeItemPtr VFolder = WwiseBrowserTestFixtures::MakeTestItem(TEXT("VFolder"), EWwiseItemType::Folder);
		FWwiseTreeItemPtr Item1 = WwiseBrowserTestFixtures::MakeTestItem(TEXT("Item1"), EWwiseItemType::Event);
		FWwiseTreeItemPtr WorkUnit = WwiseBrowserTestFixtures::MakeTestItem(TEXT("WorkUnit"), EWwiseItemType::NestedWorkUnit);
		FWwiseTreeItemPtr Item2 = WwiseBrowserTestFixtures::MakeTestItem(TEXT("Item2"), EWwiseItemType::Event);
		FWwiseTreeItemPtr PFolder = WwiseBrowserTestFixtures::MakeTestItem(TEXT("PFolder"), EWwiseItemType::PhysicalFolder);

		Root->AddChild(Item10);
		Root->AddChild(VFolder);
		Root->AddChild(Item1);
		Root->AddChild(WorkUnit);
		Root->AddChild(Item2);
		Root->AddChild(PFolder);

		Root->SortChildren();

		const TArray<FWwiseTreeItemPtr>& Children = Root->GetChildren();
		WWISE_CHECK(Children.Num() == 6);
		if (Children.Num() == 6)
		{
			WWISE_CHECK(Children[0] == PFolder);
			WWISE_CHECK(Children[1] == WorkUnit);
			WWISE_CHECK(Children[2] == VFolder);
			WWISE_CHECK(Children[3] == Item1);
			WWISE_CHECK(Children[4] == Item2);
			WWISE_CHECK(Children[5] == Item10);
		}
	}
}

WWISE_TEST_CASE(TreeItemStatus_FindItemRecursive_ExactGuidMatch, "Wwise::AudiokineticTools::TreeItemStatus::FindItemRecursive_ExactGuidMatch", "[EditorContext][ProductFilter]")
{
	SECTION("Exact Guid match is found even though ShortId and DisplayName both mismatch")
	{
		FWwiseTreeItemPtr Root = WwiseBrowserTestFixtures::MakeTestItem(TEXT("Root"), EWwiseItemType::Folder);
		const FGuid TargetGuid = FGuid::NewGuid();
		FWwiseTreeItemPtr Target = WwiseBrowserTestFixtures::MakeTestItem(TEXT("Foo"), EWwiseItemType::Event, TargetGuid, 5);
		Root->AddChild(Target);

		FWwiseTreeItemPtr Query = WwiseBrowserTestFixtures::MakeTestItem(TEXT("Bar"), EWwiseItemType::Event, TargetGuid, 999);

		FWwiseTreeItemPtr Found = Root->FindItemRecursive(Query);
		WWISE_CHECK(Found == Target);
	}
}

WWISE_TEST_CASE(TreeItemStatus_FindItemRecursive_ShortIdMatch, "Wwise::AudiokineticTools::TreeItemStatus::FindItemRecursive_ShortIdMatch", "[EditorContext][ProductFilter]")
{
	SECTION("ShortId match is found when Guid mismatches, only if ShortId > 0")
	{
		FWwiseTreeItemPtr Root = WwiseBrowserTestFixtures::MakeTestItem(TEXT("Root"), EWwiseItemType::Folder);
		FWwiseTreeItemPtr Target = WwiseBrowserTestFixtures::MakeTestItem(TEXT("Foo"), EWwiseItemType::Event, FGuid::NewGuid(), 5);
		Root->AddChild(Target);

		FWwiseTreeItemPtr Query = WwiseBrowserTestFixtures::MakeTestItem(TEXT("Bar"), EWwiseItemType::Event, FGuid::NewGuid(), 5);

		FWwiseTreeItemPtr Found = Root->FindItemRecursive(Query);
		WWISE_CHECK(Found == Target);
	}
}

WWISE_TEST_CASE(TreeItemStatus_FindItemRecursive_ShortIdZero, "Wwise::AudiokineticTools::TreeItemStatus::FindItemRecursive_ShortIdZero", "[EditorContext][ProductFilter]")
{
	SECTION("ShortId of 0 never matches, even when equal")
	{
		FWwiseTreeItemPtr Root = WwiseBrowserTestFixtures::MakeTestItem(TEXT("Root"), EWwiseItemType::Folder);
		FWwiseTreeItemPtr Target = WwiseBrowserTestFixtures::MakeTestItem(TEXT("Foo"), EWwiseItemType::Event, FGuid::NewGuid(), 0);
		Root->AddChild(Target);

		FWwiseTreeItemPtr Query = WwiseBrowserTestFixtures::MakeTestItem(TEXT("Bar"), EWwiseItemType::Event, FGuid::NewGuid(), 0);

		FWwiseTreeItemPtr Found = Root->FindItemRecursive(Query);
		WWISE_CHECK_FALSE(Found.IsValid());
	}
}

WWISE_TEST_CASE(TreeItemStatus_FindItemRecursive_DisplayNameMatch, "Wwise::AudiokineticTools::TreeItemStatus::FindItemRecursive_DisplayNameMatch", "[EditorContext][ProductFilter]")
{
	SECTION("DisplayName match is found only when the query's Guid is invalid")
	{
		FWwiseTreeItemPtr Root = WwiseBrowserTestFixtures::MakeTestItem(TEXT("Root"), EWwiseItemType::Folder);
		FWwiseTreeItemPtr Target = WwiseBrowserTestFixtures::MakeTestItem(TEXT("Foo"), EWwiseItemType::Event, FGuid::NewGuid(), 0);
		Root->AddChild(Target);

		FWwiseTreeItemPtr QueryWithInvalidGuid = MakeShared<FWwiseTreeItem>(FString(TEXT("Foo")), FString(TEXT("\\Foo")), nullptr, EWwiseItemType::Event, FGuid());
		FWwiseTreeItemPtr Found = Root->FindItemRecursive(QueryWithInvalidGuid);
		WWISE_CHECK(Found == Target);

		FWwiseTreeItemPtr QueryWithValidGuid = WwiseBrowserTestFixtures::MakeTestItem(TEXT("Foo"), EWwiseItemType::Event, FGuid::NewGuid(), 0);
		FWwiseTreeItemPtr NotFound = Root->FindItemRecursive(QueryWithValidGuid);
		WWISE_CHECK_FALSE(NotFound.IsValid());
	}
}

#endif // WWISE_UNIT_TESTS