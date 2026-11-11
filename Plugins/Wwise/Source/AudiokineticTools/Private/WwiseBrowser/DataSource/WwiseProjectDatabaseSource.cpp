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

#include "WwiseProjectDatabaseSource.h"

#include <Wwise/Stats/AudiokineticTools.h>

#include "IAudiokineticTools.h"
#include "AssetManagement/AkAssetDatabase.h"
#include "Async/Async.h"
#include "Wwise/WwiseTreeItem.h"
#include "Wwise/WwiseProjectDatabase.h"
#include "Wwise/WwiseProjectDatabaseDelegates.h"
#include "Wwise/Metadata/WwiseMetadataProjectInfo.h"

FWwiseProjectDatabaseDataSource::FWwiseProjectDatabaseDataSource()
{
	RootItems.SetNum(static_cast<int32>(EWwiseItemType::LastWwiseBrowserType) + 1);
}

FWwiseProjectDatabaseDataSource::~FWwiseProjectDatabaseDataSource()
{
	if (OnDatabaseUpdateCompleteHandle.IsValid())
	{
		FWwiseProjectDatabaseDelegates::Get()->GetOnDatabaseUpdateCompletedDelegate().Remove(OnDatabaseUpdateCompleteHandle);
		OnDatabaseUpdateCompleteHandle.Reset();
	}
}

FText FWwiseProjectDatabaseDataSource::GetProjectName()
{
	auto* ProjectDatabase = FWwiseProjectDatabase::Get();
	if (UNLIKELY(!ProjectDatabase))
	{
		return {};
	}

	const WwiseDataStructureScopeLock DataStructure(*ProjectDatabase);
	const WwiseRefPlatform Platform = DataStructure.GetPlatform(ProjectDatabase->GetCurrentPlatform());

	if (Platform.IsValid())
	{
		if (const WwiseMetadataProjectInfo* ProjectInfo = Platform.ProjectInfo.GetProjectInfo())
		{
			return FText::FromString(*ProjectInfo->Project.Name);
		}
	}
	return {};
}

bool FWwiseProjectDatabaseDataSource::Init()
{
	OnDatabaseUpdateCompleteHandle = FWwiseProjectDatabaseDelegates::Get()->GetOnDatabaseUpdateCompletedDelegate().AddLambda([this]
	{
		AsyncTask(ENamedThreads::Type::GameThread, [this]
		{
			ProjectDatabaseUpdatedExternally.ExecuteIfBound();
		});
	});

	return true;
}

bool FWwiseProjectDatabaseDataSource::ConstructTree(bool bShouldRefresh)
{
	SCOPED_AUDIOKINETICTOOLS_EVENT_2(TEXT("FWwiseProjectDatabaseDataSource::ConstructTree"))

	UE_LOG(LogAudiokineticTools, Log, TEXT("Rebuilding tree for Wwise Browser"));

	FWwiseProjectDatabase* ProjectDatabase = FWwiseProjectDatabase::Get();
	if (UNLIKELY(!ProjectDatabase))
	{
		UE_LOG(LogAudiokineticTools, Error, TEXT("ConstructTree: ProjectDatabase not loaded"));
		return false;
	}

	TArray<FWwiseTreeItemPtr> NewRootItems;
	NewRootItems.SetNum(static_cast<int32>(EWwiseItemType::LastWwiseBrowserType) + 1);
	TMap<FGuid, FWwiseTreeItemPtr> NewAllValidTreeItemsByGuid;
	TMultiMap<uint32, FWwiseTreeItemPtr> NewAllValidTreeItemsByShortId;
	TMultiMap<FString, FWwiseTreeItemPtr> NewAllValidTreeItemsByName;
	TMap<FString, FWwiseTreeItemPtr> NewNodesByPath;
	FWwiseTreeBuildContext Ctx{ NewRootItems, NewAllValidTreeItemsByGuid, NewAllValidTreeItemsByShortId, NewAllValidTreeItemsByName, NewNodesByPath };

	{
		const WwiseDataStructureScopeLock DataStructure(*ProjectDatabase);
		if (DataStructure.GetCurrentPlatformData() == nullptr)
		{
			return false;
		}
		const WwiseEventGlobalIdsMap& Events = DataStructure.GetEvents();
		const WwiseDialogueEventGlobalIdsMap& DialogueEvents = DataStructure.GetDialogueEvents();
		const WwiseBusGlobalIdsMap& Busses = DataStructure.GetBusses();
		const WwiseAuxBusGlobalIdsMap& AuxBusses = DataStructure.GetAuxBusses();
		const WwiseAcousticTextureGlobalIdsMap& AcousticTextures = DataStructure.GetAcousticTextures();
		const WwiseAudioDeviceGlobalIdsMap& AudioDevices = DataStructure.GetAudioDevices();
		const WwiseStateGroupGlobalIdsMap& StateGroups = DataStructure.GetStateGroups();
		const WwiseStateGlobalIdsMap& States = DataStructure.GetStates();
		const WwiseSwitchGlobalIdsMap& Switches = DataStructure.GetSwitches();
		const WwiseGameParameterGlobalIdsMap& GameParameters = DataStructure.GetGameParameters();
		const WwiseTriggerGlobalIdsMap& Triggers = DataStructure.GetTriggers();
		const WwisePluginShareSetGlobalIdsMap& EffectShareSets = DataStructure.GetPluginShareSets();
		const WwiseSwitchGroupGlobalIdsMap SwitchGroups = DataStructure.GetSwitchGroups();

		BuildEvents(Events, Ctx);
		if (IsRebuildCancelled())
		{
			return false;
		}
		BuildBusses(Busses, Ctx);
		if (IsRebuildCancelled())
		{
			return false;
		}
		BuildAuxBusses(AuxBusses, Ctx);
		if (IsRebuildCancelled())
		{
			return false;
		}
		BuildSwitchGroups(SwitchGroups, Ctx);
		if (IsRebuildCancelled())
		{
			return false;
		}
		BuildSwitches(Switches, Ctx);
		if (IsRebuildCancelled())
		{
			return false;
		}
		BuildStateGroups(StateGroups, Ctx);
		if (IsRebuildCancelled())
		{
			return false;
		}
		BuildStates(States, Ctx);
		if (IsRebuildCancelled())
		{
			return false;
		}
		BuildGameParameters(GameParameters, Ctx);
		if (IsRebuildCancelled())
		{
			return false;
		}
		BuildDialogueEvents(DialogueEvents, Ctx);
		if (IsRebuildCancelled())
		{
			return false;
		}
		BuildEffectShareSets(EffectShareSets, Ctx);
		if (IsRebuildCancelled())
		{
			return false;
		}
		BuildTriggers(Triggers, Ctx);
		if (IsRebuildCancelled())
		{
			return false;
		}
		BuildAcousticTextures(AcousticTextures, Ctx);
		if (IsRebuildCancelled())
		{
			return false;
		}
		BuildAudioDevices(AudioDevices, Ctx);
	}

	if (IsRebuildCancelled())
	{
		return false;
	}

	{
		FScopeLock AutoLock(&TreeDataLock);
		RootItems = MoveTemp(NewRootItems);
		AllValidTreeItemsByGuid = MoveTemp(NewAllValidTreeItemsByGuid);
		AllValidTreeItemsByShortId = MoveTemp(NewAllValidTreeItemsByShortId);
		AllValidTreeItemsByName = MoveTemp(NewAllValidTreeItemsByName);
		NodesByPath = MoveTemp(NewNodesByPath);
	}

	if(bShouldRefresh)
	{
		ProjectDatabaseDataSourceRefreshed.ExecuteIfBound();
	}
	return true;
}

