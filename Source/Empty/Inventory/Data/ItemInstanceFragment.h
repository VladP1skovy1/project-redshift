#pragma once

#include "CoreMinimal.h"
#include "ItemInstanceFragment.generated.h"

/**
 * Base struct for optional per-instance item data (e.g., durability, ammo, custom modifiers).
 * Derive concrete USTRUCTs from this type and attach them to UItemDefinition::DefaultInstanceFragments
 * or FInventoryItem::InstanceFragments via FInstancedStruct.
 */
USTRUCT(BlueprintType)
struct EMPTY_API FItemInstanceFragment
{
	GENERATED_BODY()
};
