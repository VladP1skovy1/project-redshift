// Copyright Epic Games, Inc. All Rights Reserved.


#include "ShooterCharacter.h"
#include "ShooterWeapon.h"
#include "EnhancedInputComponent.h"
#include "Components/InputComponent.h"
#include "Components/PawnNoiseEmitterComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "Camera/CameraComponent.h"
#include "TimerManager.h"
#include "ShooterGameMode.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Engine/Engine.h"
#include "Components/LightComponent.h"
#include "UObject/UObjectIterator.h"
#include "HAL/IConsoleManager.h"

// type "Shooter.ADSDebug 1" in the console to see aim down sights debug info
static TAutoConsoleVariable<int32> CVarShooterADSDebug(TEXT("Shooter.ADSDebug"), 0, TEXT("Show aim down sights / reload offset debug info"));

AShooterCharacter::AShooterCharacter()
{
	// create the noise emitter component
	PawnNoiseEmitter = CreateDefaultSubobject<UPawnNoiseEmitterComponent>(TEXT("Pawn Noise Emitter"));

	// configure movement
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 600.0f, 0.0f);

	// tick is used for the aim down sights zoom
	PrimaryActorTick.bCanEverTick = true;
}

void AShooterCharacter::BeginPlay()
{
	Super::BeginPlay();

	// reset HP to max
	CurrentHP = MaxHP;

	// cache the non-aiming camera FOV and walk speed
	DefaultFieldOfView = GetFirstPersonCameraComponent()->FieldOfView;
	DefaultMaxWalkSpeed = GetCharacterMovement()->MaxWalkSpeed;

	// cache the default arms and camera placement for the procedural weapon offsets
	DefaultArmsRelativeLocation = GetFirstPersonMesh()->GetRelativeLocation();
	DefaultArmsRelativeRotation = GetFirstPersonMesh()->GetRelativeRotation();
	DefaultCameraRelativeLocation = GetFirstPersonCameraComponent()->GetRelativeLocation();

	// keep muzzle flashes off the first person arms
	SetupFirstPersonLightingChannels();

	// apply the weapon offsets after the arms animation is evaluated each frame
	ArmsBonesFinalizedHandle = GetFirstPersonMesh()->RegisterOnBoneTransformsFinalizedDelegate(
		FOnBoneTransformsFinalizedMultiCast::FDelegate::CreateUObject(this, &AShooterCharacter::OnArmsBoneTransformsFinalized));

	// update the HUD
	OnDamaged.Broadcast(1.0f);
}

void AShooterCharacter::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	// clear the respawn timer
	GetWorld()->GetTimerManager().ClearTimer(RespawnTimer);

	// stop listening to the arms animation
	if (ArmsBonesFinalizedHandle.IsValid() && GetFirstPersonMesh())
	{
		GetFirstPersonMesh()->UnregisterOnBoneTransformsFinalizedDelegate(ArmsBonesFinalizedHandle);
		ArmsBonesFinalizedHandle.Reset();
	}
}

void AShooterCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// we're aiming if the button is held, the weapon supports it and we're not busy reloading
	const bool bShouldAim = bWantsToAimDownSights && !IsDead()
		&& IsValid(CurrentWeapon) && CurrentWeapon->CanAimDownSights() && !CurrentWeapon->IsReloading();

	// update the walk speed when the state changes
	if (bShouldAim != bIsAimingDownSights)
	{
		bIsAimingDownSights = bShouldAim;
		GetCharacterMovement()->MaxWalkSpeed = DefaultMaxWalkSpeed * (bIsAimingDownSights ? AimDownSightsWalkSpeedMultiplier : 1.0f);

		// let the HUD know (hide / show the crosshair)
		OnAimDownSightsChanged.Broadcast(bIsAimingDownSights);
	}

	// smoothly zoom the camera
	UCameraComponent* Camera = GetFirstPersonCameraComponent();
	const float TargetFOV = bIsAimingDownSights ? CurrentWeapon->GetAimDownSightsFOV() : DefaultFieldOfView;

	if (!FMath::IsNearlyEqual(Camera->FieldOfView, TargetFOV, 0.01f))
	{
		Camera->SetFieldOfView(FMath::FInterpTo(Camera->FieldOfView, TargetFOV, DeltaSeconds, AimDownSightsInterpSpeed));
	}

	// blend the procedural weapon offsets
	const bool bReloading = IsValid(CurrentWeapon) && CurrentWeapon->IsReloading();

	AimDownSightsAlpha = FMath::FInterpTo(AimDownSightsAlpha, bIsAimingDownSights ? 1.0f : 0.0f, DeltaSeconds, AimDownSightsInterpSpeed);
	ReloadAlpha = FMath::FInterpTo(ReloadAlpha, bReloading ? 1.0f : 0.0f, DeltaSeconds, ReloadDipInterpSpeed);

	// camera recoil
	UpdateRecoil(DeltaSeconds);

	// visual weapon kick springs back
	if (IsValid(CurrentWeapon))
	{
		WeaponKickAlpha = FMath::FInterpTo(WeaponKickAlpha, 0.0f, DeltaSeconds, CurrentWeapon->GetWeaponKickReturnSpeed());
	}
	else
	{
		WeaponKickAlpha = 0.0f;
	}

	// the offsets themselves are applied in OnArmsBoneTransformsFinalized
}

