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

#include "WwiseBrowserDataSource.h"

#include <atomic>

#include "AssetRegistry/AssetRegistryModule.h"
#include "Async/Async.h"
#include "Editor.h"
#include "Templates/SharedPointer.h"

#include "AkAudioDevice.h"
#include "AkUnrealAssetDataHelper.h"
#include "AkWaapiUtils.h"
#include "AssetManagement/AkAssetDatabase.h"
#include "IAudiokineticTools.h"
#include "WaapiDataSource.h"
#include <Wwise/Stats/AudiokineticTools.h>
#include "WwiseBrowserRebuildCoordinator.h"
#include "WwiseProjectDatabaseSource.h"
#include "WwiseRebuildCancellationToken.h"
#include "Wwise/WwiseReconcile.h"
#include "Wwise/WwiseTreeItem.h"
#include "Wwise/WwiseProjectDatabase.h"
#include "WwiseUnrealHelper.h"

struct FMergeContext
{
	IWwiseReconcile* Reconcile = nullptr;
	FWwiseProjectDatabase* ProjectDatabase = nullptr;
	TUniquePtr<WwiseDataStructureScopeLock> DbLock;
	TSharedPtr<StringFilter> FilterText;

	FWwiseRebuildCancellationToken CancelToken;
	bool IsCancelled() const
	{
		return CancelToken.IsCancelled();
	}

	bool IsValid() const
	{
		return Reconcile && ProjectDatabase && DbLock.IsValid();
	}
};

static FMergeContext MakeMergeContext(FWwiseRebuildCancellationToken CancelToken = {})
{
	FMergeContext Ctx;
	Ctx.CancelToken = CancelToken;
	Ctx.Reconcile = IWwiseReconcile::Get();
	Ctx.ProjectDatabase = FWwiseProjectDatabase::Get();
	if (LIKELY(Ctx.Reconcile && Ctx.ProjectDatabase))
	{
		Ctx.DbLock = MakeUnique<WwiseDataStructureScopeLock>(*Ctx.ProjectDatabase);
	}
	else
	{
		UE_LOG(LogAudiokineticTools, Error, TEXT("Wwise Browser merge: could not resolve the Reconcile module or project database; item move-status will be skipped."));
	}
	return Ctx;
}

bool FSoundBankStatusFilter::IsKeptInBrowser(FWwiseTreeItemPtr& Item) const
{
	if (AreFiltersOff())
	{
		return true;
	}
	if (bFilters[NotInWwise] && Item->IsNotInWwiseOrSoundBank())
	{
		return true;
	}

	if (bFilters[DeletedInWwise] && Item->IsDeletedInWwise())
	{
		return true;
	}

	if (bFilters[NewInWwise] && Item->IsNewInWwise())
	{
		return true;
	}

	if (bFilters[RenamedInWwise] && Item->IsRenamedInWwise())
	{
		return true;
	}

	if (bFilters[MovedInWwise] && Item->IsMovedInWwise())
	{
		return true;
	}

	if (bFilters[UpToDate] && Item->IsSoundBankUpToDate())
	{
		return true;
	}

	return false;
}

bool FSoundBankStatusFilter::AreFiltersOff() const
{
	for (auto filter : bFilters)
	{
		if (filter)
		{
			return false;
		}
	}
	return true;
}

bool FUAssetStatusFilter::IsKeptInBrowser(FWwiseTreeItemPtr& Item) const
{
	if (AreFiltersOff())
	{
		return true;
	}
	if(bFilters[UAssetMissing] && Item->IsUAssetMissing())
	{
		return true;
	}

	if(bFilters[UAssetOrphaned] && Item->IsUAssetOrphaned())
	{
		return true;
	}

	if(bFilters[NotInSoundBankOrUnreal] && Item->IsNotInSoundBankOrUnreal())
	{
		return true;
	}

	if(bFilters[MultipleUAssets] && Item->HasMultipleUAssets())
	{
		return true;
	}

	if (bFilters[RenamedInSoundBank] && Item->IsRenamedInSoundBank())
	{
		return true;
	}

	if (bFilters[UAssetNeedsUpdate] && Item->IsUAssetOutOfDate())
	{
		return true;
	}

	if(bFilters[UAssetUpToDate] && Item->IsUAssetUpToDate())
	{
		return true;
	}

	return false;
}

bool FUAssetStatusFilter::AreFiltersOff() const
{
	for (auto filter : bFilters)
	{
		if (filter)
		{
			return false;
		}
	}
	return true;
}

bool FWwiseTypeFilter::IsKeptInBrowser(FWwiseTreeItemPtr& Item) const
{
	if(AreFiltersOff())
	{
		return true;
	}
	for(int i = 0; i < NumberOfWwiseTypes; i++)
	{
		if(bFilters[i] && Item->ItemType == GetExpectedType((EWwiseTypeFilter)i))
		{
			return true;
		}
	}
	return false;
}

bool FWwiseTypeFilter::AreFiltersOff() const
{
	for(auto bFilter : bFilters)
	{
		if(bFilter)
		{
			return false;
		}
	}
	return true;
}

