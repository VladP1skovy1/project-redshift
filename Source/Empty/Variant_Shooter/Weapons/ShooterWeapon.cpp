// Copyright Epic Games, Inc. All Rights Reserved.


#include "ShooterWeapon.h"
#include "Kismet/KismetMathLibrary.h"
#include "Engine/World.h"
#include "ShooterProjectile.h"
#include "ShooterWeaponHolder.h"
#include "Components/SceneComponent.h"
#include "TimerManager.h"
#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "Components/PointLightComponent.h"

AShooterWeapon::AShooterWeapon()
{
	PrimaryActorTick.bCanEverTick = true;

	// create the root
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	// create the first person mesh
	FirstPersonMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("First Person Mesh"));
	FirstPersonMesh->SetupAttachment(RootComponent);

	FirstPersonMesh->SetCollisionProfileName(FName("NoCollision"));
	FirstPersonMesh->SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType::FirstPerson);
	FirstPersonMesh->bOnlyOwnerSee = true;

	// create the third person mesh
	ThirdPersonMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Third Person Mesh"));
	ThirdPersonMesh->SetupAttachment(RootComponent);

	ThirdPersonMesh->SetCollisionProfileName(FName("NoCollision"));
	ThirdPersonMesh->SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType::WorldSpaceRepresentation);
	ThirdPersonMesh->bOwnerNoSee = true;

	// create the muzzle flash light (attached to the muzzle socket on BeginPlay)
	MuzzleFlashLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("Muzzle Flash Light"));
	MuzzleFlashLight->SetupAttachment(FirstPersonMesh);
	MuzzleFlashLight->SetVisibility(false);
	MuzzleFlashLight->SetCastShadows(false);
	MuzzleFlashLight->SetIntensity(0.0f);
	MuzzleFlashLight->SetIntensityUnits(ELightUnits::Unitless);

	// first person meshes can't cast shadows, so the flash would shine through the gun onto the hands.
	// The flash only uses lighting channel 0 (the world); the first person weapon mesh uses channel 1 instead
	MuzzleFlashLight->LightingChannels.bChannel0 = true;
	MuzzleFlashLight->LightingChannels.bChannel1 = false;
	FirstPersonMesh->LightingChannels.bChannel0 = false;
	FirstPersonMesh->LightingChannels.bChannel1 = true;
}

void AShooterWeapon::BeginPlay()
{
	Super::BeginPlay();

	// subscribe to the owner's destroyed delegate
	GetOwner()->OnDestroyed.AddDynamic(this, &AShooterWeapon::OnOwnerDestroyed);

	// cast the weapon owner
	WeaponOwner = Cast<IShooterWeaponHolder>(GetOwner());
	PawnOwner = Cast<APawn>(GetOwner());

	// fill the first ammo clip and the reserve
	CurrentBullets = MagazineSize;
	ReserveAmmo = MaxReserveAmmo;

	// attach the meshes to the owner
	WeaponOwner->AttachWeaponMeshes(this);

	// put the muzzle flash light on the muzzle
	if (FirstPersonMesh->DoesSocketExist(MuzzleSocketName))
	{
		MuzzleFlashLight->AttachToComponent(FirstPersonMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, MuzzleSocketName);
	}

	MuzzleFlashLight->SetLightColor(MuzzleFlashColor);
	MuzzleFlashLight->SetAttenuationRadius(MuzzleFlashRadius);
}

void AShooterWeapon::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	// clear the refire, reload and muzzle flash timers
	GetWorld()->GetTimerManager().ClearTimer(RefireTimer);
	GetWorld()->GetTimerManager().ClearTimer(ReloadTimer);
	GetWorld()->GetTimerManager().ClearTimer(MuzzleFlashTimer);
}

void AShooterWeapon::OnOwnerDestroyed(AActor* DestroyedActor)
{
	// ensure this weapon is destroyed when the owner is destroyed
	Destroy();
}

