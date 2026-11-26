// Copyright Epic Games, Inc. All Rights Reserved.

#include "EmptyPlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "EmptyCameraManager.h"
#include "EmptyCharacter.h"
#include "Blueprint/UserWidget.h"
#include "Empty.h"
#include "Interaction/InteractionComponent.h"
#include "Inventory/Components/InventoryComponent.h"
#include "Inventory/UI/InteractionPromptWidget.h"
#include "Inventory/UI/InventoryHUDWidget.h"
#include "Inventory/World/StorageContainerActor.h"
#include "Widgets/Input/SVirtualJoystick.h"

AEmptyPlayerController::AEmptyPlayerController()
{
	// set the player camera manager class
	PlayerCameraManagerClass = AEmptyCameraManager::StaticClass();
}

void AEmptyPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (IsLocalPlayerController())
	{
		if (ShouldUseTouchControls())
		{
			MobileControlsWidget = CreateWidget<UUserWidget>(this, MobileControlsWidgetClass);
			if (MobileControlsWidget)
			{
				MobileControlsWidget->AddToPlayerScreen(0);
			}
			else
			{
				UE_LOG(LogEmpty, Error, TEXT("Could not spawn mobile controls widget."));
			}
		}

		InitializeInventoryUI();
		BindPromptToPawn(GetPawn());
	}
}

void AEmptyPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (IsLocalPlayerController())
	{
		BindPromptToPawn(InPawn);
	}
}

void AEmptyPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	if (bIsInventoryOpen && ActiveStorageActor.IsValid())
	{
		const APawn* ControlledPawn = GetPawn();
		const float MaxDistSq = FMath::Square(ActiveStorageActor->GetMaxOpenDistance());
		if (!ControlledPawn || FVector::DistSquared(ControlledPawn->GetActorLocation(), ActiveStorageActor->GetActorLocation()) > MaxDistSq)
		{
			CloseInventory();
		}
	}
	else if (bIsInventoryOpen && ActiveStorageActor.IsStale())
	{
		// Storage actor was destroyed while open.
		CloseInventory();
	}
}

void AEmptyPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// only add IMCs for local player controllers
	if (IsLocalPlayerController())
	{
		// Add Input Mapping Context
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
			{
				Subsystem->AddMappingContext(CurrentContext, 0);
			}

			// only add these IMCs if we're not using mobile touch input
			if (!ShouldUseTouchControls())
			{
				for (UInputMappingContext* CurrentContext : MobileExcludedMappingContexts)
				{
					Subsystem->AddMappingContext(CurrentContext, 0);
				}
			}
		}

		if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
		{
			if (ToggleInventoryAction)
			{
				EnhancedInputComponent->BindAction(
					ToggleInventoryAction,
					ETriggerEvent::Started,
					this,
					&AEmptyPlayerController::ToggleInventory);
			}
		}
	}
}

bool AEmptyPlayerController::ShouldUseTouchControls() const
{
	// are we on a mobile platform? Should we force touch?
	return SVirtualJoystick::ShouldDisplayTouchInterface() || bForceTouchControls;
}

void AEmptyPlayerController::InitializeInventoryUI()
{
	if (InteractionPromptWidgetClass && !InteractionPromptWidget)
	{
		InteractionPromptWidget = CreateWidget<UInteractionPromptWidget>(this, InteractionPromptWidgetClass);
		if (InteractionPromptWidget)
		{
			InteractionPromptWidget->AddToViewport(5);
		}
	}

	if (InventoryHUDWidgetClass && !InventoryHUDWidget)
	{
		InventoryHUDWidget = CreateWidget<UInventoryHUDWidget>(this, InventoryHUDWidgetClass);
		if (InventoryHUDWidget)
		{
			InventoryHUDWidget->AddToViewport(10);
			InventoryHUDWidget->HideInventory();
			InventoryHUDWidget->OnCloseRequested.AddDynamic(this, &AEmptyPlayerController::HandleInventoryCloseRequested);
			InventoryHUDWidget->OnItemDropRequested.AddDynamic(this, &AEmptyPlayerController::HandleItemDropRequested);
		}
	}
}