EWwiseItemType FWwiseTypeFilter::GetExpectedType(EWwiseTypeFilter Filter) const
{
	switch(Filter)
	{
	case AcousticTexture:
		return EWwiseItemType::AcousticTexture;
	case AudioDeviceShareSet:
		return EWwiseItemType::AudioDeviceShareSet;
	case DialogueEvents:
		return EWwiseItemType::DialogueEvent;
	case Effects:
		return EWwiseItemType::EffectShareSet;
	case Events:
		return EWwiseItemType::Event;
	case GameParameters:
		return EWwiseItemType::GameParameter;
	case MasterMixerHierarchy:
		return EWwiseItemType::AuxBus;
	case State:
		return EWwiseItemType::State;
	case Switch:
		return EWwiseItemType::Switch;
	case Trigger:
		return EWwiseItemType::Trigger;
	default:
		break;
	}
	return EWwiseItemType::None;
}

FWwiseBrowserDataSource::FWwiseBrowserDataSource()
{
	WaapiDataSource = MakeUnique<FWaapiDataSource>();
	ProjectDBDataSource = MakeUnique<FWwiseProjectDatabaseDataSource>();
	UAssetDataSource = MakeUnique<FUAssetDataSource>();

	WaapiDataSource->Init();
	WaapiDataSource->WaapiDataSourceRefreshed.BindRaw(this, &FWwiseBrowserDataSource::OnWaapiDataSourceRefreshed);
	WaapiDataSource->WwiseSelectionChange.BindRaw(this, &FWwiseBrowserDataSource::OnWwiseSelectionChange);
	WaapiDataSource->WwiseExpansionChange.BindRaw(this, &FWwiseBrowserDataSource::OnWwiseExpansionChange);

	ProjectDBDataSource->Init();
	ProjectDBDataSource->ProjectDatabaseDataSourceRefreshed.BindRaw(this, &FWwiseBrowserDataSource::OnProjectDBDataSourceRefreshed);
	ProjectDBDataSource->ProjectDatabaseUpdatedExternally.BindRaw(this, &FWwiseBrowserDataSource::OnProjectDatabaseUpdatedExternally);

	FAssetRegistryModule* AssetRegistryModule = &FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

	auto& AssetRegistry = AssetRegistryModule->Get();
	if (AssetRegistry.IsLoadingAssets())
	{
		OnFilesLoaded = AssetRegistry.OnFilesLoaded().AddRaw(this, &FWwiseBrowserDataSource::OnFilesFullyLoaded);
	}
	else
	{
		SetupAssetCallbacks();
	}
}

FWwiseBrowserDataSource::~FWwiseBrowserDataSource()
{
	checkf(IsInGameThread(), TEXT("~FWwiseBrowserDataSource must be called on the GameThread"));

	IsAliveRef->store(false, std::memory_order_seq_cst);

	//Bump the generation to forcefully cancel ongoing build operation.
	RebuildCoordinator.CancelCurrentRebuild();

	// Wait on the most recently dispatched worker step before freeing WaapiDataSource/ProjectDBDataSource below.
	// Necessary to avoid situations where a worker thread could still be mid-call into one of them when this destructor frees it resulting in a use-after-free.
	if (CurrentWorkerFuture.IsValid())
	{
		CurrentWorkerFuture.Wait();
	}

#if WWISE_UNIT_TESTS
	if (OnTeardownDrained)
	{
		OnTeardownDrained(RebuildCoordinator.CurrentGeneration(), CurrentWorkerFuture.IsReady());
	}
#endif

	WaapiDataSource->WaapiDataSourceRefreshed.Unbind();
	WaapiDataSource->WwiseSelectionChange.Unbind();
	WaapiDataSource->WwiseExpansionChange.Unbind();

	ProjectDBDataSource->ProjectDatabaseDataSourceRefreshed.Unbind();
	ProjectDBDataSource->ProjectDatabaseUpdatedExternally.Unbind();

	FAssetRegistryModule* AssetRegistryModule = &FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

	auto& AssetRegistry = AssetRegistryModule->Get();

	AssetRegistry.OnAssetAdded().Remove(OnAssetAdded);
	AssetRegistry.OnAssetRemoved().Remove(OnAssetRemoved);
	AssetRegistry.OnAssetRenamed().Remove(OnAssetRenamed);
	AssetRegistry.OnAssetUpdated().Remove(OnAssetUpdated);
	AssetRegistry.OnFilesLoaded().Remove(OnFilesLoaded);

	if (PostEditorTickHandle.IsValid())
	{
		GEngine->OnPostEditorTick().Remove(PostEditorTickHandle);
		PostEditorTickHandle.Reset();
	}
	
	if (WaapiRefreshTickHandle.IsValid())
	{
		GEngine->OnPostEditorTick().Remove(WaapiRefreshTickHandle);
		WaapiRefreshTickHandle.Reset();
	}
}

void FWwiseBrowserDataSource::ConstructTree()
{
	SCOPED_AUDIOKINETICTOOLS_EVENT_2(TEXT("FWwiseBrowserDataSource::ConstructTree"))

	checkf(IsInGameThread(), TEXT("ConstructTree must be called on the GameThread"));

	// Cancel any background chain and wait for a worker step to stop touching shared state before
	// this function starts its own synchronous rebuild below. Avoid situations where a stale worker step
	// could still be writing to RootItemsUnfiltered/the DB tree maps concurrently with this call, racing on shared state.
	RebuildCoordinator.CancelCurrentRebuild();
	if (CurrentWorkerFuture.IsValid())
	{
		CurrentWorkerFuture.Wait();
	}

	auto ConstructTask = [this]
	{
		if(ProjectDBDataSource->ConstructTree(false))
		{
			UAssetDataSource->ConstructItems();
			WaapiDataSource->ConstructTree(false);
			MergeDataSources();
		}
	};

	WwiseUnrealHelper::RunTaskInGameThread(ConstructTask);
}