FWwiseTreeItemPtr FWwiseProjectDatabaseDataSource::ConstructTreeRoot(EWwiseItemType Type)
{
	ConstructTree(false);

	{
		FScopeLock AutoLock(&TreeDataLock);
		return RootItems[static_cast<int32>(Type)];
	}
}

int32 FWwiseProjectDatabaseDataSource::LoadChildren(
	const FGuid& InParentId, const FString& InParentPath, TArray<FWwiseTreeItemPtr>& OutChildren)
{
	FScopeLock AutoLock(&TreeDataLock);
	FWwiseTreeItemPtr* TreeItem = NodesByPath.Find(InParentPath);

	if (TreeItem && TreeItem->IsValid())
	{
		OutChildren = TreeItem->Get()->GetChildren();
	}

	else
	{
		OutChildren = {};
	}

	return OutChildren.Num();
}

int32 FWwiseProjectDatabaseDataSource::LoadChildren(FWwiseTreeItemPtr InParentItem)
{
	if (!InParentItem)
	{
		return 0;
	}


	// For now, we have to construct the whole tree at once, so this can't be individually loaded. So we return what we have.
	return InParentItem->GetChildren().Num();
}

int32 FWwiseProjectDatabaseDataSource::GetChildItemCount(const FWwiseTreeItemPtr& InParentItem)
{
	return InParentItem->GetChildren().Num();
}

FWwiseTreeItemPtr FWwiseProjectDatabaseDataSource::GetRootItem(EWwiseItemType RootType)
{
	check(RootType <= EWwiseItemType::LastWwiseBrowserType)

	FScopeLock AutoLock(&TreeDataLock);
	if (static_cast<int32>(RootType) > RootItems.Num() - 1)
	{
		UE_LOG(LogAudiokineticTools, Error, TEXT("Failed to get the Project Database Root Items. Index out of range."));
		return nullptr;
	}

	return RootItems[static_cast<int32>(RootType)];
}

FWwiseTreeItemPtr FWwiseProjectDatabaseDataSource::LoadFilteredRootItem(EWwiseItemType ItemType,
	TSharedPtr<StringFilter> CurrentFilter)
{
	SCOPED_AUDIOKINETICTOOLS_EVENT_3(TEXT("FWwiseProjectDatabaseDataSource::LoadFilteredRootItem"))
	check(CurrentFilter.IsValid())

	FWwiseTreeItemPtr CurrentTreeRootItem;

	{
		FScopeLock AutoLock(&TreeDataLock);
		if (static_cast<int32>(ItemType) >= RootItems.Num())
		{
			UE_LOG(LogAudiokineticTools, Error, TEXT("Failed to get the Project Database Root Items. Index out of range."));
			return nullptr;
		}

		CurrentTreeRootItem = RootItems[static_cast<int32>(ItemType)];
		FString CurrentFilterText = CurrentFilter->GetRawFilterText().ToString();

		if (!CurrentFilterText.IsEmpty() && CurrentTreeRootItem.IsValid())
		{
			FWwiseTreeItemPtr FilteredTreeRootItem = MakeShared<FWwiseTreeItem>(WwiseItemType::BrowserDisplayNames[static_cast<int32>(ItemType)],
				CurrentTreeRootItem->FolderPath, nullptr, EWwiseItemType::Folder, FGuid(CurrentTreeRootItem->ItemId), CurrentTreeRootItem->ShortId);
			FilteredTreeRootItem->ChildCountInWwise = CurrentTreeRootItem->ChildCountInWwise;

			if (!OldFilterText.IsEmpty() && CurrentFilterText.StartsWith(OldFilterText))
			{
				CopyTree(CurrentTreeRootItem, FilteredTreeRootItem);
			}

			else
			{
				CopyTree(RootItems[static_cast<int32>(ItemType)], FilteredTreeRootItem);
			}

			FilterTree(FilteredTreeRootItem, CurrentFilter);
			OldFilterText = CurrentFilterText;
			return FilteredTreeRootItem;
		}

		if (RootItems[static_cast<int32>(ItemType)])
		{
			return RootItems[static_cast<int32>(ItemType)];
		}

		return nullptr;
	}
}

