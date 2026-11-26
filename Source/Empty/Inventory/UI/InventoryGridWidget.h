#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Inventory/UI/InventorySlotWidget.h"
#include "InventoryGridWidget.generated.h"

class UInventoryComponent;
class UTextBlock;
class UUniformGridPanel;

/**
 * Reusable inventory grid widget that dynamically binds to any UInventoryComponent
 * (used for both the Player Inventory panel and the Storage Container panel).
 */
UCLASS(Abstract)
class EMPTY_API UInventoryGridWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	/** Forwarded from child slot widgets so the parent HUD knows which slot is currently hovered. */
	FOnInventorySlotHoverChanged OnSlotHoverChanged;

protected:

	/** Uniform grid container populated with UInventorySlotWidget instances. */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory|UI", meta = (BindWidget))
	TObjectPtr<UUniformGridPanel> SlotGrid;

	/** Optional header title populated from InventoryComponent::GetInventoryName(). */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory|UI", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TitleText;

	/** Slot widget Blueprint class instantiated for each slot in the bound inventory. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory|UI")
	TSubclassOf<UInventorySlotWidget> SlotWidgetClass;

	/** Number of columns per row in SlotGrid. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory|UI", meta = (ClampMin = "1", ClampMax = "20"))
	int32 Columns = 5;

	//~ Begin UUserWidget Interface
	virtual void NativeDestruct() override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	//~ End UUserWidget Interface

public:

	/** Binds this grid to InInventory, subscribing to slot delegates and building the slot widgets. */
	UFUNCTION(BlueprintCallable, Category = "Inventory|UI")
	void BindToInventory(UInventoryComponent* InInventory, UInventoryComponent* InCounterpartInventory = nullptr);

	/** Unsubscribes from the currently bound inventory and clears generated slot widgets. */
	UFUNCTION(BlueprintCallable, Category = "Inventory|UI")
	void UnbindInventory();

	/** Updates the counterpart inventory on all slot widgets (for Shift+Click quick transfers). */
	UFUNCTION(BlueprintCallable, Category = "Inventory|UI")
	void SetCounterpartInventory(UInventoryComponent* InCounterpartInventory);

	/** Returns the currently bound inventory component. */
	UFUNCTION(BlueprintPure, Category = "Inventory|UI")
	UInventoryComponent* GetBoundInventory() const { return BoundInventory.Get(); }

private:

	UFUNCTION()
	void HandleSlotUpdated(int32 SlotIndex);

	UFUNCTION()
	void HandleInventoryReset();

	void HandleChildSlotHoverChanged(UInventoryComponent* InInventory, int32 SlotIndex, bool bIsHovered);

	void RebuildGrid();

	TWeakObjectPtr<UInventoryComponent> BoundInventory;
	TWeakObjectPtr<UInventoryComponent> CounterpartInventory;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UInventorySlotWidget>> SlotWidgets;
};