bool FWwiseBrowserDataSource::AreFiltersOff()
{
	return WwiseTypeFilter.AreFiltersOff() &&
		UAssetStatusFilter.AreFiltersOff() &&
		SoundBankStatusFilter.AreFiltersOff();
}

void FWwiseBrowserDataSource::ApplyTextFilter(TSharedPtr<StringFilter> FilterText)
{
	SCOPED_AUDIOKINETICTOOLS_EVENT_3(TEXT("FWwiseBrowserDataSource::ApplyTextFilter"))

	{
		FScopeLock AutoLock(&FilterTextLock);
		CurrentFilterText = FilterText;
	}
	
	ConstructTree();
	if (!AreFiltersOff())
	{
		ApplyFilter(SoundBankStatusFilter, UAssetStatusFilter, WwiseTypeFilter);
	}

}

void FWwiseBrowserDataSource::ApplyFilter(FSoundBankStatusFilter SoundBankStatus,
										FUAssetStatusFilter UAssetStatus,
										FWwiseTypeFilter WwiseType)
{
	SoundBankStatusFilter = SoundBankStatus;
	UAssetStatusFilter = UAssetStatus;
	WwiseTypeFilter = WwiseType;
	
	TArray<FWwiseTreeItemPtr> UnfilteredSnapshot;
	{
		FScopeLock AutoLock(&RootItemsLock);
		UnfilteredSnapshot = RootItemsUnfiltered;
	}

	RootItems.Empty();
	for (auto& RootItem : UnfilteredSnapshot)
	{
		FWwiseTreeItemPtr RootItemFiltered;
		ApplyFilter(RootItem, RootItemFiltered);
		RootItems.Add(RootItemFiltered);
	}
}

void FWwiseBrowserDataSource::ApplyFilter(FWwiseTreeItemPtr Item, FWwiseTreeItemPtr& OutItem)
{
	OutItem = MakeShared<FWwiseTreeItem>(*Item.Get());
	OutItem->EmptyChildren();
	for (auto CurrItem : Item->GetChildren())
	{
		if (!IsKeptInBrowser(CurrItem))
		{
			continue;
		}
		if (CurrItem->IsFolder())
		{
			FWwiseTreeItemPtr Children;
			ApplyFilter(CurrItem, Children);
			OutItem->AddChild(Children);
		}
		else
		{
			FWwiseTreeItemPtr Children = MakeShared<FWwiseTreeItem>(*CurrItem.Get());
			OutItem->AddChild(Children);
		}
	}
}

void FWwiseBrowserDataSource::ClearFilter()
{
	FScopeLock AutoLock(&FilterTextLock);
	CurrentFilterText = {};
}

bool FWwiseBrowserDataSource::IsKeptInBrowser(FWwiseTreeItemPtr Item)
{
	if(AreFiltersOff())
	{
		return true;
	}

	if (Item->IsFolder())
	{
		for (auto Children : Item->GetChildren())
		{
			if (IsKeptInBrowser(Children))
			{
				return true;
			}
		}
		return false;
	}

	bool bWwiseFilterResult = true;
	if (GetWaapiConnectionStatus() == EWwiseConnectionStatus::Connected)
	{
		bWwiseFilterResult = SoundBankStatusFilter.IsKeptInBrowser(Item);
	}

	return  bWwiseFilterResult &&
		UAssetStatusFilter.IsKeptInBrowser(Item) &&
		WwiseTypeFilter.IsKeptInBrowser(Item);
}

FWwiseTreeItemPtr FWwiseBrowserDataSource::GetTreeRootForType(EWwiseItemType ItemType, const FString& FilterText)
{
	FScopeLock AutoLock(&RootItemsLock);

	if (RootItemsUnfiltered.Num() < static_cast<int32>(ItemType) + 1)
	{
		UE_LOG(LogAudiokineticTools, Error, TEXT("Failed to get Item Type. Index out of range."));
		return MakeShared<FWwiseTreeItem>("", "", nullptr, EWwiseItemType::None, FGuid());
	}
	if (AreFiltersOff())
	{
		return RootItemsUnfiltered[static_cast<int32>(ItemType)];
	}
	return RootItems[static_cast<int32>(ItemType)];
}

FText FWwiseBrowserDataSource::GetProjectName()
{
	if (ProjectDBDataSource.IsValid())
	{
		return ProjectDBDataSource->GetProjectName();
	}
	return {};
}

FText FWwiseBrowserDataSource::GetConnectedWwiseProjectName()
{
	return FText::FromString(WaapiDataSource->LoadProjectName());
}

