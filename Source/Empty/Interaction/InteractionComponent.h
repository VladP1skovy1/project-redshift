#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InteractionComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnFocusedInteractableChanged,
	AActor*, NewInteractable,
	const FText&, PromptText);

/**
 * Component attached to a player character that detects look-targeted IInteractableInterface actors
 * and routes interaction input cleanly without coupling the character to specific actor types.
 */
UCLASS(ClassGroup = (Interaction), meta = (BlueprintSpawnableComponent))
class EMPTY_API UInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UInteractionComponent();

	/** Broadcast whenever the focused interactable or its contextual prompt text changes. */
	UPROPERTY(BlueprintAssignable, Category = "Interaction|Events")
	FOnFocusedInteractableChanged OnFocusedInteractableChanged;

protected:

	/** Maximum reach distance (in cm) for interaction traces. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction", meta = (ClampMin = "50.0"))
	float InteractionDistance = 250.0f;

	/** Sphere sweep radius (in cm) for forgiving targeting of small pickups. Set to 0 for an exact line trace. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction", meta = (ClampMin = "0.0"))
	float TraceRadius = 12.0f;

	/** Collision channel used for interaction traces. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;

	/** How often (in seconds) the component updates the focused interactable. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction", meta = (ClampMin = "0.01"))
	float TraceInterval = 0.05f;

	//~ Begin UActorComponent Interface
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	//~ End UActorComponent Interface

public:

	/** Attempts to interact with the currently focused actor. Returns true if an interaction executed. */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	bool TryInteract();

	/** Immediately refreshes the focused interactable and broadcasts if anything changed. */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void RefreshFocus();

	/** Returns the currently focused interactable actor, or nullptr if none. */
	UFUNCTION(BlueprintPure, Category = "Interaction")
	AActor* GetFocusedInteractable() const { return FocusedInteractable.Get(); }

	/** Returns the cached prompt text for the currently focused interactable. */
	UFUNCTION(BlueprintPure, Category = "Interaction")
	const FText& GetFocusedPromptText() const { return CachedPromptText; }

private:

	/** Performs a view-aligned trace to find the best IInteractableInterface actor in front of the owner. */
	AActor* TraceForInteractable() const;

	/** Updates FocusedInteractable and broadcasts OnFocusedInteractableChanged when target or prompt changes. */
	void UpdateFocus(AActor* NewTarget);

	TWeakObjectPtr<AActor> FocusedInteractable;
	FText CachedPromptText;
	float TimeSinceLastTrace = 0.0f;
};