void AShooterCharacter::SetupFirstPersonLightingChannels()
{
	// the arms only receive channel 1
	GetFirstPersonMesh()->SetLightingChannels(false, true, false);

	// every regular light in this world that lights channel 0 also lights channel 1, so the arms stay lit as before.
	// Muzzle flashes are skipped: they stay on channel 0 only
	UWorld* World = GetWorld();

	for (TObjectIterator<ULightComponent> It; It; ++It)
	{
		ULightComponent* Light = *It;

		if (!IsValid(Light) || Light->IsTemplate() || Light->GetWorld() != World)
		{
			continue;
		}

		if (Light->GetOwner() && Light->GetOwner()->IsA<AShooterWeapon>())
		{
			continue;
		}

		if (Light->LightingChannels.bChannel0 && !Light->LightingChannels.bChannel1)
		{
			Light->SetLightingChannels(true, true, Light->LightingChannels.bChannel2);
		}
	}
}

void AShooterCharacter::ApplyWeaponRecoil(const FRotator& Kick, AShooterWeapon* Weapon)
{
	PendingRecoil += Kick;
	LastRecoilTime = GetWorld()->GetTimeSeconds();

	// visual kick in the hands, scaled the same way as the camera kick (ADS multiplier)
	if (Weapon)
	{
		const float Multiplier = IsAimingDownSights() ? Weapon->GetAimDownSightsRecoilMultiplier() : 1.0f;
		WeaponKickOffset = Weapon->GetWeaponKickOffset() * Multiplier;
		WeaponKickAlpha = 1.0f;
	}
}

void AShooterCharacter::UpdateRecoil(float DeltaSeconds)
{
	if (!Controller || !IsValid(CurrentWeapon))
	{
		PendingRecoil = FRotator::ZeroRotator;
		RecoverableRecoil = FRotator::ZeroRotator;
		bRecoilPitchTracked = false;
		return;
	}

	FRotator ControlRot = GetControlRotation();
	const float CurrentPitch = FRotator::NormalizeAxis(ControlRot.Pitch);

	// if the player pulled the mouse down since last frame, that counters the recoil:
	// the camera should not "recover" past where the player put it
	if (bRecoilPitchTracked)
	{
		const float PlayerDelta = FMath::FindDeltaAngleDegrees(LastRecoilControlPitch, CurrentPitch);

		if (PlayerDelta < 0.0f && RecoverableRecoil.Pitch > 0.0f)
		{
			RecoverableRecoil.Pitch = FMath::Max(0.0f, RecoverableRecoil.Pitch + PlayerDelta);
		}
	}

	FRotator Delta = FRotator::ZeroRotator;

	if (!PendingRecoil.IsNearlyZero(0.001f))
	{
		// kick: apply part of the pending recoil this frame
		const float Alpha = FMath::Clamp(DeltaSeconds * CurrentWeapon->GetRecoilKickSpeed(), 0.0f, 1.0f);
		const FRotator Step = PendingRecoil * Alpha;

		PendingRecoil -= Step;
		Delta += Step;

		RecoverableRecoil += Step * CurrentWeapon->GetRecoilRecoveryFraction();
	}
	else if (GetWorld()->GetTimeSeconds() - LastRecoilTime > CurrentWeapon->GetRecoilRecoveryDelay() && !RecoverableRecoil.IsNearlyZero(0.001f))
	{
		// recovery: drift back towards where we were aiming
		const float MaxStep = CurrentWeapon->GetRecoilRecoverySpeed() * DeltaSeconds;

		const float PitchStep = FMath::Clamp(RecoverableRecoil.Pitch, -MaxStep, MaxStep);
		const float YawStep = FMath::Clamp(RecoverableRecoil.Yaw, -MaxStep, MaxStep);

		RecoverableRecoil.Pitch -= PitchStep;
		RecoverableRecoil.Yaw -= YawStep;

		Delta.Pitch -= PitchStep;
		Delta.Yaw -= YawStep;
	}

	if (!Delta.IsNearlyZero(0.0001f))
	{
		ControlRot.Pitch = FMath::Clamp(CurrentPitch + Delta.Pitch, -89.0f, 89.0f);
		ControlRot.Yaw += Delta.Yaw;
		Controller->SetControlRotation(ControlRot);
	}

	LastRecoilControlPitch = FRotator::NormalizeAxis(Controller->GetControlRotation().Pitch);
	bRecoilPitchTracked = true;
}

