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

#include "IWwiseBrowserDataSource.h"
#include "Misc/TextFilter.h"
#include "Wwise/Ref/WwiseRefCollections.h"
#include "WwiseRebuildCancellationToken.h"

struct WwiseMetadataBasicReference;

struct WwiseItemTreePath
{
	FString HierarchyName;
	FString RootFolder;
	TArray<FString> IntermediateFolders;
	FString ItemName;
};

struct FWwiseTreeBuildContext
{
	TArray<FWwiseTreeItemPtr>& RootItems;
	TMap<FGuid, FWwiseTreeItemPtr>& AllValidTreeItemsByGuid;
	TMultiMap<uint32, FWwiseTreeItemPtr>& AllValidTreeItemsByShortId;
	TMultiMap<FString, FWwiseTreeItemPtr>& AllValidTreeItemsByName;
	TMap<FString, FWwiseTreeItemPtr>& NodesByPath;
};

class FWwiseProjectDatabaseDataSource : IWwiseBrowserDataSource
{
public:
	DECLARE_DELEGATE(FOnWwiseProjectDatasbaseDataSourceRefreshed)
	DECLARE_DELEGATE(FOnWwiseProjectDatabaseUpdatedExternally)

	FWwiseProjectDatabaseDataSource();
	virtual ~FWwiseProjectDatabaseDataSource() override;

	FText GetProjectName();

	// IWwiseBrowserDataSource

	virtual bool Init() override;
	virtual bool ConstructTree(bool bShouldRefresh) override;
	void SetRebuildCancellation(FWwiseRebuildCancellationToken InCancelToken)
	{
		CancelToken = InCancelToken;
	}
	virtual FWwiseTreeItemPtr ConstructTreeRoot(EWwiseItemType Type) override;
	virtual int32 LoadChildren(const FGuid& InParentId, const FString& InParentPath, TArray<FWwiseTreeItemPtr>& OutChildren) override;
	virtual int32 LoadChildren(FWwiseTreeItemPtr InParentItem) override;
	virtual int32 GetChildItemCount(const FWwiseTreeItemPtr& InParentItem) override;
	virtual FWwiseTreeItemPtr GetRootItem(EWwiseItemType RootType) override;
	virtual FWwiseTreeItemPtr LoadFilteredRootItem(EWwiseItemType ItemType, TSharedPtr<StringFilter> CurrentFilter) override;

	FWwiseTreeItemPtr FindItemFromPath(const FString& InCurrentItemPath);
	FWwiseTreeItemPtr FindItem(const FWwiseTreeItemPtr InItem);

	FOnWwiseProjectDatasbaseDataSourceRefreshed ProjectDatabaseDataSourceRefreshed;
	FOnWwiseProjectDatabaseUpdatedExternally ProjectDatabaseUpdatedExternally;

private:

	bool ParseTreePath(const FString& ObjectPath, WwiseItemTreePath& OutItemPath);

	void CopyTree(FWwiseTreeItemPtr SourceTreeItem, FWwiseTreeItemPtr DestTreeItem);

	void FilterTree(FWwiseTreeItemPtr TreeItem, TSharedPtr<StringFilter> SearchFilter);

	bool IsContainer(EWwiseItemType ItemType) const;
	
	bool IsRebuildCancelled() const
	{
		return CancelToken.IsCancelled();
	}
	static constexpr int32 RebuildCancellationCheckInterval = 100;
	FWwiseRebuildCancellationToken CancelToken;

	// Allows some optimization if we have already applied a search
	FString OldFilterText;

	FDelegateHandle OnDatabaseUpdateCompleteHandle;

protected:
	FCriticalSection TreeDataLock;

	/** Root items, one for each type of Wwise object */
	TArray< FWwiseTreeItemPtr > RootItems;

	// Container paths along the Browser Tree
	TMap<FString, FWwiseTreeItemPtr> NodesByPath;

	// Map of all tree items
	TMap<FGuid, FWwiseTreeItemPtr> AllValidTreeItemsByGuid;
	TMultiMap<uint32,  FWwiseTreeItemPtr> AllValidTreeItemsByShortId;
	TMultiMap<FString, FWwiseTreeItemPtr> AllValidTreeItemsByName;

	void BuildEvents(const WwiseEventGlobalIdsMap& Events, FWwiseTreeBuildContext& Ctx);
	void BuildDialogueEvents(const WwiseDialogueEventGlobalIdsMap& DialogueEvents, FWwiseTreeBuildContext& Ctx);
	void BuildBusses(const WwiseBusGlobalIdsMap& Busses, FWwiseTreeBuildContext& Ctx);
	void BuildAuxBusses(const WwiseAuxBusGlobalIdsMap& AuxBusses, FWwiseTreeBuildContext& Ctx);
	void BuildAcousticTextures(const WwiseAcousticTextureGlobalIdsMap& AcousticTextures, FWwiseTreeBuildContext& Ctx);
	void BuildAudioDevices(const WwiseAudioDeviceGlobalIdsMap& AudioDevices, FWwiseTreeBuildContext& Ctx);
	void BuildStateGroups(const WwiseStateGroupGlobalIdsMap& StateGroups, FWwiseTreeBuildContext& Ctx);
	void BuildStates(const WwiseStateGlobalIdsMap& States, FWwiseTreeBuildContext& Ctx);
	void BuildSwitchGroups(const WwiseSwitchGroupGlobalIdsMap& SwitchGroups, FWwiseTreeBuildContext& Ctx);
	void BuildSwitches(const WwiseSwitchGlobalIdsMap& Switches, FWwiseTreeBuildContext& Ctx);
	void BuildGameParameters(const WwiseGameParameterGlobalIdsMap& GameParameters, FWwiseTreeBuildContext& Ctx);
	void BuildTriggers(const WwiseTriggerGlobalIdsMap& Triggers, FWwiseTreeBuildContext& Ctx);
	void BuildEffectShareSets(const WwisePluginShareSetGlobalIdsMap& EffectShareSets, FWwiseTreeBuildContext& Ctx);

	bool BuildFolderHierarchy(const WwiseMetadataBasicReference& WwiseItem, EWwiseItemType ItemType, const FWwiseTreeItemPtr CurrentRootFolder, FWwiseTreeBuildContext& Ctx, uint32 GroupId = 0);
};
