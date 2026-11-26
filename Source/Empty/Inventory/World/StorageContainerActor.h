#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/InteractableInterface.h"
#include "StorageContainerActor.generated.h"

class UInventoryComponent;
class UStaticMeshComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStorageInteracted, AActor*, Interactor);

/**
 * World storage object (e.g., chest, crate, locker) that owns a UInventoryComponent
 * and opens alongside the player's inventory when interacted with.
 */
UCLASS(Blueprintable)
class EMPTY_API AStorageContainerActor : public AActor, public IInteractableInterface
{
	GENERATED_BODY()

public:

	AStorageContainerActor();

	/** Broadcast when an actor interacts with this container (useful for open animations or audio in Blueprints). */
	UPROPERTY(BlueprintAssignable, Category = "Storage|Events")
	FOnStorageInteracted OnStorageInteracted;

protected:

	/** Visual mesh and collision body for the storage container. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	/** Reusable inventory component storing this container's items. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UInventoryComponent> InventoryComponent;

	/** Display name shown in the interaction prompt and storage UI header. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Storage")
	FText ContainerName;

	/** Maximum distance (in cm) at which the container UI remains open before auto-closing. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Storage", meta = (ClampMin = "100.0"))
	float MaxOpenDistance = 350.0f;

	//~ Begin AActor Interface
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	//~ End AActor Interface

public:

	/** Returns this container's inventory component. */
	UFUNCTION(BlueprintPure, Category = "Storage")
	UInventoryComponent* GetInventoryComponent() const { return InventoryComponent; }

	/** Returns the display name of this container. */
	UFUNCTION(BlueprintPure, Category = "Storage")
	const FText& GetContainerName() const { return ContainerName; }

	/** Returns the maximum distance at which this storage should stay open. */
	UFUNCTION(BlueprintPure, Category = "Storage")
	float GetMaxOpenDistance() const { return MaxOpenDistance; }

	//~ Begin IInteractableInterface
	virtual FText GetInteractionPrompt_Implementation(AActor* Interactor) const override;
	virtual bool CanInteract_Implementation(AActor* Interactor) const override;
	virtual void Interact_Implementation(AActor* Interactor) override;
	//~ End IInteractableInterface
};
