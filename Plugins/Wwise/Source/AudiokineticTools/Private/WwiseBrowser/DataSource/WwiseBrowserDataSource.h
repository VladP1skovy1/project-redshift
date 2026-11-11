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

#include "CoreMinimal.h"
#include "UAssetDataSource.h"

#include "Async/Future.h"
#include "WaapiDataSource.h"
#include "../WwiseBrowserForwards.h"
#include "WwiseBrowserRebuildCoordinator.h"
#include "WwiseItemType.h"
#include "WwiseProjectDatabaseSource.h"
#include "WwiseBrowser/WwiseAssetDragDropOp.h"
#include "Wwise/WwiseUnitTests.h"

struct FWwiseTreeItem;
struct FMergeContext;

enum ESoundBankStatusFilter
{
	NewInWwise,
	DeletedInWwise,
	RenamedInWwise,
	NotInWwise,
	MovedInWwise,
	UpToDate,
	NumberOfSoundBankStatus
};

enum EUAssetStatusFilter
{
	UAssetMissing,
	NotInSoundBankOrUnreal,
	UAssetOrphaned,
	RenamedInSoundBank,
	MultipleUAssets,
	UAssetNeedsUpdate,
	UAssetUpToDate,
	NumberOfUAssetStatus
};

enum EWwiseTypeFilter
{
	AcousticTexture,
	AudioDeviceShareSet,
	DialogueEvents,
	Effects,
	Events,
	GameParameters,
	MasterMixerHierarchy,
	State,
	Switch,
	Trigger,
	NumberOfWwiseTypes
};

struct FSoundBankStatusFilter
{
	bool bFilters[NumberOfSoundBankStatus] = { false };
	bool IsKeptInBrowser(FWwiseTreeItemPtr& Item) const;
	bool AreFiltersOff() const;
};

struct FUAssetStatusFilter
{
	bool bFilters[NumberOfUAssetStatus] = { false };
	bool IsKeptInBrowser(FWwiseTreeItemPtr& Item) const;
	bool AreFiltersOff() const;
};

struct FWwiseTypeFilter
{
	bool bFilters[NumberOfWwiseTypes] = { false };
	bool IsKeptInBrowser(FWwiseTreeItemPtr& Item) const;
	bool AreFiltersOff() const;
private:
	EWwiseItemType GetExpectedType(EWwiseTypeFilter Filter) const;
};

class FWwiseBrowserDataSource
{
public:
	DECLARE_DELEGATE(FOnWwiseBrowserDataSourceRefreshed)

	FWwiseBrowserDataSource();

	~FWwiseBrowserDataSource();

	void ConstructTree();
	bool AreFiltersOff();

	void ApplyTextFilter(TSharedPtr<StringFilter> FilterText);

	void ApplyFilter(FSoundBankStatusFilter SoundBankStatusFilter,
					FUAssetStatusFilter UAssetStatusFilter,
					FWwiseTypeFilter WwiseTypeFilter);

	void ApplyFilter(FWwiseTreeItemPtr Item, FWwiseTreeItemPtr& OutItem);

	void ClearFilter();

	bool IsKeptInBrowser(FWwiseTreeItemPtr Item);

	FWwiseTreeItemPtr GetTreeRootForType(EWwiseItemType ItemType, const FString& FilterText={});

	FText GetProjectName();

	FText GetConnectedWwiseProjectName();

	int32 LoadChildren(FWwiseTreeItemPtr TreeItem, TArray<FWwiseTreeItemPtr>& OutChildren);

	// Clean placeholder children
	void ClearEmptyChildren(FWwiseTreeItemPtr TreeItem);

	// Remove all childrens that should not be visible
	void ClearInvisibleChildren(FWwiseTreeItemPtr TreeItem);

	EWwiseConnectionStatus GetWaapiConnectionStatus() const;

	FString GetItemWorkUnitPath(FWwiseTreeItemPtr InTreeItem);

	void SelectInWwiseProjectExplorer(TArray<FWwiseTreeItemPtr>& InTreeItem);

	FOnWwiseBrowserDataSourceRefreshed WwiseBrowserDataSourceRefreshed;

	FOnWaapiSelectionChange WwiseSelectionChange;
	FOnWaapiSelectionChange WwiseExpansionChange;

	FDelegateHandle PostEditorTickHandle;

	void HandleFindWwiseItemInProjectExplorerCommandExecute(const TArray<FWwiseTreeItemPtr>& SelectedItems) const;