void AShooterWeapon::ActivateWeapon(const FName& OwnerTag)
{
	// save the owner tag for perception noise detection
	NoiseOwnerTag = OwnerTag;

	// unhide this weapon
	SetActorHiddenInGame(false);

	// notify the owner
	WeaponOwner->OnWeaponActivated(this);
}

void AShooterWeapon::DeactivateWeapon()
{
	// ensure we're no longer firing or reloading this weapon while deactivated
	StopFiring();
	CancelReload();
	HideMuzzleFlash();

	// hide the weapon
	SetActorHiddenInGame(true);

	// notify the owner
	WeaponOwner->OnWeaponDeactivated(this);
}

void AShooterWeapon::StartFiring()
{
	// raise the firing flag
	bIsFiring = true;

	// check how much time has passed since we last shot
	// this may be under the refire rate if the weapon shoots slow enough and the player is spamming the trigger
	const float TimeSinceLastShot = GetWorld()->GetTimeSeconds() - TimeOfLastShot;

	if (TimeSinceLastShot > RefireRate)
	{
		// fire the weapon right away
		Fire();

	} else {

		// if we're full auto, schedule the next shot
		if (bFullAuto)
		{
			GetWorld()->GetTimerManager().SetTimer(RefireTimer, this, &AShooterWeapon::Fire, TimeSinceLastShot, false);
		}

	}
}

void AShooterWeapon::StopFiring()
{
	// lower the firing flag
	bIsFiring = false;

	// clear the refire timer
	GetWorld()->GetTimerManager().ClearTimer(RefireTimer);
}

void AShooterWeapon::Fire()
{
	// ensure the player still wants to fire. They may have let go of the trigger
	if (!bIsFiring)
	{
		return;
	}

	// can't shoot while reloading. Full auto fire resumes in FinishReload
	if (bIsReloading)
	{
		return;
	}

	// empty magazine: try to reload instead of shooting
	if (CurrentBullets <= 0)
	{
		StartReload();
		return;
	}
	
	// fire a projectile at the target
	const FVector TargetLocation = WeaponOwner->GetWeaponTargetLocation();
	FireProjectile(TargetLocation);

	// flash
	PlayMuzzleFlash(TargetLocation);

	// update the time of our last shot
	TimeOfLastShot = GetWorld()->GetTimeSeconds();

	// make noise so the AI perception system can hear us
	MakeNoise(ShotLoudness, PawnOwner, PawnOwner->GetActorLocation(), ShotNoiseRange, NoiseOwnerTag);

	// are we full auto?
	if (bFullAuto)
	{
		// schedule the next shot
		GetWorld()->GetTimerManager().SetTimer(RefireTimer, this, &AShooterWeapon::Fire, RefireRate, false);
	} else {

		// for semi-auto weapons, schedule the cooldown notification
		GetWorld()->GetTimerManager().SetTimer(RefireTimer, this, &AShooterWeapon::FireCooldownExpired, RefireRate, false);

	}
}

void AShooterWeapon::FireCooldownExpired()
{
	// notify the owner
	WeaponOwner->OnSemiWeaponRefire();
}

void AShooterWeapon::FireProjectile(const FVector& TargetLocation)
{
	// get the projectile transform
	FTransform ProjectileTransform = CalculateProjectileSpawnTransform(TargetLocation);
	
	// spawn the projectile
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParams.TransformScaleMethod = ESpawnActorScaleMethod::OverrideRootScale;
	SpawnParams.Owner = GetOwner();
	SpawnParams.Instigator = PawnOwner;

	AShooterProjectile* Projectile = GetWorld()->SpawnActor<AShooterProjectile>(ProjectileClass, ProjectileTransform, SpawnParams);

	// set the noise tag on the projectile
	if (Projectile)
	{
		Projectile->SetNoiseTag(NoiseOwnerTag);
	}

	// play the firing montage
	WeaponOwner->PlayFiringMontage(FiringMontage);

	// add recoil (pattern based, reduced while aiming down sights)
	ApplyRecoil();

	// consume bullets
	--CurrentBullets;

	// update the weapon HUD
	WeaponOwner->UpdateWeaponHUD(CurrentBullets, MagazineSize);

	// if the clip is depleted, start reloading
	if (CurrentBullets <= 0 && bAutoReloadWhenEmpty)
	{
		StartReload();
	}
}

