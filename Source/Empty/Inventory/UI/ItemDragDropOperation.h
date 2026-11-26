#pragma once

#include "CoreMinimal.h"
#include "Blueprint/DragDropOperation.h"
#include "ItemDragDropOperation.generated.h"

class UInventoryComponent;

/**
 * Payload carried while dragging an item stack between inventory slots or out into the world drop zone.
 * Source slot data is not mutated until a valid drop target accepts the operation, preventing item loss on cancel.
 */
UCLASS()
class EMPTY_API UItemDragDropOperation : public UDragDropOperation
{
	GENERATED_BODY()

public:

	/** Inventory component that owns the slot being dragged. */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	TWeakObjectPtr<UInventoryComponent> SourceInventory;

	/** Slot index within SourceInventory being dragged. */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	int32 SourceSlotIndex = INDEX_NONE;

	/** Number of units to move/drop (<= 0 means the entire stack). */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	int32 DragQuantity = -1;
};