FWwiseTreeItemPtr FWwiseProjectDatabaseDataSource::FindItemFromPath(const FString& InCurrentItemPath)
{
	FScopeLock AutoLock(&TreeDataLock);
	FWwiseTreeItemPtr* FoundItem = NodesByPath.Find(InCurrentItemPath);
	if (FoundItem)
	{
		return *FoundItem;
	}

	return nullptr;
}

static FWwiseTreeItemPtr DisambiguateTreeItemCandidates(const TArray<FWwiseTreeItemPtr>& Candidates, const FWwiseTreeItemPtr& InItem)
{
	for (const FWwiseTreeItemPtr& Candidate : Candidates)
	{
		if (Candidate->ItemType == InItem->ItemType && Candidate->GroupId == InItem->GroupId)
		{
			return Candidate;
		}
	}
	return Candidates.Num() > 0 ? Candidates[0] : nullptr;
}

FWwiseTreeItemPtr FWwiseProjectDatabaseDataSource::FindItem(const FWwiseTreeItemPtr InItem)
{
	FScopeLock AutoLock(&TreeDataLock);
	if (FWwiseTreeItemPtr* Found = AllValidTreeItemsByGuid.Find(InItem->ItemId))
	{
		return *Found;
	}
	if (InItem->ShortId > AK_INVALID_UNIQUE_ID)
	{
		TArray<FWwiseTreeItemPtr> Candidates;
		AllValidTreeItemsByShortId.MultiFind(InItem->ShortId, Candidates);
		if (FWwiseTreeItemPtr Found = DisambiguateTreeItemCandidates(Candidates, InItem))
		{
			return Found;
		}
	}
	{
		TArray<FWwiseTreeItemPtr> Candidates;
		AllValidTreeItemsByName.MultiFind(InItem->DisplayName, Candidates);
		if (FWwiseTreeItemPtr Found = DisambiguateTreeItemCandidates(Candidates, InItem))
		{
			return Found;
		}
	}
	return nullptr;
}

void FWwiseProjectDatabaseDataSource::BuildEvents(const WwiseEventGlobalIdsMap& Events, FWwiseTreeBuildContext& Ctx)
{
	SCOPED_AUDIOKINETICTOOLS_EVENT_3(TEXT("FWwiseProjectDatabaseDataSource::BuildEvents"))

	const auto& FolderItem = MakeShared<FWwiseTreeItem>(WwiseItemType::EventsBrowserName, TEXT("\\") + WwiseItemType::FolderNames[static_cast<int32>(EWwiseItemType::Event)], nullptr, EWwiseItemType::Folder, FGuid());
	Ctx.NodesByPath.Add(FolderItem->FolderPath, FolderItem);

	Ctx.RootItems[static_cast<int32>(EWwiseItemType::Event)] = FolderItem;

	int32 ProcessedCount = 0;
	for (const auto& Event : Events)
	{
		if (CancelToken.ShouldCancelAtInterval(ProcessedCount, RebuildCancellationCheckInterval))
		{
			return;
		}
		WwiseDBPair<const WwiseDatabaseLocalizableIdKey, WwiseRefEvent> EventPair(Event);
		const auto& WwiseItem = EventPair.GetSecond().GetEvent();
		UE_LOG(LogAudiokineticTools, VeryVerbose, TEXT("Event Name: %s"), *FWwiseStringConverter::ToFString(WwiseItem->Name));

		WwiseMetadataBasicReference EventRef = WwiseMetadataBasicReference(WwiseItem->Id, WwiseItem->Name, WwiseItem->ObjectPath, WwiseItem->GUID);
		if (!BuildFolderHierarchy(EventRef, EWwiseItemType::Event, FolderItem, Ctx))
		{
			UE_LOG(LogAudiokineticTools, Error, TEXT("Failed to place %s in the Wwise Browser"), *FWwiseStringConverter::ToFString(WwiseItem->ObjectPath));
		}
	}

	FolderItem->ChildCountInWwise = FolderItem->GetChildren().Num();
}