int32 FWwiseBrowserDataSource::LoadChildren(FWwiseTreeItemPtr TreeItem, TArray<FWwiseTreeItemPtr>& OutChildren)
{
	SCOPED_AUDIOKINETICTOOLS_EVENT_3(TEXT("FWwiseBrowserDataSource::LoadChildren"))

	UE_LOG(LogAudiokineticTools, VeryVerbose, TEXT("Loading children for %s"), *TreeItem->FolderPath )

	TArray<FWwiseTreeItemPtr> WaapiChildren;
	TArray<FWwiseTreeItemPtr> SoundBanksChildren;

	FWwiseTreeItemPtr WaapiParent = WaapiDataSource->FindItemFromPath(TreeItem->FolderPath);
	FWwiseTreeItemPtr SoundBanksParent = ProjectDBDataSource->FindItemFromPath(TreeItem->FolderPath);

	WaapiDataSource->LoadChildren(WaapiParent);
	ProjectDBDataSource->LoadChildren(SoundBanksParent);

	//In this case, it is an orphaned uasset.
	if(!WaapiParent && !SoundBanksParent)
	{
		return TreeItem->GetChildren().Num();
	}
	TreeItem->EmptyChildren();

	if(SoundBanksParent || WaapiParent)
	{
		const FMergeContext MergeContext = MakeMergeContext();
		CreateUnifiedTree(SoundBanksParent, WaapiParent, TreeItem, MergeContext);

		TreeItem->SortChildren();

		return TreeItem->GetChildren().Num();
	}

	return 0;
}

void FWwiseBrowserDataSource::ClearEmptyChildren(FWwiseTreeItemPtr TreeItem)
{
	TArray<FWwiseTreeItemPtr> ChildrenToRemove;

	for (auto Child : *TreeItem->GetChildrenMutable())
	{
		if (Child->IsOfType({EWwiseItemType::None}))
		{
			ChildrenToRemove.Add(Child);
		}
	}

	TreeItem->RemoveChildren(ChildrenToRemove);
}

void FWwiseBrowserDataSource::ClearInvisibleChildren(FWwiseTreeItemPtr TreeItem)
{
	TArray<FWwiseTreeItemPtr> ChildrenToRemove;

	for (auto Child : *TreeItem->GetChildrenMutable())
	{
		if (Child->IsVisible)
		{
			// If we have a work unit with no child, we hide it
			if (Child->IsOfType({ EWwiseItemType::NestedWorkUnit, EWwiseItemType::StandaloneWorkUnit }) && Child->GetChildren().IsEmpty())
			{
				ChildrenToRemove.Add(Child);
			}
		}
		else
		{
			ChildrenToRemove.Add(Child);
		}
	}

	TreeItem->RemoveChildren(ChildrenToRemove);
}

EWwiseConnectionStatus FWwiseBrowserDataSource::GetWaapiConnectionStatus() const
{
	if(!WaapiDataSource.IsValid())
	{
		return EWwiseConnectionStatus::WwiseNotOpen;
	}
	return WaapiDataSource->IsProjectLoaded();
}

FString FWwiseBrowserDataSource::GetItemWorkUnitPath(FWwiseTreeItemPtr InTreeItem)
{
	if (WaapiDataSource.IsValid())
	{
		return WaapiDataSource->GetItemWorkUnitPath(InTreeItem);
	}
	return {};
}

void FWwiseBrowserDataSource::SelectInWwiseProjectExplorer(TArray<FWwiseTreeItemPtr>& InTreeItem)
{
	if (WaapiDataSource.IsValid())
	{
		WaapiDataSource->SelectInProjectExplorer(InTreeItem);
	}
}

void FWwiseBrowserDataSource::HandleFindWwiseItemInProjectExplorerCommandExecute(
	const TArray<FWwiseTreeItemPtr>& SelectedItems) const
{
	WaapiDataSource->HandleFindWwiseItemInProjectExplorerCommandExecute(SelectedItems);
}

