#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InteractionPromptWidget.generated.h"

class UInteractionComponent;
class UTextBlock;

/**
 * Minimal HUD widget displaying the contextual prompt for the currently targeted interactable
 * (e.g., "[E] Pick up Health Potion (x3)" or "[E] Open Storage Chest").
 */
UCLASS(Abstract)
class EMPTY_API UInteractionPromptWidget : public UUserWidget
{
	GENERATED_BODY()

protected:

	/** Text label displaying the key hint and action prompt. */
	UPROPERTY(BlueprintReadOnly, Category = "Interaction|UI", meta = (BindWidget))
	TObjectPtr<UTextBlock> PromptText;

	/** Key label shown in brackets before the prompt text (defaults to "E"). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction|UI")
	FText InteractKeyLabel;

	//~ Begin UUserWidget Interface
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	//~ End UUserWidget Interface

public:

	UInteractionPromptWidget(const FObjectInitializer& ObjectInitializer);

	/** Subscribes to focus changes on InInteractionComponent and syncs initial visibility. */
	UFUNCTION(BlueprintCallable, Category = "Interaction|UI")
	void BindToInteractionComponent(UInteractionComponent* InInteractionComponent);

	/** Unsubscribes from the current interaction component and hides the prompt. */
	UFUNCTION(BlueprintCallable, Category = "Interaction|UI")
	void UnbindInteractionComponent();

	/** Suppresses the prompt (e.g., while the inventory screen is open) without losing the component binding. */
	UFUNCTION(BlueprintCallable, Category = "Interaction|UI")
	void SetPromptSuppressed(bool bInSuppressed);

private:

	UFUNCTION()
	void HandleFocusedInteractableChanged(AActor* NewInteractable, const FText& NewPromptText);

	void UpdatePromptDisplay(const FText& ActionPrompt);

	TWeakObjectPtr<UInteractionComponent> InteractionComponent;
	FText CurrentPromptText;
	bool bIsSuppressed = false;
};