void FWwiseProjectDatabaseDataSource::BuildDialogueEvents(const WwiseDialogueEventGlobalIdsMap& DialogueEvents, FWwiseTreeBuildContext& Ctx)
{
	SCOPED_AUDIOKINETICTOOLS_EVENT_3(TEXT("FWwiseProjectDatabaseDataSource::BuildDialogueEvents"))

	const auto& FolderItem = MakeShared<FWwiseTreeItem>(WwiseItemType::DialogueEventsBrowserName, TEXT("\\") + WwiseItemType::FolderNames[static_cast<int32>(EWwiseItemType::DialogueEvent)], nullptr, EWwiseItemType::Folder, FGuid());
	Ctx.NodesByPath.Add(FolderItem->FolderPath, FolderItem);

	Ctx.RootItems[static_cast<int32>(EWwiseItemType::DialogueEvent)] = FolderItem;

	int32 ProcessedCount = 0;
	for (const auto& DialogueEvent : DialogueEvents)
	{
		if (CancelToken.ShouldCancelAtInterval(ProcessedCount, RebuildCancellationCheckInterval))
		{
			return;
		}
		WwiseDBPair<const WwiseDatabaseLocalizableIdKey, WwiseRefDialogueEvent> EventPair(DialogueEvent);
		const auto& WwiseItem = EventPair.GetSecond().GetDialogueEvent();
		UE_LOG(LogAudiokineticTools, VeryVerbose, TEXT("DialogueEvent Name: %s"), *FWwiseStringConverter::ToFString(WwiseItem->Name));

		WwiseMetadataBasicReference DialogueEventRef = WwiseMetadataBasicReference(WwiseItem->Id, WwiseItem->Name, WwiseItem->ObjectPath, WwiseItem->GUID);
		if (!BuildFolderHierarchy(DialogueEventRef, EWwiseItemType::DialogueEvent, FolderItem, Ctx))
		{
			UE_LOG(LogAudiokineticTools, Error, TEXT("Failed to place %s in the Wwise Browser"), *FWwiseStringConverter::ToFString(WwiseItem->ObjectPath));
		}
	}

	FolderItem->ChildCountInWwise = FolderItem->GetChildren().Num();
}

void FWwiseProjectDatabaseDataSource::BuildBusses(const WwiseBusGlobalIdsMap& Busses, FWwiseTreeBuildContext& Ctx)
{
	SCOPED_AUDIOKINETICTOOLS_EVENT_3(TEXT("FWwiseProjectDatabaseDataSource::BuildBusses"))

	const auto& FolderItem = MakeShared<FWwiseTreeItem>(WwiseItemType::BussesBrowserName, TEXT("\\") + WwiseItemType::FolderNames[static_cast<int32>(EWwiseItemType::AuxBus)], nullptr, EWwiseItemType::Folder, FGuid());
	Ctx.NodesByPath.Add(FolderItem->FolderPath, FolderItem);

	Ctx.RootItems[static_cast<int32>(EWwiseItemType::AuxBus)] = FolderItem;

	int32 ProcessedCount = 0;
	for (auto& Bus : Busses)
	{
		if (CancelToken.ShouldCancelAtInterval(ProcessedCount, RebuildCancellationCheckInterval))
		{
			return;
		}
		WwiseDBPair<const WwiseDatabaseLocalizableIdKey, WwiseRefBus> BusPair(Bus);
		const auto& WwiseItem = BusPair.GetSecond().GetBus();
		UE_LOG(LogAudiokineticTools, VeryVerbose, TEXT("Bus Name: %s"), *FWwiseStringConverter::ToFString(WwiseItem->Name));

		if (!BuildFolderHierarchy(*WwiseItem, EWwiseItemType::Bus, FolderItem, Ctx))
		{
			UE_LOG(LogAudiokineticTools, Error, TEXT("Failed to place %s in the Wwise Browser"), *FWwiseStringConverter::ToFString(WwiseItem->ObjectPath));
		}
	}

	FolderItem->ChildCountInWwise = FolderItem->GetChildren().Num();
}

void FWwiseProjectDatabaseDataSource::BuildAuxBusses(const WwiseAuxBusGlobalIdsMap& AuxBusses, FWwiseTreeBuildContext& Ctx)
{
	SCOPED_AUDIOKINETICTOOLS_EVENT_3(TEXT("FWwiseProjectDatabaseDataSource::BuildEvents"))

	FWwiseTreeItemPtr FolderItem = Ctx.RootItems[static_cast<int32>(EWwiseItemType::AuxBus)];

	int32 ProcessedCount = 0;
	for (const auto& AuxBus : AuxBusses)
	{
		if (CancelToken.ShouldCancelAtInterval(ProcessedCount, RebuildCancellationCheckInterval))
		{
			return;
		}
		WwiseDBPair<const WwiseDatabaseLocalizableIdKey, WwiseRefAuxBus> AuxBusPair(AuxBus);
		const auto& WwiseItem = AuxBusPair.GetSecond().GetAuxBus();
		UE_LOG(LogAudiokineticTools, VeryVerbose, TEXT("Aux Bus Name: %s"), *FWwiseStringConverter::ToFString(WwiseItem->Name));

		if (!BuildFolderHierarchy(*WwiseItem, EWwiseItemType::AuxBus, FolderItem, Ctx))
		{
			UE_LOG(LogAudiokineticTools, Error, TEXT("Failed to place %s in the Wwise Browser"), *FWwiseStringConverter::ToFString(WwiseItem->ObjectPath));
		}
	}
}

