#include "Inventory/World/StorageContainerActor.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "Inventory/Components/InventoryComponent.h"
#include "EmptyPlayerController.h"

AStorageContainerActor::AStorageContainerActor()
{
	PrimaryActorTick.bCanEverTick = false;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	SetRootComponent(MeshComponent);

	InventoryComponent = CreateDefaultSubobject<UInventoryComponent>(TEXT("InventoryComponent"));

	ContainerName = NSLOCTEXT("Inventory", "DefaultStorageName", "Storage Chest");
	InventoryComponent->SetInventoryName(ContainerName);
}

void AStorageContainerActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (InventoryComponent && !ContainerName.IsEmpty())
	{
		InventoryComponent->SetInventoryName(ContainerName);
	}
}

void AStorageContainerActor::BeginPlay()
{
	Super::BeginPlay();

	if (InventoryComponent && !ContainerName.IsEmpty())
	{
		InventoryComponent->SetInventoryName(ContainerName);
	}
}

FText AStorageContainerActor::GetInteractionPrompt_Implementation(AActor* Interactor) const
{
	return FText::Format(
		NSLOCTEXT("Inventory", "PromptOpenStorage", "Open {0}"),
		ContainerName);
}

bool AStorageContainerActor::CanInteract_Implementation(AActor* Interactor) const
{
	return Interactor != nullptr && InventoryComponent != nullptr;
}

void AStorageContainerActor::Interact_Implementation(AActor* Interactor)
{
	if (!Interactor || !InventoryComponent)
	{
		return;
	}

	OnStorageInteracted.Broadcast(Interactor);

	if (const APawn* InteractorPawn = Cast<APawn>(Interactor))
	{
		if (AEmptyPlayerController* PlayerController = Cast<AEmptyPlayerController>(InteractorPawn->GetController()))
		{
			PlayerController->OpenStorage(this);
		}
	}
}