bool FWwiseBrowserDataSource::MergeDataSources(bool bCheckCancellation, uint32 Generation)
{
	SCOPED_AUDIOKINETICTOOLS_EVENT_3(TEXT("FWwiseBrowserDataSource::MergeDataSources"))

	FWwiseTreeItemPtr WaapiRootItem;
	FWwiseTreeItemPtr SoundBanksRootItem;

	// Build into a local list so an aborted pass never shows a partial tree.
	TArray<FWwiseTreeItemPtr> NewRootItems;
	NewRootItems.SetNum(static_cast<int32>(EWwiseItemType::LastWwiseBrowserType) + 1);

	FMergeContext MergeContext = bCheckCancellation	? MakeMergeContext(FWwiseRebuildCancellationToken{ &RebuildCoordinator.GenerationRef(), Generation }) : MakeMergeContext();
	{
		FScopeLock AutoLock(&FilterTextLock);
		MergeContext.FilterText = CurrentFilterText;
	}
	const TSharedPtr<StringFilter>& FilterText = MergeContext.FilterText;
	
	for (auto i = EWwiseItemType::First; i <= EWwiseItemType::LastWwiseBrowserType; ++i)
	{
		if (MergeContext.IsCancelled())
		{
			return false;
		}

		if (!FilterText.IsValid() || FilterText->GetRawFilterText().IsEmpty())
		{
			SoundBanksRootItem = ProjectDBDataSource->GetRootItem(i);
		}
		else
		{
			SoundBanksRootItem = ProjectDBDataSource->LoadFilteredRootItem(i, FilterText);
		}

		FWwiseTreeItemPtr NewRootItem;

		if (SoundBanksRootItem)
		{
			NewRootItem = MakeShared<FWwiseTreeItem>(SoundBanksRootItem->DisplayName, SoundBanksRootItem->FolderPath, nullptr, SoundBanksRootItem->ItemType, SoundBanksRootItem->ItemId, SoundBanksRootItem->ShortId);
			NewRootItem->WwiseItemRef = SoundBanksRootItem->WwiseItemRef;
			UE_LOG(LogAudiokineticTools, VeryVerbose, TEXT("Creating Tree for %s "), *SoundBanksRootItem->FolderPath)
		}

		if (!FilterText.IsValid() || FilterText->GetRawFilterText().IsEmpty())
		{
			WaapiRootItem = WaapiDataSource->GetRootItem(i);
		}
		else
		{
			WaapiRootItem = WaapiDataSource->LoadFilteredRootItem(i, FilterText);
		}

		if (WaapiRootItem)
		{
			if (!NewRootItem)
			{
				NewRootItem = MakeShared<FWwiseTreeItem>(WaapiRootItem->DisplayName, WaapiRootItem->FolderPath, nullptr, WaapiRootItem->ItemType, WaapiRootItem->ItemId, WaapiRootItem->ShortId);
				NewRootItem->WwiseItemRef = WaapiRootItem->WwiseItemRef;
			}
			NewRootItem->ItemId = WaapiRootItem->ItemId;
		}

		if (SoundBanksRootItem || WaapiRootItem)
		{
			CreateUnifiedTree(SoundBanksRootItem, WaapiRootItem, NewRootItem, MergeContext);
			UE_LOG(LogAudiokineticTools, VeryVerbose, TEXT("Merging Tree for %s "), *NewRootItem->FolderPath);
		}
		else
		{
			NewRootItem = MakeShared<FWwiseTreeItem>(WwiseItemType::FolderNames[static_cast<int32>(i)],TEXT("\\") + WwiseItemType::FolderNames[static_cast<int32>(i)], nullptr, EWwiseItemType::Folder, FGuid());
		}
		NewRootItems[static_cast<int32>(i)] = NewRootItem;

#if WWISE_UNIT_TESTS
		if (OnMergeTypeProcessed_ForTesting)
		{
			OnMergeTypeProcessed_ForTesting(i);
		}
#endif
	}

	MergeContext.DbLock.Reset();

	// Items only found in Unreal (previously "Orphan UAssets")
	{
		FWwiseTreeItemPtr RootItem = MakeShared<FWwiseTreeItem>(FString("Found only in Unreal"), FString("\\Found only in Unreal"), nullptr, EWwiseItemType::Folder, FGuid());
		TArray<UAssetDataSourceInformation> OrphanAssets;
		UAssetDataSource->GetOrphanAssets(OrphanAssets);
		for(auto& OrphanAsset : OrphanAssets)
		{
			if (FilterText.IsValid() && !FilterText->GetRawFilterText().IsEmpty())
			{
				if (!OrphanAsset.Id.Name.ToString().Contains(FilterText->GetRawFilterText().ToString()))
				{
					continue;
				}
			}
			FWwiseTreeItemPtr NewItem = MakeShared<FWwiseTreeItem>(OrphanAsset.Id.Name.ToString(), FString(), RootItem, OrphanAsset.Type, FGuid());
			NewItem->Assets = OrphanAsset.AssetsData;
			NewItem->UAssetName = OrphanAsset.Id.Name;
			NewItem->ShortId = OrphanAsset.Id.ShortId;
			RootItem->AddChild(NewItem);
		}
		NewRootItems.Add(RootItem);
	}

	if (MergeContext.IsCancelled())
	{
		return false;
	}

	{
		FScopeLock AutoLock(&RootItemsLock);
		RootItemsUnfiltered = MoveTemp(NewRootItems);
	}
	
	if (IsInGameThread())
	{
		WwiseBrowserDataSourceRefreshed.ExecuteIfBound();
	}
	else
	{
		TSharedRef<std::atomic<bool>, ESPMode::ThreadSafe> AliveRef = IsAliveRef;
		AsyncTask(ENamedThreads::GameThread, [this, AliveRef]
		{
			if (!AliveRef->load(std::memory_order_seq_cst))
			{
				return;
			}
			WwiseBrowserDataSourceRefreshed.ExecuteIfBound();
		});
	}

	return true;
}

void FWwiseBrowserDataSource::RequestRebuild(bool bRebuildProjectDB, bool bRebuildWaapi)
{
	checkf(IsInGameThread(), TEXT("RequestRebuild must be called on the GameThread"));

	if (!RebuildCoordinator.RequestRebuild(bRebuildProjectDB, bRebuildWaapi))
	{
		return;
	}
	LaunchRebuildChain();
}

