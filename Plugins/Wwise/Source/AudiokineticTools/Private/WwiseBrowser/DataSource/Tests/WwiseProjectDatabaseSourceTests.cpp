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
#include "Wwise/AdapterTypes/Unreal/WwiseProjectDatabaseTestFixture.h"
#include "Wwise/AdapterTypes/Unreal/WwiseProjectDatabaseTestingUtils.h"
#include "Wwise/AdapterTypes/Unreal/WwiseUnrealString.h"
#include "Wwise/WwiseTreeItem.h"

// FWwiseProjectDatabaseDataSource::Build* field-mapping coverage.
// All test cases below share one synthetic "Init" SoundBank JSON blob (MakeInitBankJson) parsed
// through WwiseProjectDatabaseTestFixture::BuildTestPlatformDataStructure.
namespace
{
	WwiseDBString MakeInitBankJson()
	{
		return R"(
{
 "SoundBanksInfo": {
  "Platform": "Windows",
  "BasePlatform": "Windows",
  "SchemaVersion": "16",
  "SoundBankVersion": "150",
  "SoundBanks": [
   {
    "Id": "1355168291",
    "Type": "User",
    "GUID": "{701ECBBD-9C7B-4030-8CDB-749EE5D1C7B9}",
    "Language": "SFX",
    "Hash": "{615997BC-9DD8-244F-E4DA-E36233127E61}",
    "ObjectPath": "Init",
    "ShortName": "Init",
    "Path": "Init.bnk",
    "Plugins": {
     "Custom": [
      {
       "Id": "1008723532",
       "Name": "Rhythmic_Talk Define custom",
       "ObjectPath": "\\Effects\\Default Work Unit\\Rhythmic_Talk Define custom",
       "GUID": "{9AA194DD-6AC1-41FC-B2F2-49B8FFF0424B}",
       "LibName": "Wwise Tremolo",
       "LibId": "8585219"
      }
     ],
     "ShareSets": [
      {
       "Id": "1958819083",
       "Name": "SpatialAudio_Room_Medium",
       "ObjectPath": "\\Effects\\Default Work Unit\\SpatialAudio_Room_Medium",
       "GUID": "{05B23C85-8D32-420B-B481-1199CA275683}",
       "LibName": "Wwise RoomVerb",
       "LibId": "7733251"
      }
     ],
     "AudioDevices": [
      {
       "Id": "530303819",
       "Name": "Communication",
       "ObjectPath": "\\Audio Devices\\Default Work Unit\\Communication",
       "GUID": "{21ED38DD-4A4C-47AA-9A7C-B33DD9DB88B4}",
       "LibName": "Communication",
       "LibId": "11534343"
      }
     ]
    },
    "Events": [
     {
      "Id": "1730369216",
      "Name": "Fire_Weapon",
      "ObjectPath": "\\Events\\Default Work Unit\\Fire_Weapon",
      "GUID": "{7982C17E-7EBF-4DB1-9CFB-B221A1526EFE}",
      "Color": "13",
      "DurationType": "OneShot",
      "DurationMin": "1.3388333",
      "DurationMax": "1.3388333"
     }
    ],
    "DialogueEvents": [
     {
      "Id": "111222333",
      "Name": "TestDialogueEvent",
      "ObjectPath": "\\Dialogue Events\\Default Work Unit\\TestDialogueEvent",
      "GUID": "{11112222-3333-4444-5555-666677778888}"
     }
    ],
    "Busses": [
     {
      "Id": "805203703",
      "Name": "Master Secondary Bus",
      "ObjectPath": "\\Busses\\Default Work Unit\\Master Secondary Bus",
      "GUID": "{D2E0BC7E-7B3F-4FFE-A670-FAE02761B631}"
     },
     {
      "Id": "3803692087",
      "Name": "Master Audio Bus",
      "ObjectPath": "\\Busses\\Default Work Unit\\Master Audio Bus",
      "GUID": "{1514A4D8-1DA6-412A-A17E-75CA0C2149F3}",
      "PluginRefs": {
       "ShareSets": [
        {
         "Id": "2589936646"
        }
       ]
      }
     }
    ],
    "AuxBusses": [
     {
      "Id": "187046019",
      "Name": "LargeRoom",
      "ObjectPath": "\\Busses\\Default Work Unit\\Master Audio Bus\\LargeRoom",
      "GUID": "{9BF4EB38-0C67-4347-8DB8-926791DD6ED3}",
      "PluginRefs": {
       "ShareSets": [
        {
         "Id": "1958819083"
        }
       ]
      }
     }
    ],
    "GameParameters": [
     {
      "Id": "194404061",
      "Name": "PreDelay",
      "ObjectPath": "\\Game Parameters\\Default Work Unit\\PreDelay",
      "GUID": "{BDCEC39D-FCA8-40DF-B620-A3BBDA354F92}"
     }
    ],
    "StateGroups": [
     {
      "Id": "3978303460",
      "Name": "TestState",
      "ObjectPath": "\\States\\Default Work Unit\\TestState",
      "GUID": "{6476B68F-10E7-42BD-8090-40A54CEED6CA}",
      "States": [
       {
        "Id": "748895195",
        "Name": "None",
        "ObjectPath": "\\States\\Default Work Unit\\TestState\\None",
        "GUID": "{801C436E-66C8-4348-BB58-1398D6799B34}"
       },
       {
        "Id": "2021425847",
        "Name": "New_State",
        "ObjectPath": "\\States\\Default Work Unit\\TestState\\New_State",
        "GUID": "{90EAE6ED-DD96-49DA-8780-12E966F33BC0}"
       }
      ]
     }
    ],
    "SwitchGroups": [
     {
      "Id": "858531997",
      "Name": "Frequency",
      "ObjectPath": "\\Switches\\Default Work Unit\\Frequency",
      "GUID": "{EEBAA75B-9E94-4EAF-A9DD-4F38ED4D5C8D}",
      "Switches": [
       {
        "Id": "545371365",
        "Name": "Low",
        "ObjectPath": "\\Switches\\Default Work Unit\\Frequency\\Low",
        "GUID": "{7690228D-2131-4E18-BF05-6B16F5105B92}"
       },
       {
        "Id": "3550808449",
        "Name": "High",
        "ObjectPath": "\\Switches\\Default Work Unit\\Frequency\\High",
        "GUID": "{8804A8FD-746F-4F88-A8B8-C1E2E9F3AB8F}"
       }
      ]
     }
    ],
    "Triggers": [
     {
      "Id": "444555666",
      "Name": "TestTrigger",
      "ObjectPath": "\\Triggers\\Default Work Unit\\TestTrigger",
      "GUID": "{99998888-7777-6666-5555-444433332222}"
     }
    ],
    "AcousticTextures": [
     {
      "Id": "1970351858",
      "Name": "Fabric",
      "ObjectPath": "\\Virtual Acoustics\\Factory Acoustic Textures\\Textures\\Fabric",
      "GUID": "{8381D532-DB6E-4EED-A7A3-BC7B0A0B8BA8}"
     }
    ]
   }
  ],
  "FileHash": "{8124FAE6-2829-1DEC-36EC-87E839A8B188}"
 }
}
)"_wwise_db;
	}

	TSharedPtr<WwisePlatformDataStructure> BuildFixture(const WwiseDBString& Json = MakeInitBankJson())
	{
		TSharedPtr<WwisePlatformDataStructure> PlatformData = WwiseProjectDatabaseTestFixture::BuildTestPlatformDataStructure(Json);
		WWISE_CHECK(PlatformData.IsValid());
		return PlatformData;
	}

	// Two Events sharing both their RootFolder ("Default Work Unit") and an intermediate folder ("Weapons") in ObjectPath
	WwiseDBString MakeFolderReuseJson()
	{
		return R"(
{
 "SoundBanksInfo": {
  "Platform": "Windows",
  "BasePlatform": "Windows",
  "SchemaVersion": "16",
  "SoundBankVersion": "150",
  "SoundBanks": [
   {
    "Id": "1355168291",
    "Type": "User",
    "GUID": "{701ECBBD-9C7B-4030-8CDB-749EE5D1C7B9}",
    "Language": "SFX",
    "Hash": "{615997BC-9DD8-244F-E4DA-E36233127E61}",
    "ObjectPath": "Init",
    "ShortName": "Init",
    "Path": "Init.bnk",
    "Plugins": {
     "Custom": [
      {
       "Id": "1008723532",
       "Name": "Rhythmic_Talk Define custom",
       "ObjectPath": "\\Effects\\Default Work Unit\\Rhythmic_Talk Define custom",
       "GUID": "{9AA194DD-6AC1-41FC-B2F2-49B8FFF0424B}",
       "LibName": "Wwise Tremolo",
       "LibId": "8585219"
      }
     ],
     "ShareSets": [
      {
       "Id": "1958819083",
       "Name": "SpatialAudio_Room_Medium",
       "ObjectPath": "\\Effects\\Default Work Unit\\SpatialAudio_Room_Medium",
       "GUID": "{05B23C85-8D32-420B-B481-1199CA275683}",
       "LibName": "Wwise RoomVerb",
       "LibId": "7733251"
      }
     ],
     "AudioDevices": [
      {
       "Id": "530303819",
       "Name": "Communication",
       "ObjectPath": "\\Audio Devices\\Default Work Unit\\Communication",
       "GUID": "{21ED38DD-4A4C-47AA-9A7C-B33DD9DB88B4}",
       "LibName": "Communication",
       "LibId": "11534343"
      }
     ]
    },
    "Events": [
     {
      "Id": "1730369216",
      "Name": "Rifle_Fire",
      "ObjectPath": "\\Events\\Default Work Unit\\Weapons\\Rifle_Fire",
      "GUID": "{7982C17E-7EBF-4DB1-9CFB-B221A1526EFE}",
      "Color": "13",
      "DurationType": "OneShot",
      "DurationMin": "1.3388333",
      "DurationMax": "1.3388333"
     },
     {
      "Id": "1730369217",
      "Name": "Rifle_Reload",
      "ObjectPath": "\\Events\\Default Work Unit\\Weapons\\Rifle_Reload",
      "GUID": "{7982C17E-7EBF-4DB1-9CFB-B221A1526EFF}",
      "Color": "13",
      "DurationType": "OneShot",
      "DurationMin": "1.3388333",
      "DurationMax": "1.3388333"
     }
    ],
    "DialogueEvents": [
     {
      "Id": "111222333",
      "Name": "TestDialogueEvent",
      "ObjectPath": "\\Dialogue Events\\Default Work Unit\\TestDialogueEvent",
      "GUID": "{11112222-3333-4444-5555-666677778888}"
     }
    ],
    "Busses": [
     {
      "Id": "805203703",
      "Name": "Master Secondary Bus",
      "ObjectPath": "\\Busses\\Default Work Unit\\Master Secondary Bus",
      "GUID": "{D2E0BC7E-7B3F-4FFE-A670-FAE02761B631}"
     },
     {
      "Id": "3803692087",
      "Name": "Master Audio Bus",
      "ObjectPath": "\\Busses\\Default Work Unit\\Master Audio Bus",
      "GUID": "{1514A4D8-1DA6-412A-A17E-75CA0C2149F3}",
      "PluginRefs": {
       "ShareSets": [
        {
         "Id": "2589936646"
        }
       ]
      }
     }
    ],
    "AuxBusses": [
     {
      "Id": "187046019",
      "Name": "LargeRoom",
      "ObjectPath": "\\Busses\\Default Work Unit\\Master Audio Bus\\LargeRoom",
      "GUID": "{9BF4EB38-0C67-4347-8DB8-926791DD6ED3}",
      "PluginRefs": {
       "ShareSets": [
        {
         "Id": "1958819083"
        }
       ]
      }
     }
    ],
    "GameParameters": [
     {
      "Id": "194404061",
      "Name": "PreDelay",
      "ObjectPath": "\\Game Parameters\\Default Work Unit\\PreDelay",
      "GUID": "{BDCEC39D-FCA8-40DF-B620-A3BBDA354F92}"
     }
    ],
    "StateGroups": [
     {
      "Id": "3978303460",
      "Name": "TestState",
      "ObjectPath": "\\States\\Default Work Unit\\TestState",
      "GUID": "{6476B68F-10E7-42BD-8090-40A54CEED6CA}",
      "States": [
       {
        "Id": "748895195",
        "Name": "None",
        "ObjectPath": "\\States\\Default Work Unit\\TestState\\None",
        "GUID": "{801C436E-66C8-4348-BB58-1398D6799B34}"
       },
       {
        "Id": "2021425847",
        "Name": "New_State",
        "ObjectPath": "\\States\\Default Work Unit\\TestState\\New_State",
        "GUID": "{90EAE6ED-DD96-49DA-8780-12E966F33BC0}"
       }
      ]
     }
    ],
    "SwitchGroups": [
     {
      "Id": "858531997",
      "Name": "Frequency",
      "ObjectPath": "\\Switches\\Default Work Unit\\Frequency",
      "GUID": "{EEBAA75B-9E94-4EAF-A9DD-4F38ED4D5C8D}",
      "Switches": [
       {
        "Id": "545371365",
        "Name": "Low",
        "ObjectPath": "\\Switches\\Default Work Unit\\Frequency\\Low",
        "GUID": "{7690228D-2131-4E18-BF05-6B16F5105B92}"
       },
       {
        "Id": "3550808449",
        "Name": "High",
        "ObjectPath": "\\Switches\\Default Work Unit\\Frequency\\High",
        "GUID": "{8804A8FD-746F-4F88-A8B8-C1E2E9F3AB8F}"
       }
      ]
     }
    ],
    "Triggers": [
     {
      "Id": "444555666",
      "Name": "TestTrigger",
      "ObjectPath": "\\Triggers\\Default Work Unit\\TestTrigger",
      "GUID": "{99998888-7777-6666-5555-444433332222}"
     }
    ],
    "AcousticTextures": [
     {
      "Id": "1970351858",
      "Name": "Fabric",
      "ObjectPath": "\\Virtual Acoustics\\Factory Acoustic Textures\\Textures\\Fabric",
      "GUID": "{8381D532-DB6E-4EED-A7A3-BC7B0A0B8BA8}"
     }
    ]
   }
  ],
  "FileHash": "{8124FAE6-2829-1DEC-36EC-87E839A8B188}"
 }
}
)"_wwise_db;
	}

	FWwiseTreeItemPtr FindChildByName(const FWwiseTreeItemPtr& Parent, const FString& Name)
	{
		if (!Parent)
		{
			return nullptr;
		}
		for (const auto& Child : Parent->GetChildren())
		{
			if (Child->DisplayName == Name)
			{
				return Child;
			}
		}
		return nullptr;
	}

	// Recursively gathers every non-Folder item under Item (Item itself included), for
	// comparison against Ctx.AllValidTreeItemsByGuid/ShortId/Name population.
	void CollectNonFolderItems(const FWwiseTreeItemPtr& Item, TArray<FWwiseTreeItemPtr>& OutItems)
	{
		if (!Item)
		{
			return;
		}
		if (Item->ItemType != EWwiseItemType::Folder)
		{
			OutItems.Add(Item);
		}
		for (const auto& Child : Item->GetChildren())
		{
			CollectNonFolderItems(Child, OutItems);
		}
	}

	// StringFilter (TTextFilter<const FString&>) construction, mirroring
	// SWwiseBrowser::PopulateSearchStrings but as a free function so it can be bound with
	// CreateStatic instead of needing a live SWwiseBrowser widget.
	void PopulateFilterStrings(const FString& InDisplayName, TArray<FString>& OutStrings)
	{
		OutStrings.Add(InDisplayName);
	}

	TSharedPtr<StringFilter> MakeTextFilter(const FString& FilterText)
	{
		TSharedPtr<StringFilter> Filter = MakeShared<StringFilter>(StringFilter::FItemToStringArray::CreateStatic(&PopulateFilterStrings));
		Filter->SetRawFilterText(FText::FromString(FilterText));
		return Filter;
	}

	// Structural dump of a tree (DisplayName/ItemType/ShortId, children sorted by name for order-independence)
	// so two independently-built trees can be compared for equality.
	FString DumpTree(const FWwiseTreeItemPtr& Item)
	{
		if (!Item)
		{
			return TEXT("<null>");
		}
		TArray<FWwiseTreeItemPtr> Children = Item->GetChildren();
		Children.Sort([](const FWwiseTreeItemPtr& A, const FWwiseTreeItemPtr& B)
		{
			return A->DisplayName < B->DisplayName;
		});
		FString Result = FString::Printf(TEXT("(%s:%d:%u"), *Item->DisplayName, static_cast<int32>(Item->ItemType), Item->ShortId);
		for (const auto& Child : Children)
		{
			Result.Append(DumpTree(Child));
		}
		Result.Append(TEXT(")"));
		return Result;
	}
}

