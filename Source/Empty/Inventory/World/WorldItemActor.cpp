#include "Inventory/World/WorldItemActor.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Inventory/Components/InventoryComponent.h"
#include "Empty.h"

AWorldItemActor::AWorldItemActor()
{
	PrimaryActorTick.bCanEverTick = false;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	SetRootComponent(MeshComponent);

	MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	MeshComponent->SetCollisionObjectType(ECC_PhysicsBody);
	MeshComponent->SetCollisionResponseToAllChannels(ECR_Block);
	MeshComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	MeshComponent->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	MeshComponent->SetGenerateOverlapEvents(false);
}

void AWorldItemActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RefreshVisuals();
}

void AWorldItemActor::BeginPlay()
{
	Super::BeginPlay();

	// Ensure designer-placed pickups receive default instance fragments if none were manually set.
	if (Item.IsValid() && Item.InstanceFragments.IsEmpty() && Item.Definition)
	{
		Item.InstanceFragments = Item.Definition->DefaultInstanceFragments;
	}

	RefreshVisuals();

	if (bSimulatePhysicsOnBeginPlay && MeshComponent->GetStaticMesh() != nullptr)
	{
		MeshComponent->SetSimulatePhysics(true);
	}
}

void AWorldItemActor::InitializeItem(const FInventoryItem& InItem, bool bEnablePhysics)
{
	Item = InItem;
	bSimulatePhysicsOnBeginPlay = bEnablePhysics;
	RefreshVisuals();

	if (MeshComponent->GetStaticMesh() != nullptr)
	{
		MeshComponent->SetSimulatePhysics(bEnablePhysics);
	}
}

void AWorldItemActor::RefreshVisuals()
{
	if (Item.Definition && Item.Definition->WorldMesh)
	{
		MeshComponent->SetStaticMesh(Item.Definition->WorldMesh);
	}
}

AWorldItemActor* AWorldItemActor::SpawnWorldItem(
	UObject* WorldContextObject,
	TSubclassOf<AWorldItemActor> ActorClass,
	const FInventoryItem& InItem,
	const FTransform& SpawnTransform,
	const FVector& LaunchVelocity)
{
	if (!WorldContextObject || !InItem.IsValid())
	{
		return nullptr;
	}

	UWorld* World = WorldContextObject->GetWorld();
	if (!World)
	{
		return nullptr;
	}

	UClass* SpawnClass = ActorClass ? ActorClass.Get() : AWorldItemActor::StaticClass();
	AWorldItemActor* SpawnedActor = World->SpawnActorDeferred<AWorldItemActor>(
		SpawnClass,
		SpawnTransform,
		nullptr,
		nullptr,
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);

	if (!SpawnedActor)
	{
		return nullptr;
	}

	SpawnedActor->InitializeItem(InItem, true);
	SpawnedActor->FinishSpawning(SpawnTransform);

	if (!LaunchVelocity.IsNearlyZero() && SpawnedActor->MeshComponent && SpawnedActor->MeshComponent->IsSimulatingPhysics())
	{
		SpawnedActor->MeshComponent->AddImpulse(LaunchVelocity, NAME_None, true);
	}

	return SpawnedActor;
}

FText AWorldItemActor::GetInteractionPrompt_Implementation(AActor* Interactor) const
{
	if (!Item.IsValid())
	{
		return FText::GetEmpty();
	}

	const FText ItemName = Item.Definition->GetEffectiveDisplayName();

	if (Interactor)
	{
		if (const UInventoryComponent* TargetInventory = Interactor->FindComponentByClass<UInventoryComponent>())
		{
			if (!TargetInventory->CanAcceptItem(Item))
			{
				return FText::Format(
					NSLOCTEXT("Inventory", "PromptInventoryFull", "Inventory Full ({0})"),
					ItemName);
			}
		}
	}

	if (Item.Quantity > 1)
	{
		return FText::Format(
			NSLOCTEXT("Inventory", "PromptPickUpMultiple", "Pick up {0} (x{1})"),
			ItemName,
			FText::AsNumber(Item.Quantity));
	}

	return FText::Format(
		NSLOCTEXT("Inventory", "PromptPickUpSingle", "Pick up {0}"),
		ItemName);
}

bool AWorldItemActor::CanInteract_Implementation(AActor* Interactor) const
{
	if (!Item.IsValid() || !Interactor)
	{
		return false;
	}

	const UInventoryComponent* TargetInventory = Interactor->FindComponentByClass<UInventoryComponent>();
	return TargetInventory != nullptr && TargetInventory->CanAcceptItem(Item);
}

void AWorldItemActor::Interact_Implementation(AActor* Interactor)
{
	if (!Item.IsValid() || !Interactor)
	{
		return;
	}

	UInventoryComponent* TargetInventory = Interactor->FindComponentByClass<UInventoryComponent>();
	if (!TargetInventory)
	{
		return;
	}

	const FText ItemName = Item.Definition->GetEffectiveDisplayName();
	const FInventoryAddResult Result = TargetInventory->TryAddItem(Item);

	if (!Result.WasAnythingAdded())
	{
		UE_LOG(LogEmpty, Log, TEXT("Could not pick up '%s' - inventory is full."), *ItemName.ToString());
		return;
	}

	UE_LOG(LogEmpty, Log, TEXT("Picked up %d x '%s' (remainder: %d)."),
		Result.AddedQuantity,
		*ItemName.ToString(),
		Result.RemainderQuantity);

	if (Result.WasFullyAdded())
	{
		Destroy();
	}
	else
	{
		Item.Quantity = Result.RemainderQuantity;
	}
}