void FWwiseBrowserDataSource::LaunchRebuildChain()
{
	checkf(IsInGameThread(), TEXT("LaunchRebuildChain must be called on the GameThread"));

	const FWwiseBrowserRebuildCoordinator::FBrowserRebuildOperation Launch = RebuildCoordinator.BeginChain();
	const uint32 MyGeneration = Launch.Generation;
	const bool bDoProjectDB = Launch.bProjectDB;
	const bool bDoWaapi = Launch.bWaapi;
	TSharedRef<std::atomic<bool>, ESPMode::ThreadSafe> AliveRef = IsAliveRef;

	CurrentWorkerFuture = Async(EAsyncExecution::TaskGraph, [this, MyGeneration, bDoProjectDB, bDoWaapi, AliveRef]
	{
		if (!AliveRef->load(std::memory_order_seq_cst))
		{
			return;
		}

		bool bProjectDbOk = true;
		if (bDoProjectDB)
		{
			ProjectDBDataSource->SetRebuildCancellation(FWwiseRebuildCancellationToken{ &RebuildCoordinator.GenerationRef(), MyGeneration });
			bProjectDbOk = ProjectDBDataSource->ConstructTree(false);
			ProjectDBDataSource->SetRebuildCancellation({});
		}
		if (bDoWaapi && !IsRebuildStale(MyGeneration))
		{
			WaapiDataSource->ConstructTree(false);
		}

		AsyncTask(ENamedThreads::GameThread, [this, MyGeneration, bProjectDbOk, AliveRef]
		{
			if (!AliveRef->load(std::memory_order_seq_cst))
			{
				return;
			}

			if (!bProjectDbOk || IsRebuildStale(MyGeneration))
			{
				FinishOrRelaunch(MyGeneration);
				return;
			}

			UAssetDataSource->ConstructItems();

			CurrentWorkerFuture = Async(EAsyncExecution::TaskGraph, [this, MyGeneration, AliveRef]
			{
				if (!AliveRef->load(std::memory_order_seq_cst))
				{
					return;
				}

				MergeDataSources(true, MyGeneration);

				AsyncTask(ENamedThreads::GameThread, [this, MyGeneration, AliveRef]
				{
					if (!AliveRef->load(std::memory_order_seq_cst))
					{
						return;
					}
					FinishOrRelaunch(MyGeneration);
				});
			});
		});
	});
}

void FWwiseBrowserDataSource::FinishOrRelaunch(uint32 FinishedGeneration)
{
	checkf(IsInGameThread(), TEXT("FinishOrRelaunch must be called on the GameThread"));

	if (RebuildCoordinator.RequireRelaunch(FinishedGeneration))
	{
		LaunchRebuildChain();
	}
}

void FWwiseBrowserDataSource::CreateUnifiedTree(const FWwiseTreeItemPtr& TreeItemRootSoundBank, const FWwiseTreeItemPtr& TreeItemRootWaapi, FWwiseTreeItemPtr& TreeItemRootDst, const FMergeContext& Ctx)
{
	if (TreeItemRootSoundBank)
	{
		CreateProjectDBItem(TreeItemRootSoundBank, TreeItemRootDst, Ctx);
		if (TreeItemRootWaapi)
		{
			CreateWaapiItem(TreeItemRootWaapi, TreeItemRootDst, Ctx);
		}
	}
}

void FWwiseBrowserDataSource::CreateProjectDBItem(const FWwiseTreeItemPtr& TreeItemRootSoundBank, FWwiseTreeItemPtr& TreeItemRootDst, const FMergeContext& Ctx)
{
	if (Ctx.IsCancelled())
	{
		return;
	}
	FString DefaultAssetName = TreeItemRootSoundBank->GetDefaultAssetName();
	TreeItemRootDst = MakeShared<FWwiseTreeItem>(TreeItemRootSoundBank->DisplayName, TreeItemRootSoundBank->FolderPath, nullptr, TreeItemRootSoundBank->ItemType, TreeItemRootSoundBank->ItemId, TreeItemRootSoundBank->ShortId);
	if(TreeItemRootSoundBank->ShouldDisplayInfo())
	{
		TArray<FAssetData> Assets;
		EWwiseItemType Type;
		FName UAssetName;
		UAssetDataSource->GetAssetsInfo(TreeItemRootSoundBank->ItemId, TreeItemRootSoundBank->ShortId, DefaultAssetName, TreeItemRootSoundBank->GroupId, Type, UAssetName, Assets);
		TreeItemRootDst->Assets = Assets;
		TreeItemRootDst->UAssetName = UAssetName;
		TreeItemRootDst->bIsInWrongLocation = IsMoved(TreeItemRootDst, Ctx);
	}
	TreeItemRootDst->WwiseItemRef = TreeItemRootSoundBank->WwiseItemRef;
	//State Groups have a None State that does not appear in Wwise. Set the Ref to true to avoid "Deleted in Wwise".
	if(TreeItemRootDst->ItemType == EWwiseItemType::State && TreeItemRootDst->DisplayName == FString("None"))
	{
		TreeItemRootDst->WaapiName = TreeItemRootDst->DisplayName;
		TreeItemRootDst->bWaapiRefExists = true;
	}
	for (auto& Child : TreeItemRootSoundBank->GetChildren())
	{
		FWwiseTreeItemPtr NewItem;
		CreateProjectDBItem(Child, NewItem, Ctx);
		TreeItemRootDst->AddChild(NewItem);
	}
	TreeItemRootDst->SortChildren();
}

bool FWwiseBrowserDataSource::IsMoved(FWwiseTreeItemPtr CurrItem, const FMergeContext& Ctx)
{
	if(!CurrItem->HasUniqueUAsset())
	{
		return false;
	}
	if(!Ctx.IsValid())
	{
		return false;
	}

	FString Out;
	WwiseAnyRef* Ref = nullptr;
	WwiseDBArray<WwiseAnyRef> Refs;
	const WwiseDBGuid Guid(CurrItem->ItemId.A, CurrItem->ItemId.B, CurrItem->ItemId.C, CurrItem->ItemId.D);

	Ctx.DbLock->GetCurrentPlatformData()->Guids.MultiFind(WwiseDatabaseLocalizableGuidKey(Guid, 0), Refs);
	for(auto& InRef : Refs)
	{
		if(InRef.GetType() != WwiseRefType::SoundBank)
		{
			Ref = &InRef; break;
		}
	}
	return Ref && Ctx.Reconcile->ShouldMove(*Ref, CurrItem->Assets[0], Out);
}

