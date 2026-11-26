#include "Inventory/Components/InventoryComponent.h"
#include "Empty.h"

UInventoryComponent::UInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	InventoryName = NSLOCTEXT("Inventory", "DefaultInventoryName", "Inventory");
}

void UInventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	EnsureSlotsInitialized();

	for (const FInventoryItem& ConfiguredItem : InitialItems)
	{
		if (!ConfiguredItem.IsValid())
		{
			continue;
		}

		// Populate default instance fragments if the designer left InstanceFragments empty.
		FInventoryItem ItemToAdd = ConfiguredItem;
		if (ItemToAdd.InstanceFragments.IsEmpty() && ItemToAdd.Definition)
		{
			ItemToAdd.InstanceFragments = ItemToAdd.Definition->DefaultInstanceFragments;
		}

		TryAddItem(ItemToAdd);
	}
}

void UInventoryComponent::EnsureSlotsInitialized()
{
	const int32 TargetCapacity = FMath::Max(1, Capacity);
	if (Slots.Num() != TargetCapacity)
	{
		Slots.SetNum(TargetCapacity);
	}
}

void UInventoryComponent::InitializeCapacity(int32 NewCapacity)
{
	Capacity = FMath::Max(1, NewCapacity);
	Slots.SetNum(Capacity);
	OnInventoryReset.Broadcast();
}

bool UInventoryComponent::IsValidSlotIndex(int32 SlotIndex) const
{
	return Slots.IsValidIndex(SlotIndex);
}

const FInventorySlot& UInventoryComponent::GetSlot(int32 SlotIndex) const
{
	static const FInventorySlot EmptySlot;
	if (!Slots.IsValidIndex(SlotIndex))
	{
		UE_LOG(LogEmpty, Warning, TEXT("UInventoryComponent::GetSlot - Invalid SlotIndex %d (Capacity: %d) on '%s'."),
			SlotIndex, Slots.Num(), *GetNameSafe(GetOwner()));
		return EmptySlot;
	}

	return Slots[SlotIndex];
}

bool UInventoryComponent::IsSlotEmpty(int32 SlotIndex) const
{
	return !Slots.IsValidIndex(SlotIndex) || Slots[SlotIndex].IsEmpty();
}

int32 UInventoryComponent::GetTotalQuantityOfItem(const UItemDefinition* Definition) const
{
	if (!Definition)
	{
		return 0;
	}

	int32 Total = 0;
	for (const FInventorySlot& Slot : Slots)
	{
		if (Slot.Item.Definition == Definition)
		{
			Total += Slot.Item.Quantity;
		}
	}
	return Total;
}

int32 UInventoryComponent::ComputeAvailableSpaceFor(const FInventoryItem& Item) const
{
	if (!Item.IsValid())
	{
		return 0;
	}

	const int32 MaxStack = Item.GetMaxStackSize();
	const int32 EffectiveSlotCount = Slots.Num() > 0 ? Slots.Num() : FMath::Max(1, Capacity);

	// Before BeginPlay, all slots are empty.
	if (Slots.IsEmpty())
	{
		return EffectiveSlotCount * MaxStack;
	}

	int32 AvailableSpace = 0;
	for (const FInventorySlot& Slot : Slots)
	{
		if (Slot.IsEmpty())
		{
			AvailableSpace += MaxStack;
		}
		else if (Slot.Item.CanStackWith(Item))
		{
			AvailableSpace += Slot.Item.GetRemainingStackSpace();
		}
	}

	return AvailableSpace;
}

bool UInventoryComponent::CanAcceptItem(const FInventoryItem& Item) const
{
	return ComputeAvailableSpaceFor(Item) > 0;
}

bool UInventoryComponent::HasStackableItem(const UItemDefinition* Definition) const
{
	if (!Definition || !Definition->IsStackable())
	{
		return false;
	}

	for (const FInventorySlot& Slot : Slots)
	{
		if (Slot.Item.IsValid() && Slot.Item.Definition == Definition)
		{
			return true;
		}
	}

	return false;
}

FInventoryAddResult UInventoryComponent::TryAddItem(const FInventoryItem& Item)
{
	FInventoryAddResult Result;
	if (!Item.IsValid())
	{
		return Result;
	}

	EnsureSlotsInitialized();

	int32 Remaining = Item.Quantity;
	const int32 MaxStack = Item.GetMaxStackSize();
	TArray<int32, TInlineAllocator<8>> ModifiedSlots;

	// Pass 1: Merge into existing compatible partial stacks.
	if (Item.Definition->IsStackable())
	{
		for (int32 SlotIdx = 0; SlotIdx < Slots.Num() && Remaining > 0; ++SlotIdx)
		{
			FInventorySlot& Slot = Slots[SlotIdx];
			if (!Slot.IsEmpty() && Slot.Item.CanStackWith(Item))
			{
				const int32 Space = Slot.Item.GetRemainingStackSpace();
				if (Space > 0)
				{
					const int32 AmountToAdd = FMath::Min(Space, Remaining);
					Slot.Item.Quantity += AmountToAdd;
					Remaining -= AmountToAdd;
					ModifiedSlots.Add(SlotIdx);
				}
			}
		}
	}

	// Pass 2: Place remaining units into empty slots.
	for (int32 SlotIdx = 0; SlotIdx < Slots.Num() && Remaining > 0; ++SlotIdx)
	{
		FInventorySlot& Slot = Slots[SlotIdx];
		if (Slot.IsEmpty())
		{
			const int32 AmountToAdd = FMath::Min(MaxStack, Remaining);
			Slot.Item.Definition = Item.Definition;
			Slot.Item.Quantity = AmountToAdd;
			Slot.Item.InstanceFragments = Item.InstanceFragments;

			Remaining -= AmountToAdd;
			ModifiedSlots.Add(SlotIdx);
		}
	}

	Result.AddedQuantity = Item.Quantity - Remaining;
	Result.RemainderQuantity = Remaining;

	for (const int32 ModifiedIdx : ModifiedSlots)
	{
		OnSlotUpdated.Broadcast(ModifiedIdx);
	}

	return Result;
}

