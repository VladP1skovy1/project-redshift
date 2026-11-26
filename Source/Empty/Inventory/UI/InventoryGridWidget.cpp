#include "Inventory/UI/InventoryGridWidget.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Inventory/Components/InventoryComponent.h"
#include "Inventory/UI/ItemDragDropOperation.h"
#include "Empty.h"

void UInventoryGridWidget::NativeDestruct()
{
	UnbindInventory();
	Super::NativeDestruct();
}

bool UInventoryGridWidget::NativeOnDrop(
	const FGeometry& InGeometry,
	const FDragDropEvent& InDragDropEvent,
	UDragDropOperation* InOperation)
{
	// Consume accidental drops onto padding between slots inside the grid so they don't fall through to the world drop zone.
	if (Cast<UItemDragDropOperation>(InOperation) != nullptr)
	{
		return true;
	}

	return Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);
}

void UInventoryGridWidget::BindToInventory(UInventoryComponent* InInventory, UInventoryComponent* InCounterpartInventory)
{
	if (BoundInventory.Get() == InInventory && InInventory != nullptr)
	{
		SetCounterpartInventory(InCounterpartInventory);
		if (TitleText)
		{
			TitleText->SetText(InInventory->GetInventoryName());
		}
		if (SlotWidgets.Num() != InInventory->GetCapacity())
		{
			RebuildGrid();
		}
		else
		{
			for (UInventorySlotWidget* SlotWidget : SlotWidgets)
			{
				if (SlotWidget)
				{
					SlotWidget->RefreshSlot();
				}
			}
		}
		return;
	}

	UnbindInventory();

	BoundInventory = InInventory;
	CounterpartInventory = InCounterpartInventory;

	if (!BoundInventory.IsValid())
	{
		return;
	}

	BoundInventory->OnSlotUpdated.AddDynamic(this, &UInventoryGridWidget::HandleSlotUpdated);
	BoundInventory->OnInventoryReset.AddDynamic(this, &UInventoryGridWidget::HandleInventoryReset);

	if (TitleText)
	{
		TitleText->SetText(BoundInventory->GetInventoryName());
	}

	RebuildGrid();
}

void UInventoryGridWidget::UnbindInventory()
{
	if (BoundInventory.IsValid())
	{
		BoundInventory->OnSlotUpdated.RemoveDynamic(this, &UInventoryGridWidget::HandleSlotUpdated);
		BoundInventory->OnInventoryReset.RemoveDynamic(this, &UInventoryGridWidget::HandleInventoryReset);
	}

	BoundInventory.Reset();
	CounterpartInventory.Reset();
	SlotWidgets.Reset();

	if (SlotGrid)
	{
		SlotGrid->ClearChildren();
	}
}

void UInventoryGridWidget::SetCounterpartInventory(UInventoryComponent* InCounterpartInventory)
{
	CounterpartInventory = InCounterpartInventory;

	for (UInventorySlotWidget* SlotWidget : SlotWidgets)
	{
		if (SlotWidget)
		{
			SlotWidget->SetCounterpartInventory(InCounterpartInventory);
		}
	}
}

void UInventoryGridWidget::HandleSlotUpdated(int32 SlotIndex)
{
	if (SlotWidgets.IsValidIndex(SlotIndex) && SlotWidgets[SlotIndex])
	{
		SlotWidgets[SlotIndex]->RefreshSlot();
	}
}

void UInventoryGridWidget::HandleInventoryReset()
{
	RebuildGrid();
}

void UInventoryGridWidget::HandleChildSlotHoverChanged(UInventoryComponent* InInventory, int32 SlotIndex, bool bIsHovered)
{
	OnSlotHoverChanged.ExecuteIfBound(InInventory, SlotIndex, bIsHovered);
}

void UInventoryGridWidget::RebuildGrid()
{
	if (!SlotGrid || !BoundInventory.IsValid())
	{
		return;
	}

	SlotGrid->ClearChildren();
	SlotWidgets.Reset();

	if (!SlotWidgetClass)
	{
		UE_LOG(LogEmpty, Warning, TEXT("UInventoryGridWidget::RebuildGrid - SlotWidgetClass is not assigned on '%s'."), *GetName());
		return;
	}

	const int32 Capacity = BoundInventory->GetCapacity();
	const int32 SafeColumns = FMath::Max(1, Columns);
	SlotWidgets.Reserve(Capacity);

	for (int32 SlotIdx = 0; SlotIdx < Capacity; ++SlotIdx)
	{
		UInventorySlotWidget* SlotWidget = CreateWidget<UInventorySlotWidget>(GetOwningPlayer(), SlotWidgetClass);
		if (!SlotWidget)
		{
			continue;
		}

		SlotWidget->InitializeSlot(BoundInventory.Get(), SlotIdx, CounterpartInventory.Get());
		SlotWidget->OnSlotHoverChanged.BindUObject(this, &UInventoryGridWidget::HandleChildSlotHoverChanged);

		const int32 Row = SlotIdx / SafeColumns;
		const int32 Col = SlotIdx % SafeColumns;
		if (UUniformGridSlot* GridSlot = SlotGrid->AddChildToUniformGrid(SlotWidget, Row, Col))
		{
			GridSlot->SetHorizontalAlignment(HAlign_Fill);
			GridSlot->SetVerticalAlignment(VAlign_Fill);
		}

		SlotWidgets.Add(SlotWidget);
	}
}
