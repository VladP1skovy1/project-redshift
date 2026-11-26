#include "Inventory/Data/ItemDefinition.h"

FText UItemDefinition::GetEffectiveDisplayName() const
{
	if (!DisplayName.IsEmpty())
	{
		return DisplayName;
	}

	return FText::FromName(GetEffectiveItemId());
}

FPrimaryAssetId UItemDefinition::GetPrimaryAssetId() const
{
	static const FPrimaryAssetType ItemAssetType(TEXT("ItemDefinition"));
	return FPrimaryAssetId(ItemAssetType, GetEffectiveItemId());
}