WWISE_TEST_CASE(WwiseProjectDatabaseSource_BuildEvents, "Wwise::AudiokineticTools::WwiseProjectDatabaseSource::BuildEvents", "[EditorContext][ProductFilter]")
{
	SECTION("Maps Events into the Event root, keyed by ObjectPath/Name/Id")
	{
		TSharedPtr<WwisePlatformDataStructure> PlatformData = BuildFixture();
		FWwiseProjectDatabaseDataSourceTest DataSource;
		FWwiseTreeBuildContext Ctx = DataSource.MakeTreeBuildContext_ForTesting();
		DataSource.BuildEvents_ForTesting(PlatformData->Events, Ctx);

		FWwiseTreeItemPtr Root = DataSource.GetRootItem(EWwiseItemType::Event);
		WWISE_CHECK(Root.IsValid());
		WWISE_CHECK(Root->ChildCountInWwise == 1);

		FWwiseTreeItemPtr WorkUnit = FindChildByName(Root, TEXT("Default Work Unit"));
		WWISE_CHECK(WorkUnit.IsValid());

		FWwiseTreeItemPtr EventItem = FindChildByName(WorkUnit, TEXT("Fire_Weapon"));
		WWISE_CHECK(EventItem.IsValid());
		if (EventItem)
		{
			WWISE_CHECK(EventItem->ItemType == EWwiseItemType::Event);
			WWISE_CHECK(EventItem->ShortId == 1730369216u);
			WWISE_CHECK(EventItem->GroupId == 0);
			WWISE_CHECK(EventItem->FolderPath == TEXT("\\Events\\Default Work Unit\\Fire_Weapon"));
			WWISE_CHECK(EventItem->ItemId.IsValid());
		}
	}
}