void FWwiseProjectDatabaseDataSource::BuildAcousticTextures(const WwiseAcousticTextureGlobalIdsMap& AcousticTextures, FWwiseTreeBuildContext& Ctx)
{
	SCOPED_AUDIOKINETICTOOLS_EVENT_3(TEXT("FWwiseProjectDatabaseDataSource::BuildAcousticTextures"))

	const auto& FolderItem = MakeShared<FWwiseTreeItem>(WwiseItemType::AcousticTexturesBrowserName, TEXT("\\") + WwiseItemType::FolderNames[static_cast<int32>(EWwiseItemType::AcousticTexture)], nullptr, EWwiseItemType::Folder, FGuid());
	Ctx.NodesByPath.Add(FolderItem->FolderPath, FolderItem);

	Ctx.RootItems[static_cast<int32>(EWwiseItemType::AcousticTexture)] = FolderItem;

	int32 ProcessedCount = 0;
	for (const auto& AcousticTexture : AcousticTextures)
	{
		if (CancelToken.ShouldCancelAtInterval(ProcessedCount, RebuildCancellationCheckInterval))
		{
			return;
		}
		WwiseDBPair<const WwiseDatabaseLocalizableIdKey, WwiseRefAcousticTexture> AcousticTexturePair(AcousticTexture);
		const WwiseMetadataAcousticTexture* WwiseItem = AcousticTexturePair.GetSecond().GetAcousticTexture();
		UE_LOG(LogAudiokineticTools, VeryVerbose, TEXT("Acoustic Texture Name: %s"), *FWwiseStringConverter::ToFString(WwiseItem->Name));

		if (!BuildFolderHierarchy(*WwiseItem, EWwiseItemType::AcousticTexture, FolderItem, Ctx))
		{
			UE_LOG(LogAudiokineticTools, Error, TEXT("Failed to place %s in the Wwise Browser"), *FWwiseStringConverter::ToFString(WwiseItem->ObjectPath));
		}
	}

	FolderItem->ChildCountInWwise = FolderItem->GetChildren().Num();
}

void FWwiseProjectDatabaseDataSource::BuildAudioDevices(const WwiseAudioDeviceGlobalIdsMap& AudioDevices, FWwiseTreeBuildContext& Ctx)
{
	const auto& FolderItem = MakeShared<FWwiseTreeItem>(WwiseItemType::AudioDeviceShareSetBrowserName, TEXT("\\") + WwiseItemType::FolderNames[static_cast<int32>(EWwiseItemType::AudioDeviceShareSet)], nullptr, EWwiseItemType::Folder, FGuid());
	Ctx.NodesByPath.Add(FolderItem->FolderPath, FolderItem);

	Ctx.RootItems[static_cast<int32>(EWwiseItemType::AudioDeviceShareSet)] = FolderItem;

	int32 ProcessedCount = 0;
	for (const auto& AudioDevice : AudioDevices)
	{
		if (CancelToken.ShouldCancelAtInterval(ProcessedCount, RebuildCancellationCheckInterval))
		{
			return;
		}
		WwiseDBPair<const WwiseDatabaseLocalizableIdKey, WwiseRefAudioDevice> AudioDevicePair(AudioDevice);
		const auto& WwiseItem = AudioDevicePair.GetSecond().GetPlugin();
		UE_LOG(LogAudiokineticTools, VeryVerbose, TEXT("Audio Device Name: %s"), *FWwiseStringConverter::ToFString(WwiseItem->Name));

		if (!BuildFolderHierarchy(*WwiseItem, EWwiseItemType::AudioDeviceShareSet, FolderItem, Ctx))
		{
			UE_LOG(LogAudiokineticTools, Error, TEXT("Failed to place %s in the Wwise Picker"), *FWwiseStringConverter::ToFString(*WwiseItem->ObjectPath));
		}
	}

	FolderItem->ChildCountInWwise = FolderItem->GetChildren().Num();
}

void FWwiseProjectDatabaseDataSource::BuildStateGroups(const WwiseStateGroupGlobalIdsMap& StateGroups, FWwiseTreeBuildContext& Ctx)
{
	SCOPED_AUDIOKINETICTOOLS_EVENT_3(TEXT("FWwiseProjectDatabaseDataSource::BuildStateGroups"))

	const auto& FolderItem = MakeShared<FWwiseTreeItem>(WwiseItemType::StatesBrowserName, TEXT("\\") + WwiseItemType::FolderNames[static_cast<int32>(EWwiseItemType::State)], nullptr, EWwiseItemType::Folder, FGuid());
	Ctx.NodesByPath.Add(FolderItem->FolderPath, FolderItem);

	Ctx.RootItems[static_cast<int32>(EWwiseItemType::State)] = FolderItem;

	int32 ProcessedCount = 0;
	for (const auto& StateGroup : StateGroups)
	{
		if (CancelToken.ShouldCancelAtInterval(ProcessedCount, RebuildCancellationCheckInterval))
		{
			return;
		}
		WwiseDBPair<const WwiseDatabaseLocalizableIdKey, WwiseRefStateGroup> StateGroupPair(StateGroup);
		const auto& WwiseItem = StateGroupPair.GetSecond().GetStateGroup();
		UE_LOG(LogAudiokineticTools, VeryVerbose, TEXT("State Group Name: %s"), *FWwiseStringConverter::ToFString(WwiseItem->Name));

		if (!BuildFolderHierarchy(*WwiseItem, EWwiseItemType::StateGroup, FolderItem, Ctx))
		{
			UE_LOG(LogAudiokineticTools, Error, TEXT("Failed to place %s in the Wwise Browser"), *FWwiseStringConverter::ToFString(WwiseItem->ObjectPath));
		}
	}

	FolderItem->ChildCountInWwise = FolderItem->GetChildren().Num();
}

