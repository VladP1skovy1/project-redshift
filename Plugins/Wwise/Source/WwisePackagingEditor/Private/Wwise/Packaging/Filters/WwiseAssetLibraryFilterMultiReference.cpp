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

#include "Wwise/Packaging/Filters/WwiseAssetLibraryFilterMultiReference.h"

#include "Wwise/Packaging/WwiseAssetLibraryFilteringSharedData.h"

bool UWwiseAssetLibraryFilterMultiReference::IsAssetAvailable(const FWwiseAssetLibraryFilteringSharedData& Shared,
		const WwiseAnyRef& Asset) const
{
	const WwiseRefType AssetType = Asset.GetType();

	if (Shared.Db.GetUsageCount(Asset) == 1)
	{
		return false;
	}

	if (AssetType == WwiseRefType::Media)
	{
		if (const auto* Media = Asset.GetMedia())
		{
			return MultiReferencedMediaIds.Contains(Media->Id);
		}
	}
	else if (AssetType == WwiseRefType::SoundBank)
	{
		if (const auto* Soundbank = Asset.GetSoundBank())
		{
			return MultiReferencedSoundBankIds.Contains(Soundbank->Id);
		}
	}

	return false;
}

void UWwiseAssetLibraryFilterMultiReference::PreFilter(const FWwiseAssetLibraryFilteringSharedData& Shared,
	const FWwiseAssetLibraryInfo& AssetLibraryInfo)
{
	MultiReferencedMediaIds.Empty();
	MultiReferencedSoundBankIds.Empty();

	TMap<int32, int32> MediaReferenceCount;
	TMap<int32, int32> SoundBankReferenceCount;

	int32 ProcessedCount = 0;
	for (const auto& UnrealAsset : Shared.AssetsData)
	{
		if (++ProcessedCount % MultiRefFilterCancellationCheckInterval == 0 && Shared.IsCancelled())
		{
			MultiReferencedMediaIds.Empty();
			MultiReferencedSoundBankIds.Empty();
			return;
		}

		auto GuidValue = UnrealAsset.TagsAndValues.FindTag(GET_MEMBER_NAME_CHECKED(FWwiseObjectInfo, WwiseGuid));
		auto ShortIdValue = UnrealAsset.TagsAndValues.FindTag(GET_MEMBER_NAME_CHECKED(FWwiseObjectInfo, WwiseShortId));
		auto NameValue = UnrealAsset.TagsAndValues.FindTag(GET_MEMBER_NAME_CHECKED(FWwiseGroupValueInfo, WwiseName));

		if (UnrealAsset.AssetClassPath.GetAssetName().ToString().Contains("AkAudioEvent"))
		{
			FWwiseEventInfo EventInfo;
			EventInfo.WwiseGuid = FGuid(GuidValue.AsString());
			EventInfo.WwiseShortId = FCString::Strtoui64(*ShortIdValue.GetValue(), NULL, 10);
			EventInfo.WwiseName = NameValue.AsName();
			auto Events = Shared.Db.GetEvent(EventInfo);

			for (auto& Event : Events)
			{
				auto Medias = Event.GetAllMedia(Shared.Db.GetMediaFiles());
				for (auto& Media : Medias)
				{
					int32 MediaId = Media.Key;
					MediaReferenceCount.FindOrAdd(MediaId, 0)++;
				}

				if (auto SoundBank = Event.GetSoundBank())
				{
					int32 SoundBankId = SoundBank->Id;
					SoundBankReferenceCount.FindOrAdd(SoundBankId, 0)++;
				}
			}
		}

		if (UnrealAsset.AssetClassPath.GetAssetName().ToString().Contains("AkAuxBus"))
		{
			FWwiseObjectInfo ObjectInfo;
			ObjectInfo.WwiseGuid = FGuid(GuidValue.AsString());
			ObjectInfo.WwiseShortId = FCString::Strtoui64(*ShortIdValue.GetValue(), NULL, 10);
			ObjectInfo.WwiseName = NameValue.AsName();
			auto Bus = Shared.Db.GetAuxBus(ObjectInfo);

			auto Medias = Bus.GetSoundBankMedia(Shared.Db.GetMediaFiles());
			for (auto& Media : Medias)
			{
				int32 MediaId = Media.Key;
				MediaReferenceCount.FindOrAdd(MediaId, 0)++;
			}

			if (auto SoundBank = Bus.GetSoundBank())
			{
				int32 SoundBankId = SoundBank->Id;
				SoundBankReferenceCount.FindOrAdd(SoundBankId, 0)++;
			}
		}
	}

	if (Shared.IsCancelled())
	{
		MultiReferencedMediaIds.Empty();
		MultiReferencedSoundBankIds.Empty();
		return;
	}

	for (const auto& Pair : MediaReferenceCount)
	{
		if (Pair.Value > 1)
		{
			MultiReferencedMediaIds.Add(Pair.Key);
		}
	}

	for (const auto& Pair : SoundBankReferenceCount)
	{
		if (Pair.Value > 1)
		{
			MultiReferencedSoundBankIds.Add(Pair.Key);
		}
	}
}

void UWwiseAssetLibraryFilterMultiReference::PostFilter(const FWwiseAssetLibraryFilteringSharedData& Shared,
	const FWwiseAssetLibraryInfo& AssetLibraryInfo)
{
	MultiReferencedMediaIds.Empty();
	MultiReferencedSoundBankIds.Empty();
}

#define LOCTEXT_NAMESPACE "AssetTypeActions"

FText FAssetTypeActions_WwiseAssetLibraryFilterMultiReference::GetName() const
{
	return LOCTEXT("AssetTypeActions_WwiseAssetLibraryFilterMultiReference", "Wwise Asset Library Filter : Multiple Reference");
}

UClass* FAssetTypeActions_WwiseAssetLibraryFilterMultiReference::GetSupportedClass() const
{
	return UWwiseAssetLibraryFilterMultiReference::StaticClass();
}

#undef LOCTEXT_NAMESPACE
