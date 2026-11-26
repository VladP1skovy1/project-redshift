#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "InteractableInterface.generated.h"

UINTERFACE(MinimalAPI, BlueprintType)
class UInteractableInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * Interface for any world actor the player can target and interact with (world item pickups,
 * storage chests, future doors, switches, NPCs, etc.).
 */
class EMPTY_API IInteractableInterface
{
	GENERATED_BODY()

public:

	/** Returns the contextual HUD prompt shown while targeting this actor (e.g., "Pick up Health Potion"). */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	FText GetInteractionPrompt(AActor* Interactor) const;

	/** Returns true if Interactor is currently allowed to interact with this actor. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	bool CanInteract(AActor* Interactor) const;

	/** Executes the interaction initiated by Interactor. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	void Interact(AActor* Interactor);
};