WWISE_TEST_CASE(WwiseProjectDatabaseSource_BuildDialogueEvents, "Wwise::AudiokineticTools::WwiseProjectDatabaseSource::BuildDialogueEvents", "[ApplicationContextMask][SmokeFilter]")
{
	SECTION("Maps DialogueEvents into the DialogueEvent root, keyed by ObjectPath/Name/Id")
	{
		TSharedPtr<WwisePlatformDataStructure> PlatformData = BuildFixture();
		FWwiseProjectDatabaseDataSourceTest DataSource;
		FWwiseTreeBuildContext Ctx = DataSource.MakeTreeBuildContext_ForTesting();
		DataSource.BuildDialogueEvents_ForTesting(PlatformData->DialogueEvents, Ctx);

		FWwiseTreeItemPtr Root = DataSource.GetRootItem(EWwiseItemType::DialogueEvent);
		WWISE_CHECK(Root.IsValid());

		FWwiseTreeItemPtr WorkUnit = FindChildByName(Root, TEXT("Default Work Unit"));
		WWISE_CHECK(WorkUnit.IsValid());

		FWwiseTreeItemPtr DialogueEventItem = FindChildByName(WorkUnit, TEXT("TestDialogueEvent"));
		WWISE_CHECK(DialogueEventItem.IsValid());
		if (DialogueEventItem)
		{
			WWISE_CHECK(DialogueEventItem->ItemType == EWwiseItemType::DialogueEvent);
			WWISE_CHECK(DialogueEventItem->ShortId == 111222333u);
			WWISE_CHECK(DialogueEventItem->FolderPath == TEXT("\\Dialogue Events\\Default Work Unit\\TestDialogueEvent"));
		}
	}
}