FInventoryItem UInventoryComponent::RemoveFromSlot(int32 SlotIndex, int32 Quantity)
{
	EnsureSlotsInitialized();

	if (!IsValidSlotIndex(SlotIndex) || Slots[SlotIndex].IsEmpty())
	{
		return FInventoryItem();
	}

	FInventorySlot& Slot = Slots[SlotIndex];
	const int32 AmountToRemove = (Quantity <= 0 || Quantity >= Slot.Item.Quantity)
		? Slot.Item.Quantity
		: Quantity;

	FInventoryItem Removed = Slot.Item.Split(AmountToRemove);
	OnSlotUpdated.Broadcast(SlotIndex);
	return Removed;
}

bool UInventoryComponent::SetSlotLocked(int32 SlotIndex, bool bLocked)
{
	EnsureSlotsInitialized();

	if (!IsValidSlotIndex(SlotIndex))
	{
		return false;
	}

	if (Slots[SlotIndex].bIsLocked == bLocked)
	{
		return false;
	}

	Slots[SlotIndex].bIsLocked = bLocked;
	OnSlotUpdated.Broadcast(SlotIndex);
	return true;
}

void UInventoryComponent::ClearAllSlots()
{
	EnsureSlotsInitialized();

	for (FInventorySlot& Slot : Slots)
	{
		Slot.Clear();
	}

	OnInventoryReset.Broadcast();
}

bool UInventoryComponent::TransferSlot(
	UInventoryComponent* Source,
	int32 SourceIndex,
	UInventoryComponent* Target,
	int32 TargetIndex,
	int32 Quantity)
{
	if (!Source || !Target)
	{
		return false;
	}

	Source->EnsureSlotsInitialized();
	Target->EnsureSlotsInitialized();

	if (!Source->IsValidSlotIndex(SourceIndex) || !Target->IsValidSlotIndex(TargetIndex))
	{
		return false;
	}

	if (Source == Target && SourceIndex == TargetIndex)
	{
		return false;
	}

	FInventorySlot& SourceSlot = Source->Slots[SourceIndex];
	FInventorySlot& TargetSlot = Target->Slots[TargetIndex];

	if (SourceSlot.IsEmpty())
	{
		return false;
	}

	const int32 MoveAmount = (Quantity <= 0 || Quantity >= SourceSlot.Item.Quantity)
		? SourceSlot.Item.Quantity
		: Quantity;
	const bool bIsFullMove = (MoveAmount == SourceSlot.Item.Quantity);

	// Case 1: Target slot is empty -> move or split into target.
	if (TargetSlot.IsEmpty())
	{
		TargetSlot.Item = SourceSlot.Item.Split(MoveAmount);
		Source->OnSlotUpdated.Broadcast(SourceIndex);
		Target->OnSlotUpdated.Broadcast(TargetIndex);
		return true;
	}

	// Case 2: Compatible stackable items -> merge up to MaxStackSize, or swap full stacks if target is full.
	if (TargetSlot.Item.CanStackWith(SourceSlot.Item))
	{
		const int32 Space = TargetSlot.Item.GetRemainingStackSpace();
		if (Space > 0)
		{
			const int32 AmountToMerge = FMath::Min(Space, MoveAmount);
			SourceSlot.Item.Split(AmountToMerge);
			TargetSlot.Item.Quantity += AmountToMerge;

			Source->OnSlotUpdated.Broadcast(SourceIndex);
			Target->OnSlotUpdated.Broadcast(TargetIndex);
			return true;
		}

		// Target stack is already full; swap only if moving a full stack of different size.
		if (bIsFullMove && SourceSlot.Item.Quantity != TargetSlot.Item.Quantity)
		{
			Swap(SourceSlot.Item, TargetSlot.Item);
			Source->OnSlotUpdated.Broadcast(SourceIndex);
			Target->OnSlotUpdated.Broadcast(TargetIndex);
			return true;
		}

		return false;
	}

	// Case 3: Different or non-stackable items -> swap only when moving the entire stack.
	if (bIsFullMove)
	{
		Swap(SourceSlot.Item, TargetSlot.Item);
		Source->OnSlotUpdated.Broadcast(SourceIndex);
		Target->OnSlotUpdated.Broadcast(TargetIndex);
		return true;
	}

	return false;
}

FInventoryAddResult UInventoryComponent::QuickTransferSlot(
	UInventoryComponent* Source,
	int32 SourceIndex,
	UInventoryComponent* Target)
{
	FInventoryAddResult Result;
	if (!Source || !Target || Source == Target)
	{
		return Result;
	}

	Source->EnsureSlotsInitialized();
	Target->EnsureSlotsInitialized();

	if (!Source->IsValidSlotIndex(SourceIndex) || Source->IsSlotEmpty(SourceIndex))
	{
		return Result;
	}

	Result = Target->TryAddItem(Source->Slots[SourceIndex].Item);
	if (Result.WasAnythingAdded())
	{
		Source->RemoveFromSlot(SourceIndex, Result.AddedQuantity);
	}

	return Result;
}
