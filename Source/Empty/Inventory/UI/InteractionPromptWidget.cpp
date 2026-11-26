#include "Inventory/UI/InteractionPromptWidget.h"
#include "Components/TextBlock.h"
#include "Interaction/InteractionComponent.h"

UInteractionPromptWidget::UInteractionPromptWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InteractKeyLabel = NSLOCTEXT("Interaction", "DefaultInteractKey", "E");
}

void UInteractionPromptWidget::NativeConstruct()
{
	Super::NativeConstruct();
	UpdatePromptDisplay(CurrentPromptText);
}

void UInteractionPromptWidget::NativeDestruct()
{
	UnbindInteractionComponent();
	Super::NativeDestruct();
}

void UInteractionPromptWidget::BindToInteractionComponent(UInteractionComponent* InInteractionComponent)
{
	UnbindInteractionComponent();

	InteractionComponent = InInteractionComponent;
	if (!InteractionComponent.IsValid())
	{
		return;
	}

	InteractionComponent->OnFocusedInteractableChanged.AddDynamic(
		this,
		&UInteractionPromptWidget::HandleFocusedInteractableChanged);

	UpdatePromptDisplay(InteractionComponent->GetFocusedPromptText());
}

void UInteractionPromptWidget::UnbindInteractionComponent()
{
	if (InteractionComponent.IsValid())
	{
		InteractionComponent->OnFocusedInteractableChanged.RemoveDynamic(
			this,
			&UInteractionPromptWidget::HandleFocusedInteractableChanged);
	}

	InteractionComponent.Reset();
	UpdatePromptDisplay(FText::GetEmpty());
}

void UInteractionPromptWidget::SetPromptSuppressed(bool bInSuppressed)
{
	bIsSuppressed = bInSuppressed;
	UpdatePromptDisplay(CurrentPromptText);
}

void UInteractionPromptWidget::HandleFocusedInteractableChanged(AActor* NewInteractable, const FText& NewPromptText)
{
	UpdatePromptDisplay(NewInteractable ? NewPromptText : FText::GetEmpty());
}

void UInteractionPromptWidget::UpdatePromptDisplay(const FText& ActionPrompt)
{
	CurrentPromptText = ActionPrompt;

	if (bIsSuppressed || CurrentPromptText.IsEmpty())
	{
		SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	if (PromptText)
	{
		PromptText->SetText(FText::Format(
			NSLOCTEXT("Interaction", "PromptFormat", "[{0}] {1}"),
			InteractKeyLabel,
			CurrentPromptText));
	}

	SetVisibility(ESlateVisibility::HitTestInvisible);
}