WWISE_TEST_CASE(WwiseProjectDatabaseSource_BuildBussesAndAuxBusses, "Wwise::AudiokineticTools::WwiseProjectDatabaseSource::BuildBussesAndAuxBusses", "[EditorContext][ProductFilter]")
{
	SECTION("BuildAuxBusses nests under the Busses root populated by BuildBusses, keyed onto their parent Bus")
	{
		TSharedPtr<WwisePlatformDataStructure> PlatformData = BuildFixture();
		FWwiseProjectDatabaseDataSourceTest DataSource;
		FWwiseTreeBuildContext Ctx = DataSource.MakeTreeBuildContext_ForTesting();
		DataSource.BuildBusses_ForTesting(PlatformData->Busses, Ctx);
		DataSource.BuildAuxBusses_ForTesting(PlatformData->AuxBusses, Ctx);

		FWwiseTreeItemPtr Root = DataSource.GetRootItem(EWwiseItemType::AuxBus);
		WWISE_CHECK(Root.IsValid());

		FWwiseTreeItemPtr WorkUnit = FindChildByName(Root, TEXT("Default Work Unit"));
		WWISE_CHECK(WorkUnit.IsValid());

		FWwiseTreeItemPtr SecondaryBus = FindChildByName(WorkUnit, TEXT("Master Secondary Bus"));
		WWISE_CHECK(SecondaryBus.IsValid());

		FWwiseTreeItemPtr MasterBus = FindChildByName(WorkUnit, TEXT("Master Audio Bus"));
		WWISE_CHECK(MasterBus.IsValid());
		if (MasterBus)
		{
			WWISE_CHECK(MasterBus->ItemType == EWwiseItemType::Bus);
			WWISE_CHECK(MasterBus->ShortId == 3803692087u);
		}

		FWwiseTreeItemPtr AuxBusItem = FindChildByName(MasterBus, TEXT("LargeRoom"));
		WWISE_CHECK(AuxBusItem.IsValid());
		if (AuxBusItem)
		{
			WWISE_CHECK(AuxBusItem->ItemType == EWwiseItemType::AuxBus);
			WWISE_CHECK(AuxBusItem->ShortId == 187046019u);
			WWISE_CHECK(AuxBusItem->FolderPath == TEXT("\\Busses\\Default Work Unit\\Master Audio Bus\\LargeRoom"));
		}
	}
}

WWISE_TEST_CASE(WwiseProjectDatabaseSource_BuildAcousticTextures, "Wwise::AudiokineticTools::WwiseProjectDatabaseSource::BuildAcousticTextures", "[EditorContext][ProductFilter]")
{
	SECTION("Maps AcousticTextures into the AcousticTexture root, keyed by ObjectPath/Name/Id")
	{
		TSharedPtr<WwisePlatformDataStructure> PlatformData = BuildFixture();
		FWwiseProjectDatabaseDataSourceTest DataSource;
		FWwiseTreeBuildContext Ctx = DataSource.MakeTreeBuildContext_ForTesting();
		DataSource.BuildAcousticTextures_ForTesting(PlatformData->AcousticTextures, Ctx);

		FWwiseTreeItemPtr Root = DataSource.GetRootItem(EWwiseItemType::AcousticTexture);
		WWISE_CHECK(Root.IsValid());

		FWwiseTreeItemPtr FactoryFolder = FindChildByName(Root, TEXT("Factory Acoustic Textures"));
		WWISE_CHECK(FactoryFolder.IsValid());

		FWwiseTreeItemPtr TexturesFolder = FindChildByName(FactoryFolder, TEXT("Textures"));
		WWISE_CHECK(TexturesFolder.IsValid());

		FWwiseTreeItemPtr TextureItem = FindChildByName(TexturesFolder, TEXT("Fabric"));
		WWISE_CHECK(TextureItem.IsValid());
		if (TextureItem)
		{
			WWISE_CHECK(TextureItem->ItemType == EWwiseItemType::AcousticTexture);
			WWISE_CHECK(TextureItem->ShortId == 1970351858u);
			WWISE_CHECK(TextureItem->FolderPath == TEXT("\\Virtual Acoustics\\Factory Acoustic Textures\\Textures\\Fabric"));
		}
	}
}

WWISE_TEST_CASE(WwiseProjectDatabaseSource_BuildAudioDevices, "Wwise::AudiokineticTools::WwiseProjectDatabaseSource::BuildAudioDevices", "[EditorContext][ProductFilter]")
{
	SECTION("Maps SoundBank.Plugins.AudioDevices into the AudioDeviceShareSet root")
	{
		TSharedPtr<WwisePlatformDataStructure> PlatformData = BuildFixture();
		FWwiseProjectDatabaseDataSourceTest DataSource;
		FWwiseTreeBuildContext Ctx = DataSource.MakeTreeBuildContext_ForTesting();
		DataSource.BuildAudioDevices_ForTesting(PlatformData->AudioDevices, Ctx);

		FWwiseTreeItemPtr Root = DataSource.GetRootItem(EWwiseItemType::AudioDeviceShareSet);
		WWISE_CHECK(Root.IsValid());

		FWwiseTreeItemPtr WorkUnit = FindChildByName(Root, TEXT("Default Work Unit"));
		WWISE_CHECK(WorkUnit.IsValid());

		FWwiseTreeItemPtr AudioDeviceItem = FindChildByName(WorkUnit, TEXT("Communication"));
		WWISE_CHECK(AudioDeviceItem.IsValid());
		if (AudioDeviceItem)
		{
			WWISE_CHECK(AudioDeviceItem->ItemType == EWwiseItemType::AudioDeviceShareSet);
			WWISE_CHECK(AudioDeviceItem->ShortId == 530303819u);
		}
	}
}

WWISE_TEST_CASE(WwiseProjectDatabaseSource_BuildStateGroupsAndStates, "Wwise::AudiokineticTools::WwiseProjectDatabaseSource::BuildStateGroupsAndStates", "[EditorContext][ProductFilter]")
{
	SECTION("BuildStates nests under the StateGroup populated by BuildStateGroups and carries its GroupId")
	{
		TSharedPtr<WwisePlatformDataStructure> PlatformData = BuildFixture();
		FWwiseProjectDatabaseDataSourceTest DataSource;
		FWwiseTreeBuildContext Ctx = DataSource.MakeTreeBuildContext_ForTesting();
		DataSource.BuildStateGroups_ForTesting(PlatformData->StateGroups, Ctx);
		DataSource.BuildStates_ForTesting(PlatformData->States, Ctx);

		FWwiseTreeItemPtr Root = DataSource.GetRootItem(EWwiseItemType::State);
		WWISE_CHECK(Root.IsValid());

		FWwiseTreeItemPtr WorkUnit = FindChildByName(Root, TEXT("Default Work Unit"));
		WWISE_CHECK(WorkUnit.IsValid());

		FWwiseTreeItemPtr StateGroupItem = FindChildByName(WorkUnit, TEXT("TestState"));
		WWISE_CHECK(StateGroupItem.IsValid());
		if (StateGroupItem)
		{
			WWISE_CHECK(StateGroupItem->ItemType == EWwiseItemType::StateGroup);
			WWISE_CHECK(StateGroupItem->ShortId == 3978303460u);
		}

		FWwiseTreeItemPtr StateItem = FindChildByName(StateGroupItem, TEXT("New_State"));
		WWISE_CHECK(StateItem.IsValid());
		if (StateItem)
		{
			WWISE_CHECK(StateItem->ItemType == EWwiseItemType::State);
			WWISE_CHECK(StateItem->ShortId == 2021425847u);
			WWISE_CHECK(StateItem->GroupId == 3978303460u);
			WWISE_CHECK(StateItem->FolderPath == TEXT("\\States\\Default Work Unit\\TestState\\New_State"));
		}
	}
}