void AShooterWeapon::ApplyRecoil()
{
	const float Now = GetWorld()->GetTimeSeconds();

	// start the pattern over if we haven't been shooting for a while
	if (Now - TimeOfLastRecoil > RecoilPatternResetTime)
	{
		RecoilShotIndex = 0;
	}

	FVector2D Kick;

	if (RecoilPattern.Num() > 0)
	{
		// fixed, learnable pattern. Once it runs out, the last entry repeats
		Kick = RecoilPattern[FMath::Min(RecoilShotIndex, RecoilPattern.Num() - 1)];
	}
	else
	{
		Kick = FVector2D(FMath::FRandRange(-RecoilYawRandom, RecoilYawRandom), RecoilPitch);
	}

	++RecoilShotIndex;
	TimeOfLastRecoil = Now;

	const float Multiplier = IsOwnerAiming() ? AimDownSightsRecoilMultiplier : 1.0f;

	// FRotator(Pitch, Yaw, Roll)
	WeaponOwner->ApplyWeaponRecoil(FRotator(Kick.Y * Multiplier, Kick.X * Multiplier, 0.0f), this);
}

void AShooterWeapon::PlayMuzzleFlash(const FVector& TargetLocation)
{
	if (!bMuzzleFlash || !MuzzleFlashLight)
	{
		return;
	}

	// put the light a bit ahead of the muzzle, along the shot direction
	if (FirstPersonMesh->DoesSocketExist(MuzzleSocketName))
	{
		const FVector MuzzleLocation = FirstPersonMesh->GetSocketLocation(MuzzleSocketName);
		const FVector ToTarget = TargetLocation - MuzzleLocation;

		// don't push the light through a wall we're standing right in front of
		const float Distance = FMath::Min(MuzzleFlashForwardOffset, ToTarget.Size() * 0.5f);

		MuzzleFlashLight->SetWorldLocation(MuzzleLocation + ToTarget.GetSafeNormal() * Distance);
	}

	MuzzleFlashLight->SetIntensity(MuzzleFlashIntensity * (IsOwnerAiming() ? MuzzleFlashAimMultiplier : 1.0f));
	MuzzleFlashLight->SetVisibility(true);

	GetWorld()->GetTimerManager().SetTimer(MuzzleFlashTimer, this, &AShooterWeapon::HideMuzzleFlash, MuzzleFlashDuration, false);
}

void AShooterWeapon::HideMuzzleFlash()
{
	if (MuzzleFlashLight)
	{
		MuzzleFlashLight->SetVisibility(false);
		MuzzleFlashLight->SetIntensity(0.0f);
	}
}

bool AShooterWeapon::StartReload()
{
	// already reloading, magazine full, or nothing left to load
	if (bIsReloading || CurrentBullets >= MagazineSize || (!bInfiniteReserveAmmo && ReserveAmmo <= 0))
	{
		return false;
	}

	bIsReloading = true;

	// stop any pending shot
	GetWorld()->GetTimerManager().ClearTimer(RefireTimer);

	// schedule the end of the reload
	if (ReloadTime > 0.0f)
	{
		GetWorld()->GetTimerManager().SetTimer(ReloadTimer, this, &AShooterWeapon::FinishReload, ReloadTime, false);
	}

	// notify the owner so it can play animations
	if (WeaponOwner)
	{
		WeaponOwner->OnWeaponReloadStarted(this, ReloadMontage, ReloadTime);
	}

	// instant reload
	if (ReloadTime <= 0.0f)
	{
		FinishReload();
	}

	return true;
}