void FWwiseProjectDatabaseDataSource::BuildStates(const WwiseStateGlobalIdsMap& States, FWwiseTreeBuildContext& Ctx)
{
	SCOPED_AUDIOKINETICTOOLS_EVENT_3(TEXT("FWwiseProjectDatabaseDataSource::BuildEvents"))

	FWwiseTreeItemPtr StateGroupFolderItem = Ctx.RootItems[static_cast<int32>(EWwiseItemType::State)];

	int32 ProcessedCount = 0;
	for (const auto& State : States)
	{
		if (CancelToken.ShouldCancelAtInterval(ProcessedCount, RebuildCancellationCheckInterval))
		{
			return;
		}
		WwiseDBPair<const WwiseDatabaseLocalizableGroupValueKey, WwiseRefState> StatePair(State);
		const auto& WwiseItem = StatePair.GetSecond().GetState();

		if (!BuildFolderHierarchy(*WwiseItem, EWwiseItemType::State, StateGroupFolderItem, Ctx, StatePair.GetSecond().GetStateGroup()->Id))
		{
			UE_LOG(LogAudiokineticTools, Error, TEXT("Failed to place %s in the Wwise Browser"), *FWwiseStringConverter::ToFString(WwiseItem->ObjectPath));
		}
	}
}

void FWwiseProjectDatabaseDataSource::BuildSwitchGroups(const WwiseSwitchGroupGlobalIdsMap& SwitchGroups, FWwiseTreeBuildContext& Ctx)
{
	SCOPED_AUDIOKINETICTOOLS_EVENT_3(TEXT("FWwiseProjectDatabaseDataSource::BuildEvents"))

	const auto& FolderItem = MakeShared<FWwiseTreeItem>(WwiseItemType::SwitchesBrowserName, TEXT("\\") + WwiseItemType::FolderNames[static_cast<int32>(EWwiseItemType::Switch)], nullptr, EWwiseItemType::Folder, FGuid());
	Ctx.NodesByPath.Add(FolderItem->FolderPath, FolderItem);

	Ctx.RootItems[static_cast<int32>(EWwiseItemType::Switch)] = FolderItem;

	int32 ProcessedCount = 0;
	for (const auto& SwitchGroup : SwitchGroups)
	{
		if (CancelToken.ShouldCancelAtInterval(ProcessedCount, RebuildCancellationCheckInterval))
		{
			return;
		}
		WwiseDBPair<const WwiseDatabaseLocalizableIdKey, WwiseRefSwitchGroup> SwitchGroupPair(SwitchGroup);
		const auto& WwiseItem = SwitchGroupPair.GetSecond().GetSwitchGroup();
		UE_LOG(LogAudiokineticTools, VeryVerbose, TEXT("Switch Group Name: %s"), *FWwiseStringConverter::ToFString(WwiseItem->Name));

		if (!BuildFolderHierarchy(*WwiseItem, EWwiseItemType::SwitchGroup, FolderItem, Ctx))
		{
			UE_LOG(LogAudiokineticTools, Error, TEXT("Failed to place %s in the Wwise Browser"), *FWwiseStringConverter::ToFString(WwiseItem->ObjectPath));
		}
	}

	FolderItem->ChildCountInWwise = FolderItem->GetChildren().Num();
}

void FWwiseProjectDatabaseDataSource::BuildSwitches(const WwiseSwitchGlobalIdsMap& Switches, FWwiseTreeBuildContext& Ctx)
{
	SCOPED_AUDIOKINETICTOOLS_EVENT_3(TEXT("FWwiseProjectDatabaseDataSource::BuildSwitchs"))

	FWwiseTreeItemPtr SwitchGroupFolderItem = Ctx.RootItems[static_cast<int32>(EWwiseItemType::Switch)];

	int32 ProcessedCount = 0;
	for (const auto& Switch : Switches)
	{
		if (CancelToken.ShouldCancelAtInterval(ProcessedCount, RebuildCancellationCheckInterval))
		{
			return;
		}
		WwiseDBPair<const WwiseDatabaseLocalizableGroupValueKey, WwiseRefSwitch> SwitchPair(Switch);
		const auto& WwiseItem = SwitchPair.GetSecond().GetSwitch();

		if (!BuildFolderHierarchy(*WwiseItem, EWwiseItemType::Switch, SwitchGroupFolderItem, Ctx, SwitchPair.GetSecond().GetSwitchGroup()->Id))
		{
			UE_LOG(LogAudiokineticTools, Error, TEXT("Failed to place %s in the Wwise Browser"), *FWwiseStringConverter::ToFString(WwiseItem->ObjectPath));
		}
	}
}

void FWwiseProjectDatabaseDataSource::BuildGameParameters(const WwiseGameParameterGlobalIdsMap& GameParameters, FWwiseTreeBuildContext& Ctx)
{
	SCOPED_AUDIOKINETICTOOLS_EVENT_3(TEXT("FWwiseProjectDatabaseDataSource::BuildGameParameters"))

	const auto& FolderItem = MakeShared<FWwiseTreeItem>(WwiseItemType::GameParametersBrowserName, TEXT("\\") + WwiseItemType::FolderNames[static_cast<int32>(EWwiseItemType::GameParameter)], nullptr, EWwiseItemType::Folder, FGuid());
	Ctx.NodesByPath.Add(FolderItem->FolderPath, FolderItem);

	Ctx.RootItems[static_cast<int32>(EWwiseItemType::GameParameter)] = FolderItem;

	int32 ProcessedCount = 0;
	for (const auto& GameParameter : GameParameters)
	{
		if (CancelToken.ShouldCancelAtInterval(ProcessedCount, RebuildCancellationCheckInterval))
		{
			return;
		}
		WwiseDBPair<const WwiseDatabaseLocalizableIdKey, WwiseRefGameParameter> GameParameterPair(GameParameter);
		const WwiseMetadataGameParameter* WwiseItem = GameParameterPair.GetSecond().GetGameParameter();
		UE_LOG(LogAudiokineticTools, VeryVerbose, TEXT("GameParameter Name: %s"), *FWwiseStringConverter::ToFString(WwiseItem->Name));

		if (!BuildFolderHierarchy(*WwiseItem, EWwiseItemType::GameParameter, FolderItem, Ctx))
		{
			UE_LOG(LogAudiokineticTools, Error, TEXT("Failed to place %s in the Wwise Browser"), *FWwiseStringConverter::ToFString(WwiseItem->ObjectPath));
		}
	}

	FolderItem->ChildCountInWwise = FolderItem->GetChildren().Num();
}