void AEmptyPlayerController::BindPromptToPawn(APawn* InPawn)
{
	if (!InteractionPromptWidget)
	{
		return;
	}

	UInteractionComponent* InteractionComp = InPawn ? InPawn->FindComponentByClass<UInteractionComponent>() : nullptr;
	if (InteractionComp)
	{
		InteractionPromptWidget->BindToInteractionComponent(InteractionComp);
	}
	else
	{
		InteractionPromptWidget->UnbindInteractionComponent();
	}
}

void AEmptyPlayerController::ToggleInventory()
{
	if (bIsInventoryOpen)
	{
		CloseInventory();
	}
	else
	{
		OpenPlayerInventory();
	}
}

void AEmptyPlayerController::OpenPlayerInventory()
{
	UInventoryComponent* PlayerInventory = GetPlayerInventoryComponent();
	if (!PlayerInventory)
	{
		return;
	}

	if (!InventoryHUDWidget)
	{
		InitializeInventoryUI();
	}

	if (!InventoryHUDWidget)
	{
		UE_LOG(LogEmpty, Warning, TEXT("AEmptyPlayerController::OpenPlayerInventory - InventoryHUDWidgetClass is not assigned."));
		return;
	}

	ActiveStorageActor.Reset();
	InventoryHUDWidget->ShowPlayerInventory(PlayerInventory);
	bIsInventoryOpen = true;
	SetInventoryInputMode(true);
}

void AEmptyPlayerController::OpenStorage(AStorageContainerActor* StorageActor)
{
	if (!StorageActor || !StorageActor->GetInventoryComponent())
	{
		return;
	}

	UInventoryComponent* PlayerInventory = GetPlayerInventoryComponent();
	if (!PlayerInventory)
	{
		return;
	}

	if (!InventoryHUDWidget)
	{
		InitializeInventoryUI();
	}

	if (!InventoryHUDWidget)
	{
		UE_LOG(LogEmpty, Warning, TEXT("AEmptyPlayerController::OpenStorage - InventoryHUDWidgetClass is not assigned."));
		return;
	}

	ActiveStorageActor = StorageActor;
	InventoryHUDWidget->ShowStorageInventory(PlayerInventory, StorageActor->GetInventoryComponent());
	bIsInventoryOpen = true;
	SetInventoryInputMode(true);
}

void AEmptyPlayerController::CloseInventory()
{
	if (!bIsInventoryOpen)
	{
		return;
	}

	ActiveStorageActor.Reset();
	bIsInventoryOpen = false;

	if (InventoryHUDWidget)
	{
		InventoryHUDWidget->HideInventory();
	}

	SetInventoryInputMode(false);
}

void AEmptyPlayerController::SetInventoryInputMode(bool bOpen)
{
	if (InteractionPromptWidget)
	{
		InteractionPromptWidget->SetPromptSuppressed(bOpen);
	}

	if (bOpen)
	{
		FInputModeGameAndUI InputMode;
		if (InventoryHUDWidget)
		{
			InputMode.SetWidgetToFocus(InventoryHUDWidget->TakeWidget());
		}
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		InputMode.SetHideCursorDuringCapture(false);

		SetInputMode(InputMode);
		SetShowMouseCursor(true);
		SetIgnoreLookInput(true);
		SetIgnoreMoveInput(true);
	}
	else
	{
		FInputModeGameOnly InputMode;
		SetInputMode(InputMode);
		SetShowMouseCursor(false);
		ResetIgnoreLookInput();
		ResetIgnoreMoveInput();
	}
}

UInventoryComponent* AEmptyPlayerController::GetPlayerInventoryComponent() const
{
	const APawn* ControlledPawn = GetPawn();
	return ControlledPawn ? ControlledPawn->FindComponentByClass<UInventoryComponent>() : nullptr;
}

void AEmptyPlayerController::HandleInventoryCloseRequested()
{
	CloseInventory();
}

void AEmptyPlayerController::HandleItemDropRequested(UInventoryComponent* SourceInventory, int32 SlotIndex, int32 Quantity)
{
	if (AEmptyCharacter* EmptyCharacter = Cast<AEmptyCharacter>(GetPawn()))
	{
		EmptyCharacter->DropItemFromInventory(SourceInventory, SlotIndex, Quantity);
	}
}