void AShooterCharacter::OnArmsBoneTransformsFinalized()
{
	UpdateFirstPersonWeaponOffset();
}

void AShooterCharacter::UpdateFirstPersonWeaponOffset()
{
	USkeletalMeshComponent* Arms = GetFirstPersonMesh();
	UCameraComponent* Camera = GetFirstPersonCameraComponent();

	// put the arms back to their default placement so we measure from a clean state
	Arms->SetRelativeLocationAndRotation(DefaultArmsRelativeLocation, DefaultArmsRelativeRotation);

	// where the camera sits without any offset (it's attached to the arms' head socket)
	const FTransform CameraParentTM = Arms->GetSocketTransform(Camera->GetAttachSocketName());
	const FVector CameraBase = CameraParentTM.TransformPosition(DefaultCameraRelativeLocation);

	// view axes (the camera uses the control rotation)
	const FRotator ViewRotation = Controller ? GetControlRotation() : GetActorRotation();
	const FRotationMatrix ViewMatrix(ViewRotation);
	const FVector Forward = ViewMatrix.GetUnitAxis(EAxis::X);
	const FVector Right = ViewMatrix.GetUnitAxis(EAxis::Y);
	const FVector Up = ViewMatrix.GetUnitAxis(EAxis::Z);

	FVector Offset = FVector::ZeroVector;
	bool bRotated = false;


	if (IsValid(CurrentWeapon))
	{
		FTransform SightTM;

		if (AimDownSightsAlpha > KINDA_SMALL_NUMBER && CurrentWeapon->GetSightTransform(SightTM))
		{
			const FVector SightLocation = SightTM.GetLocation();

			// aim down sights, step 1 (optional, off by default): small rotation so the line of sight points along the view
			if (CurrentWeapon->ShouldAlignSightRotation())
			{
				FQuat FullAlign = FQuat::Identity;
				FVector FrontSightLocation;

				// point the line of sight (rear -> front sight) along the view, but only by a few degrees:
				// the arms mesh is the whole body, big rotations swing the shoulders/head into view
				if (CurrentWeapon->GetFrontSightLocation(FrontSightLocation))
				{
					const FVector SightLine = (FrontSightLocation - SightLocation).GetSafeNormal();

					if (!SightLine.IsNearlyZero())
					{
						FullAlign = FQuat::FindBetweenNormals(SightLine, Forward);

						const float MaxAngle = FMath::DegreesToRadians(CurrentWeapon->GetMaxSightAlignAngle());
						const float Angle = FullAlign.GetAngle();

						if (Angle > MaxAngle && Angle > KINDA_SMALL_NUMBER)
						{
							FullAlign = FQuat::Slerp(FQuat::Identity, FullAlign, MaxAngle / Angle);
						}
					}
				}

				FullAlign.Normalize();

				const FQuat Align = FQuat::Slerp(FQuat::Identity, FullAlign, AimDownSightsAlpha);

				const FVector NewArmsLocation = SightLocation + Align.RotateVector(Arms->GetComponentLocation() - SightLocation);
				const FQuat NewArmsRotation = Align * Arms->GetComponentQuat();

				Arms->SetWorldLocationAndRotation(NewArmsLocation, NewArmsRotation);
				bRotated = true;
			}

			// step 2: move the arms so the sight lands on the view axis (= screen center)
			const FVector ToSight = SightLocation - CameraBase;

			FVector Correction = -(FVector::DotProduct(ToSight, Right) * Right + FVector::DotProduct(ToSight, Up) * Up);

			// optionally bring the weapon closer to the eye
			const float EyeDistance = CurrentWeapon->GetAimDownSightsEyeDistance();

			if (EyeDistance > 0.0f)
			{
				Correction -= (FVector::DotProduct(ToSight, Forward) - EyeDistance) * Forward;
			}

			Offset += Correction * AimDownSightsAlpha;
		}

		// recoil: visual kick back/up in the hands
		if (WeaponKickAlpha > KINDA_SMALL_NUMBER)
		{
			Offset += (Forward * WeaponKickOffset.X + Right * WeaponKickOffset.Y + Up * WeaponKickOffset.Z) * WeaponKickAlpha;
		}

		// reload: dip the weapon down
		if (ReloadAlpha > KINDA_SMALL_NUMBER)
		{
			const FVector& Dip = CurrentWeapon->GetReloadDipOffset();
			Offset += (Forward * Dip.X + Right * Dip.Y + Up * Dip.Z) * ReloadAlpha;
		}
	}

	if (CVarShooterADSDebug.GetValueOnGameThread() > 0 && GEngine)
	{
		FVector DebugSight = FVector::ZeroVector;
		const bool bHasWeapon = IsValid(CurrentWeapon);
		const bool bHasSightSocket = bHasWeapon && CurrentWeapon->GetFirstPersonMesh()->DoesSocketExist(FName("Sight"));
		const bool bHasAnyPoint = bHasWeapon && CurrentWeapon->GetSightLocation(DebugSight);
		const FString MeshName = bHasWeapon ? GetNameSafe(CurrentWeapon->GetFirstPersonMesh()->GetSkeletalMeshAsset()) : TEXT("none");

		GEngine->AddOnScreenDebugMessage(9001, 0.0f, FColor::Yellow, FString::Printf(
			TEXT("ADS alpha %.2f | reload alpha %.2f | mesh %s | Sight socket %s | aim point %s | offset %s"),
			AimDownSightsAlpha, ReloadAlpha, *MeshName,
			bHasSightSocket ? TEXT("FOUND") : TEXT("MISSING"),
			bHasAnyPoint ? TEXT("ok") : TEXT("none"),
			*Offset.ToString()));
	}

	if (!bRotated && Offset.IsNearlyZero(0.01f))
	{
		// nothing to do, make sure the camera is exactly at its default spot
		Camera->SetRelativeLocation(DefaultCameraRelativeLocation);
		return;
	}

	// move the arms (and the weapon attached to them)
	if (!Offset.IsNearlyZero(0.01f))
	{
		Arms->AddWorldOffset(Offset);
	}

	// the camera is attached to the arms, so keep it where it would have been
	Camera->SetWorldLocation(CameraBase);
}

void AShooterCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// base class handles move, aim and jump inputs
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Firing
		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Started, this, &AShooterCharacter::DoStartFiring);
		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Completed, this, &AShooterCharacter::DoStopFiring);

		// Switch weapon
		EnhancedInputComponent->BindAction(SwitchWeaponAction, ETriggerEvent::Triggered, this, &AShooterCharacter::DoSwitchWeapon);

		// Aim down sights (hold)
		EnhancedInputComponent->BindAction(AimDownSightsAction, ETriggerEvent::Started, this, &AShooterCharacter::DoStartAimDownSights);
		EnhancedInputComponent->BindAction(AimDownSightsAction, ETriggerEvent::Completed, this, &AShooterCharacter::DoStopAimDownSights);

		// Reload
		EnhancedInputComponent->BindAction(ReloadAction, ETriggerEvent::Started, this, &AShooterCharacter::DoReload);
	}

}

float AShooterCharacter::TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	// ignore if already dead
	if (CurrentHP <= 0.0f)
	{
		return 0.0f;
	}

	// Reduce HP
	CurrentHP -= Damage;

	// Have we depleted HP?
	if (CurrentHP <= 0.0f)
	{
		Die();
	}

	// update the HUD
	OnDamaged.Broadcast(FMath::Max(0.0f, CurrentHP / MaxHP));

	return Damage;
}

