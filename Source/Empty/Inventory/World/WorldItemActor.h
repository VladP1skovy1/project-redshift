#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/InteractableInterface.h"
#include "Inventory/Data/InventoryTypes.h"
#include "WorldItemActor.generated.h"

class UStaticMeshComponent;

/**
 * Physical representation of an FInventoryItem lying in the world.
 * Supports both level-placed pickups and items dynamically dropped from an inventory.
 */
UCLASS(Blueprintable)
class EMPTY_API AWorldItemActor : public AActor, public IInteractableInterface
{
	GENERATED_BODY()

public:

	AWorldItemActor();

protected:

	/** Static mesh displaying the item's WorldMesh and providing physics/trace collision. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	/** The item stack contained by this world pickup. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
	FInventoryItem Item;

	/** Whether this pickup should simulate physics when Play begins (always enabled for dropped items). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
	bool bSimulatePhysicsOnBeginPlay = true;

	//~ Begin AActor Interface
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	//~ End AActor Interface

public:

	/** Configures this actor's item payload, refreshes its mesh, and applies physics settings. */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void InitializeItem(const FInventoryItem& InItem, bool bEnablePhysics = true);

	/** Syncs MeshComponent's static mesh from Item.Definition->WorldMesh. */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void RefreshVisuals();

	/** Returns the item stack currently represented by this world actor. */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	const FInventoryItem& GetItem() const { return Item; }

	/**
	 * Spawns a configured AWorldItemActor in World using deferred spawning so visuals and physics
	 * are initialized cleanly before BeginPlay.
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory", meta = (WorldContext = "WorldContextObject"))
	static AWorldItemActor* SpawnWorldItem(
		UObject* WorldContextObject,
		TSubclassOf<AWorldItemActor> ActorClass,
		const FInventoryItem& InItem,
		const FTransform& SpawnTransform,
		const FVector& LaunchVelocity = FVector::ZeroVector);

	//~ Begin IInteractableInterface
	virtual FText GetInteractionPrompt_Implementation(AActor* Interactor) const override;
	virtual bool CanInteract_Implementation(AActor* Interactor) const override;
	virtual void Interact_Implementation(AActor* Interactor) override;
	//~ End IInteractableInterface
};