void FWwiseProjectDatabaseDataSource::BuildTriggers(const WwiseTriggerGlobalIdsMap& Triggers, FWwiseTreeBuildContext& Ctx)
{
	SCOPED_AUDIOKINETICTOOLS_EVENT_3(TEXT("FWwiseProjectDatabaseDataSource::BuildTriggers"))

	const auto& FolderItem = MakeShared<FWwiseTreeItem>(WwiseItemType::TriggersBrowserName, TEXT("\\") + WwiseItemType::FolderNames[static_cast<int32>(EWwiseItemType::Trigger)], nullptr, EWwiseItemType::Folder, FGuid());
	Ctx.NodesByPath.Add(FolderItem->FolderPath, FolderItem);

	Ctx.RootItems[static_cast<int32>(EWwiseItemType::Trigger)] = FolderItem;

	int32 ProcessedCount = 0;
	for (const auto& Trigger : Triggers)
	{
		if (CancelToken.ShouldCancelAtInterval(ProcessedCount, RebuildCancellationCheckInterval))
		{
			return;
		}
		WwiseDBPair<const WwiseDatabaseLocalizableIdKey, WwiseRefTrigger> TriggerPair(Trigger);
		const auto& WwiseItem = TriggerPair.GetSecond().GetTrigger();
		UE_LOG(LogAudiokineticTools, VeryVerbose, TEXT("Trigger Name: %s"), *FWwiseStringConverter::ToFString(WwiseItem->Name));

		if (!BuildFolderHierarchy(*WwiseItem, EWwiseItemType::Trigger, FolderItem, Ctx))
		{
			UE_LOG(LogAudiokineticTools, Error, TEXT("Failed to place %s in the Wwise Browser"), *FWwiseStringConverter::ToFString(WwiseItem->ObjectPath));
		}
	}

	FolderItem->ChildCountInWwise = FolderItem->GetChildren().Num();
}

void FWwiseProjectDatabaseDataSource::BuildEffectShareSets(const WwisePluginShareSetGlobalIdsMap& EffectShareSets, FWwiseTreeBuildContext& Ctx)
{
	SCOPED_AUDIOKINETICTOOLS_EVENT_3(TEXT("FWwiseProjectDatabaseDataSource::BuildEffectShareSets"))

	const auto& FolderItem = MakeShared<FWwiseTreeItem>(WwiseItemType::ShareSetsBrowserName, TEXT("\\") + WwiseItemType::FolderNames[static_cast<int32>(EWwiseItemType::EffectShareSet)], nullptr, EWwiseItemType::Folder, FGuid());
	Ctx.NodesByPath.Add(FolderItem->FolderPath, FolderItem);

	Ctx.RootItems[static_cast<int32>(EWwiseItemType::EffectShareSet)] = FolderItem;

	int32 ProcessedCount = 0;
	for (const auto& EffectShareSet : EffectShareSets)
	{
		if (CancelToken.ShouldCancelAtInterval(ProcessedCount, RebuildCancellationCheckInterval))
		{
			return;
		}
		WwiseDBPair<const WwiseDatabaseLocalizableIdKey, WwiseRefPluginShareSet> EffectShareSetPair(EffectShareSet);
		const auto& WwiseItem = EffectShareSetPair.GetSecond().GetPlugin();
		UE_LOG(LogAudiokineticTools, VeryVerbose, TEXT("ShareSet Name: %s"), *FWwiseStringConverter::ToFString(WwiseItem->Name));

		if (!BuildFolderHierarchy(*WwiseItem, EWwiseItemType::EffectShareSet, FolderItem, Ctx))
		{
			UE_LOG(LogAudiokineticTools, Error, TEXT("Failed to place %s in the Wwise Browser"), *FWwiseStringConverter::ToFString(WwiseItem->ObjectPath));
		}
	}

	FolderItem->ChildCountInWwise = FolderItem->GetChildren().Num();
}

bool FWwiseProjectDatabaseDataSource::ParseTreePath(const FString& ObjectPath, WwiseItemTreePath& OutItemPath)
{
	TArray<FString> SwitchPartsArray;
	ObjectPath.ParseIntoArray(SwitchPartsArray, TEXT("\\"));

	if (SwitchPartsArray.Num() < 3)
	{
		UE_LOG(LogAudiokineticTools, Warning, TEXT("Failed to determine object hierarchy from path: %s"), *ObjectPath);
		return false;
	}

	OutItemPath.HierarchyName = SwitchPartsArray[0];
	OutItemPath.RootFolder = SwitchPartsArray[1];
	OutItemPath.ItemName = SwitchPartsArray[SwitchPartsArray.Num() - 1];

	for (int i = 2; i < SwitchPartsArray.Num() - 1; ++i)
	{
		OutItemPath.IntermediateFolders.Add(SwitchPartsArray[i]);
	}

	return true;
}