WWISE_TEST_CASE(WwiseProjectDatabaseSource_BuildSwitchGroupsAndSwitches, "Wwise::AudiokineticTools::WwiseProjectDatabaseSource::BuildSwitchGroupsAndSwitches", "[EditorContext][ProductFilter]")
{
	SECTION("BuildSwitches nests under the SwitchGroup populated by BuildSwitchGroups and carries its GroupId")
	{
		TSharedPtr<WwisePlatformDataStructure> PlatformData = BuildFixture();
		FWwiseProjectDatabaseDataSourceTest DataSource;
		FWwiseTreeBuildContext Ctx = DataSource.MakeTreeBuildContext_ForTesting();
		DataSource.BuildSwitchGroups_ForTesting(PlatformData->SwitchGroups, Ctx);
		DataSource.BuildSwitches_ForTesting(PlatformData->Switches, Ctx);

		FWwiseTreeItemPtr Root = DataSource.GetRootItem(EWwiseItemType::Switch);
		WWISE_CHECK(Root.IsValid());

		FWwiseTreeItemPtr WorkUnit = FindChildByName(Root, TEXT("Default Work Unit"));
		WWISE_CHECK(WorkUnit.IsValid());

		FWwiseTreeItemPtr SwitchGroupItem = FindChildByName(WorkUnit, TEXT("Frequency"));
		WWISE_CHECK(SwitchGroupItem.IsValid());
		if (SwitchGroupItem)
		{
			WWISE_CHECK(SwitchGroupItem->ItemType == EWwiseItemType::SwitchGroup);
			WWISE_CHECK(SwitchGroupItem->ShortId == 858531997u);
		}

		FWwiseTreeItemPtr SwitchItem = FindChildByName(SwitchGroupItem, TEXT("High"));
		WWISE_CHECK(SwitchItem.IsValid());
		if (SwitchItem)
		{
			WWISE_CHECK(SwitchItem->ItemType == EWwiseItemType::Switch);
			WWISE_CHECK(SwitchItem->ShortId == 3550808449u);
			WWISE_CHECK(SwitchItem->GroupId == 858531997u);
			WWISE_CHECK(SwitchItem->FolderPath == TEXT("\\Switches\\Default Work Unit\\Frequency\\High"));
		}
	}
}

WWISE_TEST_CASE(WwiseProjectDatabaseSource_BuildGameParameters, "Wwise::AudiokineticTools::WwiseProjectDatabaseSource::BuildGameParameters", "[EditorContext][ProductFilter]")
{
	SECTION("Maps GameParameters into the GameParameter root, keyed by ObjectPath/Name/Id")
	{
		TSharedPtr<WwisePlatformDataStructure> PlatformData = BuildFixture();
		FWwiseProjectDatabaseDataSourceTest DataSource;
		FWwiseTreeBuildContext Ctx = DataSource.MakeTreeBuildContext_ForTesting();
		DataSource.BuildGameParameters_ForTesting(PlatformData->GameParameters, Ctx);

		FWwiseTreeItemPtr Root = DataSource.GetRootItem(EWwiseItemType::GameParameter);
		WWISE_CHECK(Root.IsValid());

		FWwiseTreeItemPtr WorkUnit = FindChildByName(Root, TEXT("Default Work Unit"));
		WWISE_CHECK(WorkUnit.IsValid());

		FWwiseTreeItemPtr GameParamItem = FindChildByName(WorkUnit, TEXT("PreDelay"));
		WWISE_CHECK(GameParamItem.IsValid());
		if (GameParamItem)
		{
			WWISE_CHECK(GameParamItem->ItemType == EWwiseItemType::GameParameter);
			WWISE_CHECK(GameParamItem->ShortId == 194404061u);
			WWISE_CHECK(GameParamItem->FolderPath == TEXT("\\Game Parameters\\Default Work Unit\\PreDelay"));
		}
	}
}

WWISE_TEST_CASE(WwiseProjectDatabaseSource_BuildTriggers, "Wwise::AudiokineticTools::WwiseProjectDatabaseSource::BuildTriggers", "[EditorContext][ProductFilter]")
{
	SECTION("Maps Triggers into the Trigger root, keyed by ObjectPath/Name/Id")
	{
		TSharedPtr<WwisePlatformDataStructure> PlatformData = BuildFixture();
		FWwiseProjectDatabaseDataSourceTest DataSource;
		FWwiseTreeBuildContext Ctx = DataSource.MakeTreeBuildContext_ForTesting();
		DataSource.BuildTriggers_ForTesting(PlatformData->Triggers, Ctx);

		FWwiseTreeItemPtr Root = DataSource.GetRootItem(EWwiseItemType::Trigger);
		WWISE_CHECK(Root.IsValid());

		FWwiseTreeItemPtr WorkUnit = FindChildByName(Root, TEXT("Default Work Unit"));
		WWISE_CHECK(WorkUnit.IsValid());

		FWwiseTreeItemPtr TriggerItem = FindChildByName(WorkUnit, TEXT("TestTrigger"));
		WWISE_CHECK(TriggerItem.IsValid());
		if (TriggerItem)
		{
			WWISE_CHECK(TriggerItem->ItemType == EWwiseItemType::Trigger);
			WWISE_CHECK(TriggerItem->ShortId == 444555666u);
			WWISE_CHECK(TriggerItem->FolderPath == TEXT("\\Triggers\\Default Work Unit\\TestTrigger"));
		}
	}
}

WWISE_TEST_CASE(WwiseProjectDatabaseSource_BuildEffectShareSets, "Wwise::AudiokineticTools::WwiseProjectDatabaseSource::BuildEffectShareSets", "[EditorContext][ProductFilter]")
{
	SECTION("Maps SoundBank.Plugins.ShareSets into the EffectShareSet root")
	{
		TSharedPtr<WwisePlatformDataStructure> PlatformData = BuildFixture();
		FWwiseProjectDatabaseDataSourceTest DataSource;
		FWwiseTreeBuildContext Ctx = DataSource.MakeTreeBuildContext_ForTesting();
		DataSource.BuildEffectShareSets_ForTesting(PlatformData->PluginShareSets, Ctx);

		FWwiseTreeItemPtr Root = DataSource.GetRootItem(EWwiseItemType::EffectShareSet);
		WWISE_CHECK(Root.IsValid());

		FWwiseTreeItemPtr WorkUnit = FindChildByName(Root, TEXT("Default Work Unit"));
		WWISE_CHECK(WorkUnit.IsValid());

		FWwiseTreeItemPtr ShareSetItem = FindChildByName(WorkUnit, TEXT("SpatialAudio_Room_Medium"));
		WWISE_CHECK(ShareSetItem.IsValid());
		if (ShareSetItem)
		{
			WWISE_CHECK(ShareSetItem->ItemType == EWwiseItemType::EffectShareSet);
			WWISE_CHECK(ShareSetItem->ShortId == 1958819083u);
		}
	}
}

