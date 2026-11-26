#pragma once

#include "CoreMinimal.h"
#include "StructUtils/InstancedStruct.h"
#include "Inventory/Data/ItemDefinition.h"
#include "InventoryTypes.generated.h"

/** Ordering criteria reserved for future inventory sorting. */
UENUM(BlueprintType)
enum class EInventorySortMode : uint8
{
	Category	UMETA(DisplayName = "Category"),
	Name		UMETA(DisplayName = "Name"),
	Quantity	UMETA(DisplayName = "Quantity")
};

/**
 * Runtime item stack stored in an inventory slot or a world pickup actor.
 * Uses value semantics for zero GC churn while supporting extensible per-instance fragments.
 */
USTRUCT(BlueprintType)
struct EMPTY_API FInventoryItem
{
	GENERATED_BODY()

	/** Immutable definition asset describing what this item is. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	TObjectPtr<const UItemDefinition> Definition = nullptr;

	/** Number of units in this stack. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory", meta = (ClampMin = "0"))
	int32 Quantity = 0;

	/** Optional per-instance state fragments (e.g., durability, ammo, custom attributes). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory",
		meta = (BaseStruct = "/Script/Empty.ItemInstanceFragment", ExcludeBaseStruct))
	TArray<FInstancedStruct> InstanceFragments;

	/** Creates a valid item stack initialized with the definition's default instance fragments. */
	static FInventoryItem MakeFromDefinition(const UItemDefinition* InDefinition, int32 InQuantity = 1);

	/** Returns true if this item has a valid definition and a positive quantity. */
	bool IsValid() const { return Definition != nullptr && Quantity > 0; }

	/** Returns true if this item holds nothing. */
	bool IsEmpty() const { return !IsValid(); }

	/** Returns the maximum stack size allowed for this item's definition (0 if invalid). */
	int32 GetMaxStackSize() const { return Definition ? FMath::Max(1, Definition->MaxStackSize) : 0; }

	/** Returns how many additional units can be merged into this stack. */
	int32 GetRemainingStackSpace() const { return IsValid() ? FMath::Max(0, GetMaxStackSize() - Quantity) : 0; }

	/**
	 * Returns true if Other can merge into this stack.
	 * Requires the same stackable definition and identical per-instance fragments.
	 */
	bool CanStackWith(const FInventoryItem& Other) const;

	/**
	 * Splits up to SplitQuantity units out of this stack into a new FInventoryItem.
	 * Clears this stack if all units are removed.
	 */
	FInventoryItem Split(int32 SplitQuantity);

	/** Resets this item to an empty state. */
	void Reset();

	/** Finds a const pointer to the first instance fragment of type T, or nullptr if absent. */
	template <typename T>
	const T* FindFragment() const
	{
		for (const FInstancedStruct& Fragment : InstanceFragments)
		{
			if (const T* Typed = Fragment.GetPtr<T>())
			{
				return Typed;
			}
		}
		return nullptr;
	}

	/** Finds a mutable pointer to the first instance fragment of type T, or nullptr if absent. */
	template <typename T>
	T* FindMutableFragment()
	{
		for (FInstancedStruct& Fragment : InstanceFragments)
		{
			if (T* Typed = Fragment.GetMutablePtr<T>())
			{
				return Typed;
			}
		}
		return nullptr;
	}

	/** Returns the existing fragment of type T, or appends and returns a default-constructed one. */
	template <typename T>
	T& FindOrAddFragment()
	{
		if (T* Existing = FindMutableFragment<T>())
		{
			return *Existing;
		}
		FInstancedStruct& NewEntry = InstanceFragments.Emplace_GetRef();
		NewEntry.InitializeAs<T>();
		return NewEntry.GetMutable<T>();
	}
};

/**
 * Single slot entry within a UInventoryComponent.
 */
USTRUCT(BlueprintType)
struct EMPTY_API FInventorySlot
{
	GENERATED_BODY()

	/** Item currently occupying this slot (empty if !Item.IsValid()). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	FInventoryItem Item;

	/** When true, future Sort and Quick Stack operations will skip this slot. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	bool bIsLocked = false;

	bool IsEmpty() const { return Item.IsEmpty(); }
	void Clear() { Item.Reset(); }
};

/**
 * Outcome returned by UInventoryComponent::TryAddItem and QuickTransferSlot.
 */
USTRUCT(BlueprintType)
struct EMPTY_API FInventoryAddResult
{
	GENERATED_BODY()

	/** Number of units successfully added to the destination inventory. */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	int32 AddedQuantity = 0;

	/** Number of units that could not fit and remain unhandled. */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	int32 RemainderQuantity = 0;

	bool WasAnythingAdded() const { return AddedQuantity > 0; }
	bool WasFullyAdded() const { return AddedQuantity > 0 && RemainderQuantity == 0; }
};