bool FWwiseProjectDatabaseDataSource::IsContainer(EWwiseItemType ItemType) const
{
	return ItemType == EWwiseItemType::Bus || ItemType == EWwiseItemType::SwitchGroup || ItemType ==
		EWwiseItemType::StateGroup || ItemType == EWwiseItemType::AuxBus || ItemType == EWwiseItemType::MotionBus;
}

bool FWwiseProjectDatabaseDataSource::BuildFolderHierarchy(
	const WwiseMetadataBasicReference& WwiseItem, EWwiseItemType
	ItemType, const FWwiseTreeItemPtr CurrentRootFolder, FWwiseTreeBuildContext& Ctx, uint32 GroupId)
{
	const FString ItemPath = *WwiseItem.ObjectPath;
	WwiseItemTreePath TreePath;
	FWwiseTreeItemPtr ParentItem = CurrentRootFolder;

	if (ParseTreePath(ItemPath, TreePath))
	{
		FString CurrentPath = "\\" + TreePath.HierarchyName;
		TArray<FString> AllFolders = TArray<FString>{ TreePath.RootFolder };
		AllFolders.Append(TreePath.IntermediateFolders);

		for (FString FolderName : AllFolders)
		{
			CurrentPath.Append("\\").Append(FolderName);

			FWwiseTreeItemPtr FolderItem;
			FWwiseTreeItemPtr* FolderItemPtr = Ctx.NodesByPath.Find(CurrentPath);

			if (!FolderItemPtr)
			{
				FolderItem = MakeShared<FWwiseTreeItem>(FolderName, CurrentPath, ParentItem,
					EWwiseItemType::Folder, FGuid());

				Ctx.NodesByPath.Add(CurrentPath, FolderItem);
				ParentItem->AddChild(FolderItem);
				ParentItem->ChildCountInWwise = ParentItem->GetChildren().Num();
			}
			else
			{
				FolderItem = *FolderItemPtr;
			}

			ParentItem = FolderItem;
		}

		int A, B, C, D;
		WwiseItem.GUID.GetGuidValues(A, B, C, D);
		FGuid Guid(A, B, C, D);
		if (!Ctx.AllValidTreeItemsByGuid.Find(Guid))
		{
			if(auto TreeItemPtr = Ctx.NodesByPath.Find(*FWwiseStringConverter::ToFString(WwiseItem.ObjectPath)))
			{
				auto TreeItem = *TreeItemPtr;
				TreeItem->ItemType = ItemType;
				TreeItem->ItemId = Guid;
				TreeItem->ShortId = WwiseItem.Id;
				TreeItem->GroupId = GroupId;
				if(ItemType == EWwiseItemType::AuxBus)
				{
					TreeItem->WwiseItemRef = MakeShared<WwiseMetadataBasicReference>(WwiseItem.Id, WwiseItem.Name, WwiseItem.ObjectPath, WwiseItem.GUID);
				}
			}
			else
			{
				const auto& NewWwiseTreeItem = MakeShared<FWwiseTreeItem>(WwiseItem, ParentItem, ItemType);
				NewWwiseTreeItem->ShortId = WwiseItem.Id;
				NewWwiseTreeItem->GroupId = GroupId;
				ParentItem->AddChild(NewWwiseTreeItem);
				Ctx.AllValidTreeItemsByGuid.Add(NewWwiseTreeItem->ItemId, NewWwiseTreeItem);
				if (NewWwiseTreeItem->ShortId > 0)
				{
					Ctx.AllValidTreeItemsByShortId.Add(NewWwiseTreeItem->ShortId, NewWwiseTreeItem);
				}
				Ctx.AllValidTreeItemsByName.Add(NewWwiseTreeItem->DisplayName, NewWwiseTreeItem);

				if (IsContainer(ItemType))
				{
					Ctx.NodesByPath.Add(FWwiseStringConverter::ToFString(WwiseItem.ObjectPath), NewWwiseTreeItem);
				}
			}
		}

		return true;
	}
	return false;
}

void FWwiseProjectDatabaseDataSource::CopyTree(FWwiseTreeItemPtr SourceTreeItem, FWwiseTreeItemPtr DestTreeItem)
{
	for (auto& CurrItem: SourceTreeItem->GetChildren())
	{
		FWwiseTreeItemPtr NewItem = MakeShared<FWwiseTreeItem>(CurrItem->DisplayName, CurrItem->FolderPath, CurrItem->Parent.Pin(), CurrItem->ItemType, CurrItem->ItemId, CurrItem->ShortId);
		NewItem->WwiseItemRef = CurrItem->WwiseItemRef;
		DestTreeItem->AddChild(NewItem);
		DestTreeItem->ChildCountInWwise = SourceTreeItem->ChildCountInWwise;

		CopyTree(CurrItem, NewItem);
	}
}

void FWwiseProjectDatabaseDataSource::FilterTree(FWwiseTreeItemPtr TreeItem, TSharedPtr<StringFilter> SearchFilter)
{
	TArray<FWwiseTreeItemPtr> ItemsToRemove;
	for (auto& CurrItem: TreeItem->GetChildren())
	{
		FilterTree(CurrItem, SearchFilter);

		if (!SearchFilter->PassesFilter(CurrItem->DisplayName) && CurrItem->GetChildren().Num() == 0)
		{
			ItemsToRemove.Add(CurrItem);
		}
	}

	for (int32 i = 0; i < ItemsToRemove.Num(); i++)
	{
		TreeItem->RemoveChild(ItemsToRemove[i]);
	}

	TreeItem->ChildCountInWwise = TreeItem->GetChildren().Num();
}