void AShooterCharacter::DoAim(float Yaw, float Pitch)
{
	// only route inputs if the character is not dead
	if (!IsDead())
	{
		Super::DoAim(Yaw, Pitch);
	}
}

void AShooterCharacter::DoMove(float Right, float Forward)
{
	// only route inputs if the character is not dead
	if (!IsDead())
	{
		Super::DoMove(Right, Forward);
	}
}

void AShooterCharacter::DoJumpStart()
{
	// only route inputs if the character is not dead
	if (!IsDead())
	{
		Super::DoJumpStart();
	}
}

void AShooterCharacter::DoJumpEnd()
{
	// only route inputs if the character is not dead
	if (!IsDead())
	{
		Super::DoJumpEnd();
	}
}

void AShooterCharacter::DoStartFiring()
{
	// fire the current weapon
	if (CurrentWeapon && !IsDead())
	{
		CurrentWeapon->StartFiring();
	}
}

void AShooterCharacter::DoStopFiring()
{
	// stop firing the current weapon
	if (CurrentWeapon && !IsDead())
	{
		CurrentWeapon->StopFiring();
	}
}

void AShooterCharacter::DoSwitchWeapon()
{
	// ensure we have at least two weapons two switch between
	if (OwnedWeapons.Num() > 1 && !IsDead())
	{
		// deactivate the old weapon
		CurrentWeapon->DeactivateWeapon();

		// find the index of the current weapon in the owned list
		int32 WeaponIndex = OwnedWeapons.Find(CurrentWeapon);

		// is this the last weapon?
		if (WeaponIndex == OwnedWeapons.Num() - 1)
		{
			// loop back to the beginning of the array
			WeaponIndex = 0;
		}
		else {
			// select the next weapon index
			++WeaponIndex;
		}

		// set the new weapon as current
		CurrentWeapon = OwnedWeapons[WeaponIndex];

		// activate the new weapon
		CurrentWeapon->ActivateWeapon(PlayerTag);
	}
}

void AShooterCharacter::DoStartAimDownSights()
{
	bWantsToAimDownSights = true;
}

void AShooterCharacter::DoStopAimDownSights()
{
	bWantsToAimDownSights = false;
}

void AShooterCharacter::DoReload()
{
	if (CurrentWeapon && !IsDead())
	{
		CurrentWeapon->StartReload();
	}
}

void AShooterCharacter::OnWeaponReloadStarted(AShooterWeapon* Weapon, UAnimMontage* Montage, float Duration)
{
	if (!Montage)
	{
		return;
	}

	// stretch the montage so it lasts exactly as long as the reload
	const float PlayRate = Duration > 0.0f ? Montage->GetPlayLength() / Duration : 1.0f;

	if (UAnimInstance* FPAnim = GetFirstPersonMesh()->GetAnimInstance())
	{
		FPAnim->Montage_Play(Montage, PlayRate);
	}

	if (UAnimInstance* TPAnim = GetMesh()->GetAnimInstance())
	{
		TPAnim->Montage_Play(Montage, PlayRate);
	}
}

void AShooterCharacter::OnWeaponReloadCancelled(AShooterWeapon* Weapon, UAnimMontage* Montage)
{
	if (!Montage)
	{
		return;
	}

	if (UAnimInstance* FPAnim = GetFirstPersonMesh()->GetAnimInstance())
	{
		FPAnim->Montage_Stop(0.2f, Montage);
	}

	if (UAnimInstance* TPAnim = GetMesh()->GetAnimInstance())
	{
		TPAnim->Montage_Stop(0.2f, Montage);
	}
}

void AShooterCharacter::AttachWeaponMeshes(AShooterWeapon* Weapon)
{
	const FAttachmentTransformRules AttachmentRule(EAttachmentRule::SnapToTarget, false);

	// attach the weapon actor
	Weapon->AttachToActor(this, AttachmentRule);

	// attach the weapon meshes
	Weapon->GetFirstPersonMesh()->AttachToComponent(GetFirstPersonMesh(), AttachmentRule, FirstPersonWeaponSocket);
	Weapon->GetThirdPersonMesh()->AttachToComponent(GetMesh(), AttachmentRule, FirstPersonWeaponSocket);
	
}

void AShooterCharacter::PlayFiringMontage(UAnimMontage* Montage)
{
	// stub
}

void AShooterCharacter::AddWeaponRecoil(float Recoil)
{
	// apply the recoil as pitch input
	AddControllerPitchInput(Recoil);
}

