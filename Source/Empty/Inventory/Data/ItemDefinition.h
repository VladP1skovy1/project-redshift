#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "StructUtils/InstancedStruct.h"
#include "ItemDefinition.generated.h"

class UStaticMesh;
class UTexture2D;

/** High-level item category used for UI organization, filtering, and future sorting. */
UENUM(BlueprintType)
enum class EItemCategory : uint8
{
	Misc		UMETA(DisplayName = "Misc"),
	Consumable	UMETA(DisplayName = "Consumable"),
	Resource	UMETA(DisplayName = "Resource"),
	Equipment	UMETA(DisplayName = "Equipment"),
	Tool		UMETA(DisplayName = "Tool")
};

/**
 * Immutable static metadata describing a type of item.
 * Shared across all stacks and world pickups of that item.
 */
UCLASS(BlueprintType, Const)
class EMPTY_API UItemDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

	/** Stable identifier for lookups, sorting, and future persistence. Defaults to asset name if left None. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Identity")
	FName ItemId;

	/** Human-readable name displayed in UI tooltips and interaction prompts. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Identity")
	FText DisplayName;

	/** Optional flavor or gameplay description displayed in UI tooltips. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Identity", meta = (MultiLine = true))
	FText Description;

	/** Classification used for grouping and future inventory sorting. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Classification")
	EItemCategory Category = EItemCategory::Misc;

	/** Secondary sort key within the same category (lower values sort first). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Classification")
	int32 SortPriority = 0;

	/** 2D icon rendered in inventory slots and drag previews. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Visuals")
	TObjectPtr<UTexture2D> Icon = nullptr;

	/** Static mesh rendered when this item lies in the world as a pickup. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Visuals")
	TObjectPtr<UStaticMesh> WorldMesh = nullptr;

	/** Maximum number of units allowed in a single inventory slot (1 = non-stackable). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Stacking", meta = (ClampMin = "1"))
	int32 MaxStackSize = 1;

	/**
	 * Optional per-instance data fragments cloned onto newly created FInventoryItem instances
	 * (e.g., initial durability or magazine count).
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Instance",
		meta = (BaseStruct = "/Script/Empty.ItemInstanceFragment", ExcludeBaseStruct))
	TArray<FInstancedStruct> DefaultInstanceFragments;

public:

	/** Returns true if more than one unit of this item can occupy the same slot. */
	UFUNCTION(BlueprintPure, Category = "Item")
	bool IsStackable() const { return MaxStackSize > 1; }

	/** Returns ItemId if explicitly configured, otherwise falls back to the asset FName. */
	UFUNCTION(BlueprintPure, Category = "Item")
	FName GetEffectiveItemId() const { return ItemId.IsNone() ? GetFName() : ItemId; }

	/** Returns DisplayName if non-empty, otherwise falls back to the asset name. */
	UFUNCTION(BlueprintPure, Category = "Item")
	FText GetEffectiveDisplayName() const;

	//~ Begin UObject Interface
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
	//~ End UObject Interface
};