FWwiseTreeItemPtr FWwiseBrowserDataSource::CreateWaapiExclusiveItem(const FWwiseTreeItemPtr& WaapiItem, const FWwiseTreeItemPtr& TreeItemRootDst, const FMergeContext& Ctx)
{
	if (Ctx.IsCancelled())
	{
		return nullptr;
	}
	FWwiseTreeItemPtr CurrItem = MakeShared<FWwiseTreeItem>(WaapiItem->DisplayName, WaapiItem->FolderPath, nullptr, WaapiItem->ItemType, WaapiItem->ItemId, WaapiItem->ShortId);
	if(WaapiItem->ShouldDisplayInfo())
	{
		FString DefaultAssetName = WaapiItem->GetDefaultAssetName();
		TArray<FAssetData> Assets;
		FName AssetName;
		EWwiseItemType ItemType;

		uint32 shortId = WaapiItem->ShortId;
		if (shortId == AK_INVALID_UNIQUE_ID)
		{
			// We still generate the short id if not returned by waapi
			shortId = FAkAudioDevice::GetShortIDFromString(WaapiItem->WaapiName);
		}
		CurrItem->ShortId = shortId;

		uint32 groupId = WaapiItem->GroupId;
		if (WaapiItem->IsInGroup() && groupId == AK_INVALID_UNIQUE_ID)
		{
			TArray<FString> SplitPath;

#ifdef WIN32
			WaapiItem->FolderPath.ParseIntoArray(SplitPath, TEXT("\\"), true);
#else
			WaapiItem->FolderPath.ParseIntoArray(SplitPath, TEXT("/"), true);
#endif	
			
			if (SplitPath.Num() > 1)
			{
				// We still generate the group id if not returned by waapi
				groupId = FAkAudioDevice::GetShortIDFromString(SplitPath[SplitPath.Num() - 2]);
			}
		}
		CurrItem->GroupId = groupId;

		UAssetDataSource->GetAssetsInfo(WaapiItem->ItemId, shortId, DefaultAssetName, groupId, ItemType, AssetName, Assets);
		CurrItem->UAssetName = AssetName;
		CurrItem->Assets = Assets;
		CurrItem->bIsInWrongLocation = IsMoved(CurrItem, Ctx);
	}
	CurrItem->WaapiName = WaapiItem->DisplayName;
	CurrItem->bWaapiRefExists = true;
	CurrItem->ChildCountInWwise = WaapiItem->ChildCountInWwise;
	TreeItemRootDst->AddChild(CurrItem);
	return CurrItem;
}

void FWwiseBrowserDataSource::CreateWaapiItem(const FWwiseTreeItemPtr& TreeItemRootWaapi, const FWwiseTreeItemPtr& TreeItemRootDst, const FMergeContext& Ctx)
{
	if (Ctx.IsCancelled())
	{
		return;
	}
	TArray<TPair<FWwiseTreeItemPtr, FWwiseTreeItemPtr>> ToRecurse;
	ToRecurse.Reserve(TreeItemRootWaapi->GetChildren().Num());

	for (const auto& WaapiItem : TreeItemRootWaapi->GetChildren())
	{
		FWwiseTreeItemPtr Dst = TreeItemRootDst->GetChild(WaapiItem->ItemId, WaapiItem->ShortId, WaapiItem->DisplayName);
		if (Dst)
		{
			Dst->WaapiName = WaapiItem->DisplayName;
			Dst->bWaapiRefExists = true;
			if (Dst->IsFolder()) { Dst->ItemId = WaapiItem->ItemId; }
		}
		else if (WaapiItem->IsFolder())
		{
			Dst = MakeShared<FWwiseTreeItem>(WaapiItem->DisplayName, WaapiItem->FolderPath, nullptr, WaapiItem->ItemType, WaapiItem->ItemId, WaapiItem->ShortId);
			TreeItemRootDst->AddChild(Dst);
		}
		else if (auto SoundBankItem = ProjectDBDataSource->FindItem(WaapiItem))
		{
			Dst = TreeItemRootDst->GetRoot()->FindItemRecursive(WaapiItem);
			if (Dst) { Dst->WaapiName = WaapiItem->DisplayName; Dst->bWaapiRefExists = true; Dst->bSameLocation = false; }
		}
		else
		{
			Dst = CreateWaapiExclusiveItem(WaapiItem, TreeItemRootDst, Ctx);
		}

		if (Dst) { ToRecurse.Emplace(WaapiItem, Dst); }
	}

	TreeItemRootDst->SortChildren();

	for (const auto& Pair : ToRecurse)
	{
		CreateWaapiItem(Pair.Key, Pair.Value, Ctx);
	}
}

void FWwiseBrowserDataSource::OnWaapiDataSourceRefreshed()
{
	SCOPED_AUDIOKINETICTOOLS_EVENT_3(TEXT("FWwiseBrowserDataSource::OnWaapiDataSourceRefreshed"))

	TSharedRef<std::atomic<bool>, ESPMode::ThreadSafe> AliveRef = IsAliveRef;
	auto ArmRefresh = [this, AliveRef]
	{
		if (!AliveRef->load(std::memory_order_seq_cst))
		{
			return;
		}
		if (GEngine && !WaapiRefreshTickHandle.IsValid())
		{
			WaapiRefreshTickHandle = GEngine->OnPostEditorTick().AddRaw(this, &FWwiseBrowserDataSource::OnWaapiRefreshTimerTick);
		}
		WaapiRefreshTimer = WaapiRefreshDelay;
	};

	WwiseUnrealHelper::RunTaskInGameThread(ArmRefresh);
}

