#include "Interaction/InteractionComponent.h"
#include "Interaction/InteractableInterface.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"

UInteractionComponent::UInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UInteractionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	TimeSinceLastTrace += DeltaTime;
	if (TimeSinceLastTrace >= TraceInterval)
	{
		TimeSinceLastTrace = 0.0f;
		RefreshFocus();
	}
}

bool UInteractionComponent::TryInteract()
{
	AActor* Target = FocusedInteractable.Get();
	AActor* OwnerActor = GetOwner();
	if (!Target || !OwnerActor)
	{
		return false;
	}

	if (!Target->Implements<UInteractableInterface>())
	{
		return false;
	}

	if (!IInteractableInterface::Execute_CanInteract(Target, OwnerActor))
	{
		return false;
	}

	IInteractableInterface::Execute_Interact(Target, OwnerActor);

	// Re-evaluate immediately so HUD prompts reflect destroyed pickups or updated stack counts.
	RefreshFocus();
	return true;
}

void UInteractionComponent::RefreshFocus()
{
	UpdateFocus(TraceForInteractable());
}

AActor* UInteractionComponent::TraceForInteractable() const
{
	AActor* OwnerActor = GetOwner();
	UWorld* World = GetWorld();
	if (!OwnerActor || !World)
	{
		return nullptr;
	}

	FVector ViewLocation;
	FRotator ViewRotation;
	if (const APawn* OwnerPawn = Cast<APawn>(OwnerActor))
	{
		if (const AController* Controller = OwnerPawn->GetController())
		{
			Controller->GetPlayerViewPoint(ViewLocation, ViewRotation);
		}
		else
		{
			OwnerPawn->GetActorEyesViewPoint(ViewLocation, ViewRotation);
		}
	}
	else
	{
		OwnerActor->GetActorEyesViewPoint(ViewLocation, ViewRotation);
	}

	const FVector TraceEnd = ViewLocation + (ViewRotation.Vector() * InteractionDistance);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(InteractionTrace), false, OwnerActor);

	FHitResult HitResult;
	bool bHit = false;
	if (TraceRadius > KINDA_SMALL_NUMBER)
	{
		bHit = World->SweepSingleByChannel(
			HitResult,
			ViewLocation,
			TraceEnd,
			FQuat::Identity,
			TraceChannel,
			FCollisionShape::MakeSphere(TraceRadius),
			QueryParams);
	}
	else
	{
		bHit = World->LineTraceSingleByChannel(
			HitResult,
			ViewLocation,
			TraceEnd,
			TraceChannel,
			QueryParams);
	}

	AActor* HitActor = bHit ? HitResult.GetActor() : nullptr;
	if (HitActor && HitActor->Implements<UInteractableInterface>())
	{
		return HitActor;
	}

	return nullptr;
}

void UInteractionComponent::UpdateFocus(AActor* NewTarget)
{
	FText NewPrompt;
	if (NewTarget && NewTarget->Implements<UInteractableInterface>())
	{
		NewPrompt = IInteractableInterface::Execute_GetInteractionPrompt(NewTarget, GetOwner());
	}

	const bool bTargetChanged = (FocusedInteractable.Get() != NewTarget);
	const bool bPromptChanged = !CachedPromptText.EqualTo(NewPrompt);

	if (bTargetChanged || bPromptChanged)
	{
		FocusedInteractable = NewTarget;
		CachedPromptText = NewPrompt;
		OnFocusedInteractableChanged.Broadcast(NewTarget, NewPrompt);
	}
}
