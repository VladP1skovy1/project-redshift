#include "Inventory/UI/InventorySlotWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "Inventory/Components/InventoryComponent.h"
#include "Inventory/UI/ItemDragDropOperation.h"

void UInventorySlotWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!bIsDragPreview)
	{
		SetHoveredVisual(false);
	}
}

void UInventorySlotWidget::InitializeSlot(
	UInventoryComponent* InInventory,
	int32 InSlotIndex,
	UInventoryComponent* InCounterpartInventory)
{
	Inventory = InInventory;
	SlotIndex = InSlotIndex;
	CounterpartInventory = InCounterpartInventory;
	bIsDragPreview = false;

	RefreshSlot();
}

void UInventorySlotWidget::SetCounterpartInventory(UInventoryComponent* InCounterpartInventory)
{
	CounterpartInventory = InCounterpartInventory;
}

void UInventorySlotWidget::RefreshSlot()
{
	if (!Inventory.IsValid() || !Inventory->IsValidSlotIndex(SlotIndex))
	{
		ApplyItemVisuals(FInventoryItem());
		return;
	}

	const FInventorySlot& SlotData = Inventory->GetSlot(SlotIndex);
	ApplyItemVisuals(SlotData.Item);
}

void UInventorySlotWidget::ConfigureAsDragPreview(const FInventoryItem& PreviewItem)
{
	bIsDragPreview = true;
	SetVisibility(ESlateVisibility::HitTestInvisible);
	ApplyItemVisuals(PreviewItem);
	SetToolTipText(FText::GetEmpty());
}

void UInventorySlotWidget::ApplyItemVisuals(const FInventoryItem& Item)
{
	const bool bHasItem = Item.IsValid();

	if (ItemIcon)
	{
		if (bHasItem)
		{
			ItemIcon->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			if (Item.Definition->Icon)
			{
				ItemIcon->SetBrushFromTexture(Item.Definition->Icon, false);
			}
			else
			{
				ItemIcon->SetBrushFromTexture(nullptr, false);
			}
		}
		else
		{
			ItemIcon->SetVisibility(ESlateVisibility::Hidden);
		}
	}

	if (QuantityText)
	{
		if (bHasItem && Item.Quantity > 1)
		{
			QuantityText->SetText(FText::AsNumber(Item.Quantity));
			QuantityText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
		else
		{
			QuantityText->SetText(FText::GetEmpty());
			QuantityText->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	if (!bIsDragPreview)
	{
		if (bHasItem)
		{
			const FText ItemName = Item.Definition->GetEffectiveDisplayName();
			if (!Item.Definition->Description.IsEmpty())
			{
				SetToolTipText(FText::Format(
					NSLOCTEXT("Inventory", "SlotTooltipWithDesc", "{0}\n{1}"),
					ItemName,
					Item.Definition->Description));
			}
			else
			{
				SetToolTipText(ItemName);
			}
		}
		else
		{
			SetToolTipText(FText::GetEmpty());
		}
	}
}

void UInventorySlotWidget::SetHoveredVisual(bool bHovered)
{
	if (SlotBorder)
	{
		SlotBorder->SetBrushColor(bHovered ? HoveredBorderColor : NormalBorderColor);
	}
}

void UInventorySlotWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);

	if (bIsDragPreview)
	{
		return;
	}

	SetHoveredVisual(true);
	OnSlotHoverChanged.ExecuteIfBound(Inventory.Get(), SlotIndex, true);
}

void UInventorySlotWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);

	if (bIsDragPreview)
	{
		return;
	}

	SetHoveredVisual(false);
	OnSlotHoverChanged.ExecuteIfBound(Inventory.Get(), SlotIndex, false);
}

FReply UInventorySlotWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bIsDragPreview || !Inventory.IsValid() || Inventory->IsSlotEmpty(SlotIndex))
	{
		return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
	}

	const FKey PressedButton = InMouseEvent.GetEffectingButton();

	// Shift + Left-Click: Quick transfer to the open counterpart inventory (Player <-> Storage).
	if (PressedButton == EKeys::LeftMouseButton && InMouseEvent.IsShiftDown() && CounterpartInventory.IsValid())
	{
		UInventoryComponent::QuickTransferSlot(Inventory.Get(), SlotIndex, CounterpartInventory.Get());
		return FReply::Handled();
	}

	// Left-Click drag: full stack. Right-Click drag: split half stack.
	if (PressedButton == EKeys::LeftMouseButton || PressedButton == EKeys::RightMouseButton)
	{
		return UWidgetBlueprintLibrary::DetectDragIfPressed(InMouseEvent, this, PressedButton).NativeReply;
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void UInventorySlotWidget::NativeOnDragDetected(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent,
	UDragDropOperation*& OutOperation)
{
	Super::NativeOnDragDetected(InGeometry, InMouseEvent, OutOperation);

	if (bIsDragPreview || !Inventory.IsValid() || Inventory->IsSlotEmpty(SlotIndex))
	{
		return;
	}

	const FInventorySlot& SlotData = Inventory->GetSlot(SlotIndex);
	const bool bIsSplitDrag = InMouseEvent.IsMouseButtonDown(EKeys::RightMouseButton);
	const int32 DragQty = (bIsSplitDrag && SlotData.Item.Quantity > 1)
		? FMath::Max(1, SlotData.Item.Quantity / 2)
		: -1;

	UItemDragDropOperation* DragOp = NewObject<UItemDragDropOperation>(this);
	DragOp->SourceInventory = Inventory;
	DragOp->SourceSlotIndex = SlotIndex;
	DragOp->DragQuantity = DragQty;
	DragOp->Pivot = EDragPivot::CenterCenter;

	TSubclassOf<UInventorySlotWidget> PreviewClass = DragVisualClass ? DragVisualClass : TSubclassOf<UInventorySlotWidget>(GetClass());
	if (UInventorySlotWidget* DragVisual = CreateWidget<UInventorySlotWidget>(GetOwningPlayer(), PreviewClass))
	{
		FInventoryItem PreviewItem = SlotData.Item;
		if (DragQty > 0)
		{
			PreviewItem.Quantity = DragQty;
		}
		DragVisual->ConfigureAsDragPreview(PreviewItem);
		DragOp->DefaultDragVisual = DragVisual;
	}

	OutOperation = DragOp;
}

bool UInventorySlotWidget::NativeOnDrop(
	const FGeometry& InGeometry,
	const FDragDropEvent& InDragDropEvent,
	UDragDropOperation* InOperation)
{
	if (bIsDragPreview || !Inventory.IsValid())
	{
		return false;
	}

	const UItemDragDropOperation* ItemDragOp = Cast<UItemDragDropOperation>(InOperation);
	if (!ItemDragOp || !ItemDragOp->SourceInventory.IsValid())
	{
		return false;
	}

	UInventoryComponent::TransferSlot(
		ItemDragOp->SourceInventory.Get(),
		ItemDragOp->SourceSlotIndex,
		Inventory.Get(),
		SlotIndex,
		ItemDragOp->DragQuantity);

	return true;
}
