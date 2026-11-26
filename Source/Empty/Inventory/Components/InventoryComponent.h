#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Inventory/Data/InventoryTypes.h"
#include "InventoryComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInventorySlotUpdated, int32, SlotIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryReset);

/**
 * Reusable slot-based inventory component shared by both the Player and Storage Containers (chests, crates, etc.).
 * Acts as the single source of truth for slot state and atomic item operations; UI widgets observe it via delegates.
 */
UCLASS(ClassGroup = (Inventory), meta = (BlueprintSpawnableComponent))
class EMPTY_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UInventoryComponent();

	/** Broadcast whenever a single slot's contents or state change. */
	UPROPERTY(BlueprintAssignable, Category = "Inventory|Events")
	FOnInventorySlotUpdated OnSlotUpdated;

	/** Broadcast after bulk state changes (initialization, resize, clear, or future sort/quick-stack). */
	UPROPERTY(BlueprintAssignable, Category = "Inventory|Events")
	FOnInventoryReset OnInventoryReset;

protected:

	/** Display title shown in the inventory UI header (e.g., "Backpack", "Wooden Chest"). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
	FText InventoryName;

	/** Fixed number of slots in this inventory. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory", meta = (ClampMin = "1", ClampMax = "200"))
	int32 Capacity = 20;

	/** Optional items granted automatically during BeginPlay (useful for starting gear or pre-filled chests). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
	TArray<FInventoryItem> InitialItems;

	/** Fixed-size array of inventory slots. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Inventory")
	TArray<FInventorySlot> Slots;

	//~ Begin UActorComponent Interface
	virtual void BeginPlay() override;
	//~ End UActorComponent Interface

public:

	// -------------------------------------------------------------------------
	// Configuration & Queries
	// -------------------------------------------------------------------------

	/** Initializes or resizes the slot array to NewCapacity and broadcasts OnInventoryReset. */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void InitializeCapacity(int32 NewCapacity);

	/** Returns the display title of this inventory. */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	const FText& GetInventoryName() const { return InventoryName; }

	/** Updates the display title of this inventory. */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SetInventoryName(const FText& NewName) { InventoryName = NewName; }

	/** Returns the configured slot count. */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetCapacity() const { return Slots.Num() > 0 ? Slots.Num() : Capacity; }

	/** Returns true if SlotIndex is within valid slot bounds. */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool IsValidSlotIndex(int32 SlotIndex) const;

	/** Returns the slot at SlotIndex (or a static empty slot if out of bounds). */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	const FInventorySlot& GetSlot(int32 SlotIndex) const;

	/** Returns read-only access to all slots. */
	const TArray<FInventorySlot>& GetAllSlots() const { return Slots; }

	/** Returns true if SlotIndex is valid and holds no item. */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool IsSlotEmpty(int32 SlotIndex) const;

	/** Returns total quantity of all stacks matching Definition across the entire inventory. */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetTotalQuantityOfItem(const UItemDefinition* Definition) const;

	/** Returns how many units of Item could currently fit into this inventory. */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 ComputeAvailableSpaceFor(const FInventoryItem& Item) const;

	/** Returns true if at least one unit of Item can fit into this inventory. */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool CanAcceptItem(const FInventoryItem& Item) const;

	/** Returns true if any slot holds a stackable item matching Definition (used by future Quick Stack). */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool HasStackableItem(const UItemDefinition* Definition) const;

	// -------------------------------------------------------------------------
	// Mutations
	// -------------------------------------------------------------------------

	/**
	 * Attempts to add Item into this inventory.
	 * Fills existing compatible partial stacks first, then spills into empty slots.
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	FInventoryAddResult TryAddItem(const FInventoryItem& Item);

	/**
	 * Removes up to Quantity units from SlotIndex (or the entire stack if Quantity <= 0).
	 * Returns the removed FInventoryItem stack.
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	FInventoryItem RemoveFromSlot(int32 SlotIndex, int32 Quantity = -1);

	/** Toggles the slot lock flag (used by future Sort and Quick Stack to protect pinned slots). */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool SetSlotLocked(int32 SlotIndex, bool bLocked);

	/** Clears every slot and broadcasts OnInventoryReset. */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void ClearAllSlots();

	// -------------------------------------------------------------------------
	// Cross-Inventory & Intra-Inventory Operations
	// -------------------------------------------------------------------------

	/**
	 * Atomically moves, merges, splits, or swaps between Source[SourceIndex] and Target[TargetIndex].
	 * Works identically within a single inventory or across two inventories (e.g., Player <-> Chest).
	 * Passing Quantity <= 0 moves the full stack.
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	static bool TransferSlot(
		UInventoryComponent* Source,
		int32 SourceIndex,
		UInventoryComponent* Target,
		int32 TargetIndex,
		int32 Quantity = -1);

	/**
	 * Moves as much of Source[SourceIndex] as fits into Target using standard stack-first placement.
	 * Intended for Shift+Click quick transfers between Player Inventory and open Storage.
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	static FInventoryAddResult QuickTransferSlot(
		UInventoryComponent* Source,
		int32 SourceIndex,
		UInventoryComponent* Target);

private:

	/** Ensures the Slots array is allocated to Capacity even if accessed before BeginPlay. */
	void EnsureSlotsInitialized();
};