void AShooterCharacter::UpdateWeaponHUD(int32 CurrentAmmo, int32 MagazineSize)
{
	OnBulletCountUpdated.Broadcast(MagazineSize, CurrentAmmo);
}

FVector AShooterCharacter::GetWeaponTargetLocation()
{
	// trace ahead from the camera viewpoint
	FHitResult OutHit;

	const FVector Start = GetFirstPersonCameraComponent()->GetComponentLocation();
	const FVector End = Start + (GetFirstPersonCameraComponent()->GetForwardVector() * MaxAimDistance);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	GetWorld()->LineTraceSingleByChannel(OutHit, Start, End, ECC_Visibility, QueryParams);

	// return either the impact point or the trace end
	return OutHit.bBlockingHit ? OutHit.ImpactPoint : OutHit.TraceEnd;
}

void AShooterCharacter::AddWeaponClass(const TSubclassOf<AShooterWeapon>& WeaponClass)
{
	// do we already own this weapon?
	AShooterWeapon* OwnedWeapon = FindWeaponOfType(WeaponClass);

	if (!OwnedWeapon)
	{
		// spawn the new weapon
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.Instigator = this;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SpawnParams.TransformScaleMethod = ESpawnActorScaleMethod::MultiplyWithRoot;

		AShooterWeapon* AddedWeapon = GetWorld()->SpawnActor<AShooterWeapon>(WeaponClass, GetActorTransform(), SpawnParams);

		if (AddedWeapon)
		{
			// add the weapon to the owned list
			OwnedWeapons.Add(AddedWeapon);

			// if we have an existing weapon, deactivate it
			if (CurrentWeapon)
			{
				CurrentWeapon->DeactivateWeapon();
			}

			// switch to the new weapon
			CurrentWeapon = AddedWeapon;
			CurrentWeapon->ActivateWeapon(PlayerTag);
		}
	}
}

void AShooterCharacter::OnWeaponActivated(AShooterWeapon* Weapon)
{
	// update the bullet counter
	OnBulletCountUpdated.Broadcast(Weapon->GetMagazineSize(), Weapon->GetBulletCount());

	// set the character mesh AnimInstances
	GetFirstPersonMesh()->SetAnimInstanceClass(Weapon->GetFirstPersonAnimInstanceClass());
	GetMesh()->SetAnimInstanceClass(Weapon->GetThirdPersonAnimInstanceClass());
}

void AShooterCharacter::OnWeaponDeactivated(AShooterWeapon* Weapon)
{
	// unused
}

void AShooterCharacter::OnSemiWeaponRefire()
{
	// unused
}

AShooterWeapon* AShooterCharacter::FindWeaponOfType(TSubclassOf<AShooterWeapon> WeaponClass) const
{
	// check each owned weapon
	for (AShooterWeapon* Weapon : OwnedWeapons)
	{
		if (Weapon->IsA(WeaponClass))
		{
			return Weapon;
		}
	}

	// weapon not found
	return nullptr;

}

void AShooterCharacter::Die()
{
	// deactivate the weapon
	if (IsValid(CurrentWeapon))
	{
		CurrentWeapon->DeactivateWeapon();
	}

	// increment the team score
	if (AShooterGameMode* GM = Cast<AShooterGameMode>(GetWorld()->GetAuthGameMode()))
	{
		GM->IncrementTeamScore(TeamByte);
	}

	// grant the death tag to the character
	Tags.Add(DeathTag);

	// stop aiming
	bWantsToAimDownSights = false;
		
	// stop character movement
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();

	// disable collision
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// disable controls
	DisableInput(nullptr);

	// reset the bullet counter UI
	OnBulletCountUpdated.Broadcast(0, 0);

	// call the BP handler
	BP_OnDeath();

	// schedule character respawn
	GetWorld()->GetTimerManager().SetTimer(RespawnTimer, this, &AShooterCharacter::OnRespawn, RespawnTime, false);
}

void AShooterCharacter::OnRespawn()
{
	// destroy the character to force the PC to respawn
	Destroy();
}

bool AShooterCharacter::IsDead() const
{
	// the character is dead if their current HP drops to zero
	return CurrentHP <= 0.0f;
}

void AShooterCharacter::SetTeam(uint8 Team)
{
	TeamByte = Team;
}