WWISE_TEST_CASE(WwiseProjectDatabaseSource_BuildFolderHierarchy_ReusesSharedFolders, "Wwise::AudiokineticTools::WwiseProjectDatabaseSource::BuildFolderHierarchy_ReusesSharedFolders", "[EditorContext][ProductFilter]")
{
	SECTION("Two items sharing RootFolder and an intermediate folder create one folder node per level, not one per item")
	{
		TSharedPtr<WwisePlatformDataStructure> PlatformData = BuildFixture(MakeFolderReuseJson());
		FWwiseProjectDatabaseDataSourceTest DataSource;
		FWwiseTreeBuildContext Ctx = DataSource.MakeTreeBuildContext_ForTesting();
		DataSource.BuildEvents_ForTesting(PlatformData->Events, Ctx);

		FWwiseTreeItemPtr Root = DataSource.GetRootItem(EWwiseItemType::Event);
		WWISE_CHECK(Root.IsValid());
		if (!Root)
		{
			return;
		}

		// Only one "Default Work Unit" folder should exist under the root, reused across both events.
		WWISE_CHECK(Root->GetChildren().Num() == 1);
		FWwiseTreeItemPtr WorkUnit = FindChildByName(Root, TEXT("Default Work Unit"));
		WWISE_CHECK(WorkUnit.IsValid());
		if (!WorkUnit)
		{
			return;
		}

		// Only one "Weapons" intermediate folder should exist, not one per event.
		WWISE_CHECK(WorkUnit->GetChildren().Num() == 1);
		FWwiseTreeItemPtr WeaponsFolder = FindChildByName(WorkUnit, TEXT("Weapons"));
		WWISE_CHECK(WeaponsFolder.IsValid());
		if (!WeaponsFolder)
		{
			return;
		}
		WWISE_CHECK(WeaponsFolder->ItemType == EWwiseItemType::Folder);

		// Both events are parented under that same shared folder node.
		WWISE_CHECK(WeaponsFolder->GetChildren().Num() == 2);
		FWwiseTreeItemPtr FireItem = FindChildByName(WeaponsFolder, TEXT("Rifle_Fire"));
		FWwiseTreeItemPtr ReloadItem = FindChildByName(WeaponsFolder, TEXT("Rifle_Reload"));
		WWISE_CHECK(FireItem.IsValid());
		WWISE_CHECK(ReloadItem.IsValid());
		if (FireItem && ReloadItem)
		{
			WWISE_CHECK(FireItem->Parent.Pin() == WeaponsFolder);
			WWISE_CHECK(ReloadItem->Parent.Pin() == WeaponsFolder);
		}
	}
}

WWISE_TEST_CASE(WwiseProjectDatabaseSource_BuildFolderHierarchy_RegistersContainersButNotLeaves, "Wwise::AudiokineticTools::WwiseProjectDatabaseSource::BuildFolderHierarchy_RegistersContainersButNotLeaves", "[EditorContext][ProductFilter]")
{
	SECTION("A container (Bus) is findable by its own ObjectPath; a leaf (Event) is not")
	{
		TSharedPtr<WwisePlatformDataStructure> PlatformData = BuildFixture();
		FWwiseProjectDatabaseDataSourceTest DataSource;
		FWwiseTreeBuildContext Ctx = DataSource.MakeTreeBuildContext_ForTesting();
		DataSource.BuildBusses_ForTesting(PlatformData->Busses, Ctx);
		DataSource.BuildEvents_ForTesting(PlatformData->Events, Ctx);

		FWwiseTreeItemPtr BusByPath = DataSource.FindItemFromPath(TEXT("\\Busses\\Default Work Unit\\Master Audio Bus"));
		WWISE_CHECK(BusByPath.IsValid());
		if (BusByPath)
		{
			WWISE_CHECK(BusByPath->ItemType == EWwiseItemType::Bus);
			WWISE_CHECK(BusByPath->ShortId == 3803692087u);
		}

		// The Event leaf itself is not registered at its own ObjectPath - only its ancestor
		// folders are (BuildFolderHierarchy only adds containers, not leaves, to NodesByPath).
		FWwiseTreeItemPtr EventByPath = DataSource.FindItemFromPath(TEXT("\\Events\\Default Work Unit\\Fire_Weapon"));
		WWISE_CHECK(!EventByPath.IsValid());

		// Its ancestor Folder is still registered, since every folder level always is.
		FWwiseTreeItemPtr EventWorkUnitByPath = DataSource.FindItemFromPath(TEXT("\\Events\\Default Work Unit"));
		WWISE_CHECK(EventWorkUnitByPath.IsValid());
		if (EventWorkUnitByPath)
		{
			WWISE_CHECK(EventWorkUnitByPath->ItemType == EWwiseItemType::Folder);
		}
	}
}

WWISE_TEST_CASE(WwiseProjectDatabaseSource_AllValidTreeItems_PopulatedForNonFolderItemsOnly, "Wwise::AudiokineticTools::WwiseProjectDatabaseSource::AllValidTreeItems_PopulatedForNonFolderItemsOnly", "[EditorContext][ProductFilter]")
{
	SECTION("AllValidTreeItemsByGuid/ShortId/Name contain exactly the non-Folder items in the built tree")
	{
		TSharedPtr<WwisePlatformDataStructure> PlatformData = BuildFixture();
		FWwiseProjectDatabaseDataSourceTest DataSource;
		FWwiseTreeBuildContext Ctx = DataSource.MakeTreeBuildContext_ForTesting();
		DataSource.BuildEvents_ForTesting(PlatformData->Events, Ctx);
		DataSource.BuildBusses_ForTesting(PlatformData->Busses, Ctx);
		DataSource.BuildAuxBusses_ForTesting(PlatformData->AuxBusses, Ctx);

		TArray<FWwiseTreeItemPtr> NonFolderItems;
		CollectNonFolderItems(DataSource.GetRootItem(EWwiseItemType::Event), NonFolderItems);
		// Busses and AuxBusses are both rooted under the AuxBus slot (see BuildBusses).
		CollectNonFolderItems(DataSource.GetRootItem(EWwiseItemType::AuxBus), NonFolderItems);

		// 1 Event + 2 Busses + 1 AuxBus.
		WWISE_CHECK(NonFolderItems.Num() == 4);
		WWISE_CHECK(Ctx.AllValidTreeItemsByGuid.Num() == NonFolderItems.Num());
		WWISE_CHECK(Ctx.AllValidTreeItemsByShortId.Num() == NonFolderItems.Num());
		WWISE_CHECK(Ctx.AllValidTreeItemsByName.Num() == NonFolderItems.Num());

		for (const FWwiseTreeItemPtr& Item : NonFolderItems)
		{
			FWwiseTreeItemPtr* ByGuid = Ctx.AllValidTreeItemsByGuid.Find(Item->ItemId);
			WWISE_CHECK(ByGuid != nullptr);
			WWISE_CHECK(ByGuid && *ByGuid == Item);

			TArray<FWwiseTreeItemPtr> ByShortId;
			Ctx.AllValidTreeItemsByShortId.MultiFind(Item->ShortId, ByShortId);
			WWISE_CHECK(ByShortId.Contains(Item));

			TArray<FWwiseTreeItemPtr> ByName;
			Ctx.AllValidTreeItemsByName.MultiFind(Item->DisplayName, ByName);
			WWISE_CHECK(ByName.Contains(Item));
		}
	}
}