void AShooterWeapon::FinishReload()
{
	if (!bIsReloading)
	{
		return;
	}

	// move bullets from the reserve into the magazine
	const int32 Needed = MagazineSize - CurrentBullets;
	const int32 Loaded = bInfiniteReserveAmmo ? Needed : FMath::Min(Needed, ReserveAmmo);

	CurrentBullets += Loaded;

	if (!bInfiniteReserveAmmo)
	{
		ReserveAmmo -= Loaded;
	}

	bIsReloading = false;

	if (WeaponOwner)
	{
		WeaponOwner->UpdateWeaponHUD(CurrentBullets, MagazineSize);
		WeaponOwner->OnWeaponReloadFinished(this);
	}

	// if the trigger is still held on a full auto weapon, keep shooting
	if (bIsFiring && bFullAuto)
	{
		Fire();
	}
	else if (!bFullAuto && WeaponOwner)
	{
		// semi auto weapons are ready to shoot again
		WeaponOwner->OnSemiWeaponRefire();
	}
}

void AShooterWeapon::CancelReload()
{
	if (!bIsReloading)
	{
		return;
	}

	bIsReloading = false;
	GetWorld()->GetTimerManager().ClearTimer(ReloadTimer);

	if (WeaponOwner)
	{
		WeaponOwner->OnWeaponReloadCancelled(this, ReloadMontage);
	}
}

bool AShooterWeapon::IsOwnerAiming() const
{
	return bCanAimDownSights && WeaponOwner && WeaponOwner->IsAimingDownSights();
}

FTransform AShooterWeapon::CalculateProjectileSpawnTransform(const FVector& TargetLocation) const
{
	// find the muzzle location
	const FVector MuzzleLoc = FirstPersonMesh->GetSocketLocation(MuzzleSocketName);

	// calculate the spawn location ahead of the muzzle
	const FVector SpawnLoc = MuzzleLoc + ((TargetLocation - MuzzleLoc).GetSafeNormal() * MuzzleOffset);

	// apply less variance while aiming down sights
	const float Variance = AimVariance * (IsOwnerAiming() ? AimDownSightsVarianceMultiplier : 1.0f);

	// find the aim rotation vector while applying some variance to the target 
	const FRotator AimRot = UKismetMathLibrary::FindLookAtRotation(SpawnLoc, TargetLocation + (UKismetMathLibrary::RandomUnitVector() * Variance));

	// return the built transform
	return FTransform(AimRot, SpawnLoc, FVector::OneVector);
}

bool AShooterWeapon::GetSightLocation(FVector& OutLocation) const
{
	if (FirstPersonMesh->DoesSocketExist(SightSocketName))
	{
		OutLocation = FirstPersonMesh->GetSocketLocation(SightSocketName);
		return true;
	}

	// fallback: line up the barrel instead
	if (FirstPersonMesh->DoesSocketExist(MuzzleSocketName))
	{
		OutLocation = FirstPersonMesh->GetSocketLocation(MuzzleSocketName);
		return true;
	}

	return false;
}

bool AShooterWeapon::GetFrontSightLocation(FVector& OutLocation) const
{
	if (FirstPersonMesh->DoesSocketExist(FrontSightSocketName))
	{
		OutLocation = FirstPersonMesh->GetSocketLocation(FrontSightSocketName);
		return true;
	}

	return false;
}

bool AShooterWeapon::GetSightTransform(FTransform& OutTransform) const
{
	if (FirstPersonMesh->DoesSocketExist(SightSocketName))
	{
		OutTransform = FirstPersonMesh->GetSocketTransform(SightSocketName);
		return true;
	}

	if (FirstPersonMesh->DoesSocketExist(MuzzleSocketName))
	{
		OutTransform = FirstPersonMesh->GetSocketTransform(MuzzleSocketName);
		return true;
	}

	return false;
}

const TSubclassOf<UAnimInstance>& AShooterWeapon::GetFirstPersonAnimInstanceClass() const
{
	return FirstPersonAnimInstanceClass;
}

const TSubclassOf<UAnimInstance>& AShooterWeapon::GetThirdPersonAnimInstanceClass() const
{
	return ThirdPersonAnimInstanceClass;
}
