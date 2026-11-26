#include "Inventory/UI/InventoryHUDWidget.h"
#include "Inventory/Components/InventoryComponent.h"
#include "Inventory/UI/InventoryGridWidget.h"
#include "Inventory/UI/ItemDragDropOperation.h"

UInventoryHUDWidget::UInventoryHUDWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsFocusable(true);
}

void UInventoryHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (PlayerInventoryGrid)
	{
		PlayerInventoryGrid->OnSlotHoverChanged.BindUObject(this, &UInventoryHUDWidget::HandleSlotHoverChanged);
	}

	if (StorageInventoryGrid)
	{
		StorageInventoryGrid->OnSlotHoverChanged.BindUObject(this, &UInventoryHUDWidget::HandleSlotHoverChanged);
	}
}

void UInventoryHUDWidget::ShowPlayerInventory(UInventoryComponent* PlayerInventory)
{
	ClearHoveredSlot();

	if (PlayerInventoryGrid)
	{
		PlayerInventoryGrid->BindToInventory(PlayerInventory, nullptr);
	}

	if (StorageInventoryGrid)
	{
		StorageInventoryGrid->UnbindInventory();
	}

	if (StoragePanelContainer)
	{
		StoragePanelContainer->SetVisibility(ESlateVisibility::Collapsed);
	}

	SetVisibility(ESlateVisibility::Visible);
}

void UInventoryHUDWidget::ShowStorageInventory(UInventoryComponent* PlayerInventory, UInventoryComponent* StorageInventory)
{
	ClearHoveredSlot();

	if (PlayerInventoryGrid)
	{
		PlayerInventoryGrid->BindToInventory(PlayerInventory, StorageInventory);
	}

	if (StorageInventoryGrid)
	{
		StorageInventoryGrid->BindToInventory(StorageInventory, PlayerInventory);
	}

	if (StoragePanelContainer)
	{
		StoragePanelContainer->SetVisibility(ESlateVisibility::Visible);
	}

	SetVisibility(ESlateVisibility::Visible);
}

void UInventoryHUDWidget::HideInventory()
{
	ClearHoveredSlot();

	if (PlayerInventoryGrid)
	{
		PlayerInventoryGrid->SetCounterpartInventory(nullptr);
	}

	if (StorageInventoryGrid)
	{
		StorageInventoryGrid->UnbindInventory();
	}

	if (StoragePanelContainer)
	{
		StoragePanelContainer->SetVisibility(ESlateVisibility::Collapsed);
	}

	SetVisibility(ESlateVisibility::Collapsed);
}

void UInventoryHUDWidget::HandleSlotHoverChanged(UInventoryComponent* Inventory, int32 SlotIndex, bool bIsHovered)
{
	if (bIsHovered)
	{
		HoveredInventory = Inventory;
		HoveredSlotIndex = SlotIndex;
	}
	else if (HoveredInventory.Get() == Inventory && HoveredSlotIndex == SlotIndex)
	{
		ClearHoveredSlot();
	}
}

void UInventoryHUDWidget::ClearHoveredSlot()
{
	HoveredInventory.Reset();
	HoveredSlotIndex = INDEX_NONE;
}

bool UInventoryHUDWidget::NativeOnDrop(
	const FGeometry& InGeometry,
	const FDragDropEvent& InDragDropEvent,
	UDragDropOperation* InOperation)
{
	const UItemDragDropOperation* ItemDragOp = Cast<UItemDragDropOperation>(InOperation);
	if (!ItemDragOp || !ItemDragOp->SourceInventory.IsValid())
	{
		return Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);
	}

	OnItemDropRequested.Broadcast(
		ItemDragOp->SourceInventory.Get(),
		ItemDragOp->SourceSlotIndex,
		ItemDragOp->DragQuantity);

	return true;
}

FReply UInventoryHUDWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey PressedKey = InKeyEvent.GetKey();

	if (PressedKey == EKeys::Escape || PressedKey == EKeys::Tab)
	{
		OnCloseRequested.Broadcast();
		return FReply::Handled();
	}

	if (PressedKey == EKeys::Q && HoveredInventory.IsValid() && HoveredSlotIndex != INDEX_NONE)
	{
		if (!HoveredInventory->IsSlotEmpty(HoveredSlotIndex))
		{
			// Q drops the full stack; Shift+Q drops a single unit.
			const int32 Quantity = InKeyEvent.IsShiftDown() ? 1 : -1;
			OnItemDropRequested.Broadcast(HoveredInventory.Get(), HoveredSlotIndex, Quantity);
			return FReply::Handled();
		}
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}