WWISE_TEST_CASE(WwiseProjectDatabaseSource_FindItemFromPath_GetRootItem_GetChildItemCount_RoundTrip, "Wwise::AudiokineticTools::WwiseProjectDatabaseSource::FindItemFromPath_GetRootItem_GetChildItemCount_RoundTrip", "[EditorContext][ProductFilter]")
{
	SECTION("GetChildItemCount matches GetChildren().Num(), and FindItemFromPath returns the exact node reached by traversal")
	{
		TSharedPtr<WwisePlatformDataStructure> PlatformData = BuildFixture();
		FWwiseProjectDatabaseDataSourceTest DataSource;
		FWwiseTreeBuildContext Ctx = DataSource.MakeTreeBuildContext_ForTesting();
		DataSource.BuildEvents_ForTesting(PlatformData->Events, Ctx);
		DataSource.BuildDialogueEvents_ForTesting(PlatformData->DialogueEvents, Ctx);

		FWwiseTreeItemPtr EventRoot = DataSource.GetRootItem(EWwiseItemType::Event);
		WWISE_CHECK(EventRoot.IsValid());
		if (!EventRoot)
		{
			return;
		}
		WWISE_CHECK(DataSource.GetChildItemCount(EventRoot) == EventRoot->GetChildren().Num());
		WWISE_CHECK(EventRoot->GetChildren().Num() == 1);

		FWwiseTreeItemPtr EventWorkUnit = FindChildByName(EventRoot, TEXT("Default Work Unit"));
		WWISE_CHECK(EventWorkUnit.IsValid());
		if (EventWorkUnit)
		{
			WWISE_CHECK(DataSource.FindItemFromPath(EventWorkUnit->FolderPath) == EventWorkUnit);
			WWISE_CHECK(DataSource.GetChildItemCount(EventWorkUnit) == EventWorkUnit->GetChildren().Num());
			WWISE_CHECK(EventWorkUnit->GetChildren().Num() == 1);
		}

		FWwiseTreeItemPtr DialogueRoot = DataSource.GetRootItem(EWwiseItemType::DialogueEvent);
		WWISE_CHECK(DialogueRoot.IsValid());
		if (!DialogueRoot)
		{
			return;
		}
		WWISE_CHECK(DataSource.GetChildItemCount(DialogueRoot) == DialogueRoot->GetChildren().Num());

		FWwiseTreeItemPtr DialogueWorkUnit = FindChildByName(DialogueRoot, TEXT("Default Work Unit"));
		WWISE_CHECK(DialogueWorkUnit.IsValid());
		if (DialogueWorkUnit)
		{
			WWISE_CHECK(DataSource.FindItemFromPath(DialogueWorkUnit->FolderPath) == DialogueWorkUnit);
		}
	}
}

WWISE_TEST_CASE(WwiseProjectDatabaseSource_FindItem_DisambiguatesShortIdCollisions, "Wwise::AudiokineticTools::WwiseProjectDatabaseSource::FindItem_DisambiguatesShortIdCollisions", "[EditorContext][ProductFilter]")
{
	SECTION("Two candidates share a ShortId; the one matching ItemType+GroupId wins, even if inserted first")
	{
		FWwiseTreeItemPtr CandidateA = MakeShared<FWwiseTreeItem>(TEXT("Happy"), TEXT("\\States\\Default Work Unit\\Happy"), nullptr, EWwiseItemType::State, FGuid::NewGuid(), 5000);
		CandidateA->GroupId = 100;
		FWwiseTreeItemPtr CandidateB = MakeShared<FWwiseTreeItem>(TEXT("Happy"), TEXT("\\Switches\\Default Work Unit\\Happy"), nullptr, EWwiseItemType::Switch, FGuid::NewGuid(), 5000);
		CandidateB->GroupId = 200;

		TMultiMap<uint32, FWwiseTreeItemPtr> ByShortId;
		ByShortId.Add(5000, CandidateA);
		ByShortId.Add(5000, CandidateB);

		FWwiseProjectDatabaseDataSourceTest DataSource;
		DataSource.SeedLookupMaps_ForTesting(TMap<FGuid, FWwiseTreeItemPtr>{}, ByShortId, TMultiMap<FString, FWwiseTreeItemPtr>{});

		FWwiseTreeItemPtr Query = MakeShared<FWwiseTreeItem>(TEXT("Happy"), TEXT("\\Query"), nullptr, EWwiseItemType::Switch, FGuid::NewGuid(), 5000);
		Query->GroupId = 200;

		FWwiseTreeItemPtr Result = DataSource.FindItem(Query);
		WWISE_CHECK(Result == CandidateB);
	}

	SECTION("Neither candidate matches ItemType+GroupId; FindItem falls back to whichever MultiFind returns first")
	{
		FWwiseTreeItemPtr CandidateC = MakeShared<FWwiseTreeItem>(TEXT("Sad"), TEXT("\\States\\Default Work Unit\\Sad"), nullptr, EWwiseItemType::State, FGuid::NewGuid(), 6000);
		CandidateC->GroupId = 100;
		FWwiseTreeItemPtr CandidateD = MakeShared<FWwiseTreeItem>(TEXT("Sad"), TEXT("\\Switches\\Default Work Unit\\Sad"), nullptr, EWwiseItemType::Switch, FGuid::NewGuid(), 6000);
		CandidateD->GroupId = 200;

		TMultiMap<uint32, FWwiseTreeItemPtr> ByShortId;
		ByShortId.Add(6000, CandidateC);
		ByShortId.Add(6000, CandidateD);

		TMultiMap<uint32, FWwiseTreeItemPtr> ByShortIdCopy = ByShortId;

		FWwiseProjectDatabaseDataSourceTest DataSource;
		DataSource.SeedLookupMaps_ForTesting(TMap<FGuid, FWwiseTreeItemPtr>{}, ByShortId, TMultiMap<FString, FWwiseTreeItemPtr>{});

		FWwiseTreeItemPtr Query = MakeShared<FWwiseTreeItem>(TEXT("Sad"), TEXT("\\Query"), nullptr, EWwiseItemType::Bus, FGuid::NewGuid(), 6000);
		Query->GroupId = 999;

		TArray<FWwiseTreeItemPtr> ExpectedCandidates;
		ByShortIdCopy.MultiFind(6000, ExpectedCandidates);
		WWISE_CHECK(ExpectedCandidates.Num() == 2);

		FWwiseTreeItemPtr Result = DataSource.FindItem(Query);
		WWISE_CHECK(ExpectedCandidates.Num() == 2 && Result == ExpectedCandidates[0]);
		WWISE_CHECK(Result == CandidateD);
	}
}

