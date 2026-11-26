#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InventoryHUDWidget.generated.h"

class UInventoryComponent;
class UInventoryGridWidget;
class UWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryCloseRequested);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
	FOnInventoryItemDropRequested,
	UInventoryComponent*, SourceInventory,
	int32, SlotIndex,
	int32, Quantity);

/**
 * Root inventory screen overlay displaying the Player Inventory panel and, when interacting with a
 * storage container, the Storage panel alongside it. Dropping dragged items outside the panels or
 * pressing Q over a hovered slot requests dropping the item into the world.
 */
UCLASS(Abstract)
class EMPTY_API UInventoryHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	UInventoryHUDWidget(const FObjectInitializer& ObjectInitializer);

	/** Fired when the user presses Escape while the inventory screen has focus. */
	UPROPERTY(BlueprintAssignable, Category = "Inventory|Events")
	FOnInventoryCloseRequested OnCloseRequested;

	/** Fired when the user drops an item outside the inventory panels or presses Q over a hovered slot. */
	UPROPERTY(BlueprintAssignable, Category = "Inventory|Events")
	FOnInventoryItemDropRequested OnItemDropRequested;

protected:

	/** Grid displaying the player's inventory. */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory|UI", meta = (BindWidget))
	TObjectPtr<UInventoryGridWidget> PlayerInventoryGrid;

	/** Grid displaying the currently open storage container's inventory. */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory|UI", meta = (BindWidget))
	TObjectPtr<UInventoryGridWidget> StorageInventoryGrid;

	/** Wrapper panel around StorageInventoryGrid shown only when a storage container is open. */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory|UI", meta = (BindWidget))
	TObjectPtr<UWidget> StoragePanelContainer;

	//~ Begin UUserWidget Interface
	virtual void NativeConstruct() override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	//~ End UUserWidget Interface

public:

	/** Opens the overlay showing only the player's inventory. */
	UFUNCTION(BlueprintCallable, Category = "Inventory|UI")
	void ShowPlayerInventory(UInventoryComponent* PlayerInventory);

	/** Opens the overlay showing both the player's inventory and an open storage container side by side. */
	UFUNCTION(BlueprintCallable, Category = "Inventory|UI")
	void ShowStorageInventory(UInventoryComponent* PlayerInventory, UInventoryComponent* StorageInventory);

	/** Closes the overlay and unbinds any active storage container. */
	UFUNCTION(BlueprintCallable, Category = "Inventory|UI")
	void HideInventory();

private:

	void HandleSlotHoverChanged(UInventoryComponent* Inventory, int32 SlotIndex, bool bIsHovered);
	void ClearHoveredSlot();

	TWeakObjectPtr<UInventoryComponent> HoveredInventory;
	int32 HoveredSlotIndex = INDEX_NONE;
};