void FWwiseBrowserDataSource::OnWaapiRefreshTimerTick(float DeltaSeconds)
{
	WaapiRefreshTimer -= DeltaSeconds;
	if (WaapiRefreshTimer > 0.f)
	{
		// Still receiving notifications (timer keeps getting re-armed) — defer the rebuild.
		return;
	}

	if (GEngine && WaapiRefreshTickHandle.IsValid())
	{
		GEngine->OnPostEditorTick().Remove(WaapiRefreshTickHandle);
		WaapiRefreshTickHandle.Reset();
	}

	RequestRebuild(true, false);
}

void FWwiseBrowserDataSource::OnProjectDBDataSourceRefreshed()
{
	SCOPED_AUDIOKINETICTOOLS_EVENT_3(TEXT("FWwiseBrowserDataSource::OnProjectDBDataSourceRefreshed"))

	TSharedRef<std::atomic<bool>, ESPMode::ThreadSafe> AliveRef = IsAliveRef;
	WwiseUnrealHelper::RunTaskInGameThread([this, AliveRef]
	{
		if (!AliveRef->load(std::memory_order_seq_cst))
		{
			return;
		}
		RequestRebuild(false, true);
	});
}

void FWwiseBrowserDataSource::OnProjectDatabaseUpdatedExternally()
{
	SCOPED_AUDIOKINETICTOOLS_EVENT_3(TEXT("FWwiseBrowserDataSource::OnProjectDatabaseUpdatedExternally"))

	checkf(IsInGameThread(), TEXT("OnProjectDatabaseUpdatedExternally must be called on the GameThread"));

	RequestRebuild(true, true);
}

void FWwiseBrowserDataSource::SetupAssetCallbacks()
{
	FAssetRegistryModule* AssetRegistryModule = &FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

	if (AssetRegistryModule)
	{
		auto& AssetRegistry = AssetRegistryModule->Get();
		OnAssetAdded = AssetRegistry.OnAssetAdded().AddRaw(this, &FWwiseBrowserDataSource::OnUAssetSourceRefresh);
		OnAssetRemoved = AssetRegistry.OnAssetRemoved().AddRaw(this, &FWwiseBrowserDataSource::OnUAssetSourceRefresh);
		OnAssetRenamed = AssetRegistry.OnAssetRenamed().AddRaw(this, &FWwiseBrowserDataSource::OnUAssetSourceRefresh);
		OnAssetUpdated = AssetRegistry.OnAssetUpdated().AddRaw(this, &FWwiseBrowserDataSource::OnUAssetSourceRefresh);
	}
}

void FWwiseBrowserDataSource::OnFilesFullyLoaded()
{
	UE_LOG(LogAudiokineticTools, Log, TEXT("Asset registry files loaded"))
	// Should only be called from the AssetRegistry on the GameThread
	checkf(IsInGameThread(), TEXT("FWwiseBrowserDataSource::OnFilesFullyLoaded should only be called from the AssetRegistry on the GameThread"))

	if (OnFilesLoaded.IsValid())
	{
		FAssetRegistryModule* AssetRegistryModule = &FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		if (AssetRegistryModule)
		{
			auto& AssetRegistry = AssetRegistryModule->Get();
			AssetRegistry.OnFilesLoaded().Remove(OnFilesLoaded);
		}
	}
	SetupAssetCallbacks();
	RequestRebuild(false,false);
}

void FWwiseBrowserDataSource::OnTimerTick(float DeltaSeconds)
{
	AssetUpdateTimer -= DeltaSeconds;
	if (AssetUpdateTimer <= 0.f)
	{
		RequestRebuild(false, false);
		if (PostEditorTickHandle.IsValid())
		{
			GEngine->OnPostEditorTick().Remove(PostEditorTickHandle);
			PostEditorTickHandle.Reset();
		}
	}
}

void FWwiseBrowserDataSource::OnUAssetSourceRefresh(const FAssetData& AssetData)
{
	if (AkUnrealAssetDataHelper::IsAssetAkAudioType(AssetData) && !AkUnrealAssetDataHelper::IsAssetTransient(AssetData))
	{
		checkf(IsInGameThread(), TEXT("FWwiseBrowserDataSource::OnUAsserSourceRefresh should only be called from the AssetRegistry on the GameThread"))

		if (!PostEditorTickHandle.IsValid())
		{
			PostEditorTickHandle = GEngine->OnPostEditorTick().AddRaw(this, &FWwiseBrowserDataSource::OnTimerTick);
		}

		AssetUpdateTimer = AssetTimerRefresh;
	}
}

// Required for handling Asset renaming
void FWwiseBrowserDataSource::OnUAssetSourceRefresh(const FAssetData& AssetData, const FString& /*OldPath*/)
{
	OnUAssetSourceRefresh(AssetData);
}

void FWwiseBrowserDataSource::OnWwiseSelectionChange(const TArray<TSharedPtr<FWwiseTreeItem>>& Items)
{
	WwiseSelectionChange.ExecuteIfBound(Items);
}

void FWwiseBrowserDataSource::OnWwiseExpansionChange(const TArray<TSharedPtr<FWwiseTreeItem>>& Items)
{
	WwiseExpansionChange.ExecuteIfBound(Items);
}