WWISE_TEST_CASE(WwiseProjectDatabaseSource_LoadFilteredRootItem_CopyTreeAndFilterTree, "Wwise::AudiokineticTools::WwiseProjectDatabaseSource::LoadFilteredRootItem_CopyTreeAndFilterTree", "[EditorContext][ProductFilter]")
{
	SECTION("Full copy (no OldFilterText relationship) filters out the non-matching leaf but keeps its ancestor folders")
	{
		TSharedPtr<WwisePlatformDataStructure> PlatformData = BuildFixture(MakeFolderReuseJson());
		FWwiseProjectDatabaseDataSourceTest DataSource;
		FWwiseTreeBuildContext Ctx = DataSource.MakeTreeBuildContext_ForTesting();
		DataSource.BuildEvents_ForTesting(PlatformData->Events, Ctx);

		FWwiseTreeItemPtr OriginalRoot = DataSource.GetRootItem(EWwiseItemType::Event);
		WWISE_CHECK(OriginalRoot.IsValid());

		FWwiseTreeItemPtr FilteredRoot = DataSource.LoadFilteredRootItem(EWwiseItemType::Event, MakeTextFilter(TEXT("Fire")));
		WWISE_CHECK(FilteredRoot.IsValid());
		if (!FilteredRoot)
		{
			return;
		}
		WWISE_CHECK(FilteredRoot != OriginalRoot);
		WWISE_CHECK(FilteredRoot->ItemType == EWwiseItemType::Folder);

		FWwiseTreeItemPtr WorkUnit = FindChildByName(FilteredRoot, TEXT("Default Work Unit"));
		WWISE_CHECK(WorkUnit.IsValid());
		FWwiseTreeItemPtr Weapons = FindChildByName(WorkUnit, TEXT("Weapons"));
		WWISE_CHECK(Weapons.IsValid());
		if (Weapons)
		{
			WWISE_CHECK(Weapons->GetChildren().Num() == 1);
			WWISE_CHECK(FindChildByName(Weapons, TEXT("Rifle_Fire")).IsValid());
			WWISE_CHECK(!FindChildByName(Weapons, TEXT("Rifle_Reload")).IsValid());
		}

		// The original unfiltered tree is untouched by CopyTree/FilterTree.
		WWISE_CHECK(OriginalRoot->GetChildren().Num() == 1);
		FWwiseTreeItemPtr OriginalWorkUnit = FindChildByName(OriginalRoot, TEXT("Default Work Unit"));
		FWwiseTreeItemPtr OriginalWeapons = FindChildByName(OriginalWorkUnit, TEXT("Weapons"));
		WWISE_CHECK(OriginalWeapons.IsValid() && OriginalWeapons->GetChildren().Num() == 2);
	}

	SECTION("An incremental filter (extends OldFilterText) produces the same result as a fresh full copy for the same final filter text")
	{
		TSharedPtr<WwisePlatformDataStructure> PlatformDataA = BuildFixture(MakeFolderReuseJson());
		FWwiseProjectDatabaseDataSourceTest DataSourceA;
		FWwiseTreeBuildContext CtxA = DataSourceA.MakeTreeBuildContext_ForTesting();
		DataSourceA.BuildEvents_ForTesting(PlatformDataA->Events, CtxA);

		// First call establishes OldFilterText = "Ri"; second extends it to "Rifle_F".
		DataSourceA.LoadFilteredRootItem(EWwiseItemType::Event, MakeTextFilter(TEXT("Ri")));
		FWwiseTreeItemPtr ResultA = DataSourceA.LoadFilteredRootItem(EWwiseItemType::Event, MakeTextFilter(TEXT("Rifle_F")));

		TSharedPtr<WwisePlatformDataStructure> PlatformDataB = BuildFixture(MakeFolderReuseJson());
		FWwiseProjectDatabaseDataSourceTest DataSourceB;
		FWwiseTreeBuildContext CtxB = DataSourceB.MakeTreeBuildContext_ForTesting();
		DataSourceB.BuildEvents_ForTesting(PlatformDataB->Events, CtxB);

		// No prior filter call, so OldFilterText is empty - takes the full-copy path directly.
		FWwiseTreeItemPtr ResultB = DataSourceB.LoadFilteredRootItem(EWwiseItemType::Event, MakeTextFilter(TEXT("Rifle_F")));

		WWISE_CHECK(ResultA.IsValid());
		WWISE_CHECK(ResultB.IsValid());
		WWISE_CHECK(DumpTree(ResultA) == DumpTree(ResultB));
	}
}

WWISE_TEST_CASE(WwiseProjectDatabaseSource_LoadFilteredRootItem_IndexBounds, "Wwise::AudiokineticTools::WwiseProjectDatabaseSource::LoadFilteredRootItem_IndexBounds", "[EditorContext][ProductFilter]")
{
	SECTION("In-range ItemType at the last valid index returns the built root; one past it returns nullptr")
	{
		TSharedPtr<WwisePlatformDataStructure> PlatformData = BuildFixture();
		FWwiseProjectDatabaseDataSourceTest DataSource;
		FWwiseTreeBuildContext Ctx = DataSource.MakeTreeBuildContext_ForTesting();
		static_assert(EWwiseItemType::LastWwiseBrowserType == EWwiseItemType::AudioDeviceShareSet,
			"LastWwiseBrowserType changed. Update this test to use the new last type.");
		DataSource.BuildAudioDevices_ForTesting(PlatformData->AudioDevices, Ctx);

		FWwiseTreeItemPtr EmptyFilterResult = DataSource.LoadFilteredRootItem(EWwiseItemType::LastWwiseBrowserType, MakeTextFilter(TEXT("")));
		WWISE_CHECK(EmptyFilterResult.IsValid());
		WWISE_CHECK(EmptyFilterResult == DataSource.GetRootItem(EWwiseItemType::LastWwiseBrowserType));

		const EWwiseItemType OutOfRange = static_cast<EWwiseItemType>(static_cast<int32>(EWwiseItemType::LastWwiseBrowserType) + 1);
		WwiseProjectDatabaseLoggingUtils::AddExpectedError("Failed to get the Project Database Root Items. Index out of range.");
		FWwiseTreeItemPtr OutOfRangeResult = DataSource.LoadFilteredRootItem(OutOfRange, MakeTextFilter(TEXT("")));
		WWISE_CHECK(!OutOfRangeResult.IsValid());
	}
}

#endif // WWISE_UNIT_TESTS
