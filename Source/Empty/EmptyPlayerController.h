// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "EmptyPlayerController.generated.h"

class UInputAction;
class UInputMappingContext;
class UUserWidget;
class UInventoryComponent;
class UInventoryHUDWidget;
class UInteractionPromptWidget;
class AStorageContainerActor;

/**
 *  Simple first person Player Controller
 *  Manages the input mapping context.
 *  Overrides the Player Camera Manager class.
 */
UCLASS(abstract, config="Game")
class EMPTY_API AEmptyPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:

	/** Constructor */
	AEmptyPlayerController();

	/** Toggles the player inventory screen open/closed (or closes an open storage screen). */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void ToggleInventory();

	/** Opens the player inventory screen without a storage container. */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void OpenPlayerInventory();

	/** Opens a world storage container alongside the player inventory. */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	virtual void OpenStorage(AStorageContainerActor* StorageActor);

	/** Closes any open inventory or storage UI and restores gameplay input mode. */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void CloseInventory();

	/** Returns true while the inventory or storage UI is open. */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool IsInventoryOpen() const { return bIsInventoryOpen; }

protected:

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category="Input|Input Mappings")
	TArray<UInputMappingContext*> DefaultMappingContexts;

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category="Input|Input Mappings")
	TArray<UInputMappingContext*> MobileExcludedMappingContexts;

	/** Mobile controls widget to spawn */
	UPROPERTY(EditAnywhere, Category="Input|Touch Controls")
	TSubclassOf<UUserWidget> MobileControlsWidgetClass;

	/** Pointer to the mobile controls widget */
	UPROPERTY()
	TObjectPtr<UUserWidget> MobileControlsWidget;

	/** If true, the player will use UMG touch controls even if not playing on mobile platforms */
	UPROPERTY(EditAnywhere, Config, Category = "Input|Touch Controls")
	bool bForceTouchControls = false;

	/** Input action to toggle the player inventory (e.g. Tab / I) */
	UPROPERTY(EditAnywhere, Category = "Input|Inventory")
	TObjectPtr<UInputAction> ToggleInventoryAction;

	/** Widget Blueprint class for the root inventory & storage screen overlay */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory|UI")
	TSubclassOf<UInventoryHUDWidget> InventoryHUDWidgetClass;

	/** Widget Blueprint class for the crosshair interaction prompt */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory|UI")
	TSubclassOf<UInteractionPromptWidget> InteractionPromptWidgetClass;

	/** Spawned inventory overlay widget instance */
	UPROPERTY(Transient)
	TObjectPtr<UInventoryHUDWidget> InventoryHUDWidget;

	/** Spawned interaction prompt widget instance */
	UPROPERTY(Transient)
	TObjectPtr<UInteractionPromptWidget> InteractionPromptWidget;

	/** Storage container currently open in the UI, if any */
	TWeakObjectPtr<AStorageContainerActor> ActiveStorageActor;

	/** Tracks whether the inventory overlay is currently open */
	bool bIsInventoryOpen = false;

	/** Gameplay initialization */
	virtual void BeginPlay() override;

	/** Rebinds HUD interaction prompt when possessing a new pawn */
	virtual void OnPossess(APawn* InPawn) override;

	/** Auto-closes open storage containers if the player moves out of range */
	virtual void PlayerTick(float DeltaTime) override;

	/** Input mapping context setup */
	virtual void SetupInputComponent() override;

	/** Returns true if the player should use UMG touch controls */
	bool ShouldUseTouchControls() const;

private:

	/** Creates InventoryHUDWidget and InteractionPromptWidget once for local players */
	void InitializeInventoryUI();

	/** Binds InteractionPromptWidget to the possessed pawn's UInteractionComponent */
	void BindPromptToPawn(APawn* InPawn);

	/** Switches between gameplay input mode and inventory UI input mode */
	void SetInventoryInputMode(bool bOpen);

	/** Returns the possessed character's inventory component, or nullptr */
	UInventoryComponent* GetPlayerInventoryComponent() const;

	UFUNCTION()
	void HandleInventoryCloseRequested();

	UFUNCTION()
	void HandleItemDropRequested(UInventoryComponent* SourceInventory, int32 SlotIndex, int32 Quantity);
};
