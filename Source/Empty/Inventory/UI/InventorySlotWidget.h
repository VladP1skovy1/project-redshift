#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Inventory/Data/InventoryTypes.h"
#include "InventorySlotWidget.generated.h"

class UBorder;
class UImage;
class UInventoryComponent;
class UTextBlock;

DECLARE_DELEGATE_ThreeParams(
	FOnInventorySlotHoverChanged,
	UInventoryComponent* /*Inventory*/,
	int32 /*SlotIndex*/,
	bool /*bIsHovered*/);

/**
 * Base UMG widget for a single inventory slot.
 * Supports left-click drag (full stack), right-click drag (half stack split), drop (move/merge/swap),
 * and Shift + Left-Click quick transfer to an open counterpart inventory.
 */
UCLASS(Abstract)
class EMPTY_API UInventorySlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	/** Fired when the cursor enters or leaves this slot (used by HUD to support hovered-slot drop hotkey). */
	FOnInventorySlotHoverChanged OnSlotHoverChanged;

protected:

	/** Item icon image (hidden when slot is empty). */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory|UI", meta = (BindWidget))
	TObjectPtr<UImage> ItemIcon;

	/** Stack count label (hidden when slot is empty or quantity <= 1). */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory|UI", meta = (BindWidget))
	TObjectPtr<UTextBlock> QuantityText;

	/** Optional background/frame border tinted on mouse hover. */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory|UI", meta = (BindWidgetOptional))
	TObjectPtr<UBorder> SlotBorder;

	/** Border tint when not hovered. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory|Style")
	FLinearColor NormalBorderColor = FLinearColor(0.12f, 0.12f, 0.14f, 0.90f);

	/** Border tint when hovered. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory|Style")
	FLinearColor HoveredBorderColor = FLinearColor(0.30f, 0.55f, 0.85f, 1.00f);

	/** Optional widget class used as the floating drag preview. Defaults to this widget's own Blueprint class. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory|UI")
	TSubclassOf<UInventorySlotWidget> DragVisualClass;

	//~ Begin UUserWidget Interface
	virtual void NativeConstruct() override;
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	//~ End UUserWidget Interface

public:

	/** Binds this widget to a specific slot in InInventory and optionally links a counterpart inventory for quick transfer. */
	UFUNCTION(BlueprintCallable, Category = "Inventory|UI")
	void InitializeSlot(
		UInventoryComponent* InInventory,
		int32 InSlotIndex,
		UInventoryComponent* InCounterpartInventory = nullptr);

	/** Updates the counterpart inventory used for Shift + Left-Click quick transfers. */
	UFUNCTION(BlueprintCallable, Category = "Inventory|UI")
	void SetCounterpartInventory(UInventoryComponent* InCounterpartInventory);

	/** Reads the bound slot from Inventory and refreshes visuals and tooltip. */
	UFUNCTION(BlueprintCallable, Category = "Inventory|UI")
	void RefreshSlot();

	/** Configures this widget as a non-interactive floating drag preview showing PreviewItem. */
	void ConfigureAsDragPreview(const FInventoryItem& PreviewItem);

	/** Returns the inventory component this slot represents. */
	UFUNCTION(BlueprintPure, Category = "Inventory|UI")
	UInventoryComponent* GetInventory() const { return Inventory.Get(); }

	/** Returns the slot index within Inventory. */
	UFUNCTION(BlueprintPure, Category = "Inventory|UI")
	int32 GetSlotIndex() const { return SlotIndex; }

private:

	/** Updates icon, quantity text, and tooltip from the given item state. */
	void ApplyItemVisuals(const FInventoryItem& Item);

	/** Updates SlotBorder brush tint if present. */
	void SetHoveredVisual(bool bHovered);

	TWeakObjectPtr<UInventoryComponent> Inventory;
	TWeakObjectPtr<UInventoryComponent> CounterpartInventory;
	int32 SlotIndex = INDEX_NONE;
	bool bIsDragPreview = false;
};