	void CreateProjectDBItem(const FWwiseTreeItemPtr& TreeItemRootSoundBank, FWwiseTreeItemPtr& TreeItemRootDst, const FMergeContext& Ctx);
	FWwiseTreeItemPtr CreateWaapiExclusiveItem(const FWwiseTreeItemPtr& WaapiItem, const FWwiseTreeItemPtr& TreeItemRootDst, const FMergeContext& Ctx);
	void CreateWaapiItem(const FWwiseTreeItemPtr& TreeItemRootWwise, const FWwiseTreeItemPtr& TreeItemRootDst, const FMergeContext& Ctx);
	bool IsMoved(FWwiseTreeItemPtr CurrItem, const FMergeContext& Ctx);

protected:
	bool MergeDataSources(bool bCheckCancellation = false, uint32 Generation = 0);
	void RequestRebuild(bool bRebuildProjectDB, bool bRebuildWaapi);
	
	TUniquePtr<FWaapiDataSource> WaapiDataSource;
	TUniquePtr<FWwiseProjectDatabaseDataSource> ProjectDBDataSource;
	TUniquePtr<FUAssetDataSource> UAssetDataSource;
	TArray< FWwiseTreeItemPtr > RootItemsUnfiltered;

#if WWISE_UNIT_TESTS
public:
	// Allocates the sub-datasources without running their Init(), so a test
	// subclass built with this constructor never touches WAAPI/AssetRegistry/GEditor.
	enum class EForTesting { SkipInit };
	explicit FWwiseBrowserDataSource(EForTesting);
	
	// Invoked once per EWwiseItemType processed inside MergeDataSources, right after that type's
	// CreateUnifiedTree call. Lets tests inject a mid-loop cancellation deterministically.
	TFunction<void(EWwiseItemType)> OnMergeTypeProcessed_ForTesting;
	void CancelCurrentRebuild();

	// Lets a test capture RebuildCoordinator's generation counter
	uint32 GetCurrentGeneration() const;

	// Fired once, at the end of the destructor's drain loop - after CancelCurrentRebuild() has
	// bumped the generation and OutstandingWorkerTasks has just reached 0. Mainly for testing purposes.
	TFunction<void(uint32 Generation, bool CurrentWorkerFutureIsReady)> OnTeardownDrained;

#endif

private:

	bool bIsDirty = false;

	float AssetUpdateTimer = 0.f;

	const float AssetTimerRefresh = 0.1f;
	
	float WaapiRefreshTimer = 0.f;

	const float WaapiRefreshDelay = 0.5f;

	FDelegateHandle WaapiRefreshTickHandle;

	FWwiseBrowserRebuildCoordinator RebuildCoordinator;
	TFuture<void> CurrentWorkerFuture;
	TSharedRef<std::atomic<bool>, ESPMode::ThreadSafe> IsAliveRef { new std::atomic{ true } };
	void LaunchRebuildChain();
	void FinishOrRelaunch(uint32 FinishedGeneration);
	bool IsRebuildStale(uint32 Generation) const
	{
		return RebuildCoordinator.IsStale(Generation);
	}

	/** Root items, one for each type of Wwise object */
	FCriticalSection RootItemsLock;
	TArray< FWwiseTreeItemPtr > RootItems;

	FCriticalSection FilterTextLock;
	TSharedPtr<StringFilter> CurrentFilterText;

	FSoundBankStatusFilter SoundBankStatusFilter;

	FUAssetStatusFilter UAssetStatusFilter;

	FWwiseTypeFilter WwiseTypeFilter;

	// Merges TreeItemRootSrc and TreeItemRootDst, with the resulting tree in TreeItemRootDst. New TreeItems will be created if they do not exist in TreeItemRootDst
	void CreateUnifiedTree(const FWwiseTreeItemPtr& TreeItemRootSoundBank, const FWwiseTreeItemPtr& TreeItemRootWaapi, FWwiseTreeItemPtr& TreeItemRootDst, const FMergeContext& Ctx);

	void OnWaapiDataSourceRefreshed();
	
	void OnWaapiRefreshTimerTick(float DeltaSeconds);

	void OnProjectDBDataSourceRefreshed();
	void OnProjectDatabaseUpdatedExternally();

	void SetupAssetCallbacks();
	void OnFilesFullyLoaded();
	// Timer to debounce updates from the Asset Registry
	void OnTimerTick(float DeltaSeconds);

	void OnUAssetSourceRefresh(const FAssetData& RemovedAssetData);

	void OnUAssetSourceRefresh(const FAssetData& RemovedAssetData, const FString& OldPath);

	void OnWwiseSelectionChange(const TArray<TSharedPtr<FWwiseTreeItem>>& Items);
	void OnWwiseExpansionChange(const TArray<TSharedPtr<FWwiseTreeItem>>& Items);

	FDelegateHandle OnAssetAdded;
	FDelegateHandle OnAssetRemoved;
	FDelegateHandle OnAssetRenamed;
	FDelegateHandle OnAssetUpdated;
	FDelegateHandle OnFilesLoaded;
};

