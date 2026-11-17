// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShooterWeaponHolder.h"
#include "Animation/AnimInstance.h"
#include "ShooterWeapon.generated.h"

class IShooterWeaponHolder;
class AShooterProjectile;
class USkeletalMeshComponent;
class UAnimMontage;
class UAnimInstance;

/**
 *  Base class for a simple first person shooter weapon
 *  Provides both first person and third person perspective meshes
 *  Handles ammo and firing logic
 *  Interacts with the weapon owner through the ShooterWeaponHolder interface
 */
UCLASS(abstract)
class EMPTY_API AShooterWeapon : public AActor
{
	GENERATED_BODY()
	
	/** First person perspective mesh */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* FirstPersonMesh;

	/** Third person perspective mesh */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* ThirdPersonMesh;

protected:

	/** Cast pointer to the weapon owner */
	IShooterWeaponHolder* WeaponOwner;

	/** Type of projectiles this weapon will shoot */
	UPROPERTY(EditAnywhere, Category="Ammo")
	TSubclassOf<AShooterProjectile> ProjectileClass;

	/** Number of bullets in a magazine */
	UPROPERTY(EditAnywhere, Category="Ammo", meta = (ClampMin = 0, ClampMax = 100))
	int32 MagazineSize = 10;

	/** Number of bullets in the current magazine */
	int32 CurrentBullets = 0;

	/** If true, the reserve never runs out (only the magazine matters) */
	UPROPERTY(EditAnywhere, Category="Ammo|Reload")
	bool bInfiniteReserveAmmo = true;

	/** Spare bullets carried outside the magazine. Ignored if bInfiniteReserveAmmo */
	UPROPERTY(EditAnywhere, Category="Ammo|Reload", meta = (ClampMin = 0, EditCondition = "!bInfiniteReserveAmmo"))
	int32 MaxReserveAmmo = 90;

	/** Current spare bullets */
	int32 ReserveAmmo = 0;

	/** How long a reload takes */
	UPROPERTY(EditAnywhere, Category="Ammo|Reload", meta = (ClampMin = 0, ClampMax = 10, Units = "s"))
	float ReloadTime = 1.5f;

	/** Automatically start reloading when the magazine runs dry */
	UPROPERTY(EditAnywhere, Category="Ammo|Reload")
	bool bAutoReloadWhenEmpty = true;

	/** Optional montage played on the character while reloading (stretched to ReloadTime). Needs a slot in the AnimBP */
	UPROPERTY(EditAnywhere, Category="Ammo|Reload")
	UAnimMontage* ReloadMontage = nullptr;

	/** Procedural reload motion: how far the weapon dips while reloading, in view space (X forward, Y right, Z up) */
	UPROPERTY(EditAnywhere, Category="Ammo|Reload", meta = (Units = "cm"))
	FVector ReloadDipOffset = FVector(-5.0f, 3.0f, -20.0f);

	/** True while a reload is in progress */
	bool bIsReloading = false;

	/** Timer that completes the reload */
	FTimerHandle ReloadTimer;

	/** If false, this weapon cannot aim down sights (e.g. grenade launcher) */
	UPROPERTY(EditAnywhere, Category="Aim|Down Sights")
	bool bCanAimDownSights = true;

	/** Camera field of view while aiming down sights (lower = more zoom) */
	UPROPERTY(EditAnywhere, Category="Aim|Down Sights", meta = (ClampMin = 5, ClampMax = 170, Units = "Degrees", EditCondition = "bCanAimDownSights"))
	float AimDownSightsFOV = 60.0f;

	/** AimVariance is multiplied by this while aiming (0 = perfect accuracy) */
	UPROPERTY(EditAnywhere, Category="Aim|Down Sights", meta = (ClampMin = 0, ClampMax = 1, EditCondition = "bCanAimDownSights"))
	float AimDownSightsVarianceMultiplier = 0.25f;

	/** Socket on the first person weapon mesh placed at the sight / optic. It gets aligned with the screen center while aiming.
	 *  Falls back to the muzzle socket if missing */
	UPROPERTY(EditAnywhere, Category="Aim|Down Sights", meta = (EditCondition = "bCanAimDownSights"))
	FName SightSocketName = FName("Sight");

	/** Experimental: slightly rotate the arms so the line Sight -> SightFront points exactly at the screen center.
	 *  Needs a SightFront socket. The rotation is capped by MaxSightAlignAngle because the arms mesh is the whole body */
	UPROPERTY(EditAnywhere, Category="Aim|Down Sights", meta = (EditCondition = "bCanAimDownSights"))
	bool bAlignSightRotation = false;

	/** Max correction angle for bAlignSightRotation */
	UPROPERTY(EditAnywhere, Category="Aim|Down Sights", meta = (ClampMin = 0, ClampMax = 30, Units = "Degrees", EditCondition = "bCanAimDownSights && bAlignSightRotation"))
	float MaxSightAlignAngle = 5.0f;

	/** Optional socket on the front sight (the post near the muzzle). Together with the Sight socket it defines the line of sight */
	UPROPERTY(EditAnywhere, Category="Aim|Down Sights", meta = (EditCondition = "bCanAimDownSights"))
	FName FrontSightSocketName = FName("SightFront");

	/** If > 0, also pulls the weapon towards the eye so the sight sits this far in front of the camera. 0 = only center it */
	UPROPERTY(EditAnywhere, Category="Aim|Down Sights", meta = (ClampMin = 0, ClampMax = 100, Units = "cm", EditCondition = "bCanAimDownSights"))
	float AimDownSightsEyeDistance = 0.0f;

	/** Recoil is multiplied by this while aiming */
	UPROPERTY(EditAnywhere, Category="Aim|Down Sights", meta = (ClampMin = 0, ClampMax = 1, EditCondition = "bCanAimDownSights"))
	float AimDownSightsRecoilMultiplier = 0.6f;

	/** Animation montage to play when firing this weapon */
	UPROPERTY(EditAnywhere, Category="Animation")
	UAnimMontage* FiringMontage;

	/** AnimInstance class to set for the first person character mesh when this weapon is active */
	UPROPERTY(EditAnywhere, Category="Animation")
	TSubclassOf<UAnimInstance> FirstPersonAnimInstanceClass;

	/** AnimInstance class to set for the third person character mesh when this weapon is active */
	UPROPERTY(EditAnywhere, Category="Animation")
	TSubclassOf<UAnimInstance> ThirdPersonAnimInstanceClass;

	/** Cone half-angle for variance while aiming */
	UPROPERTY(EditAnywhere, Category="Aim", meta = (ClampMin = 0, ClampMax = 90, Units = "Degrees"))
	float AimVariance = 0.0f;

	/** Amount of firing recoil to apply to the owner */
	UPROPERTY(EditAnywhere, Category="Aim", meta = (ClampMin = 0, ClampMax = 100))
	float FiringRecoil = 0.0f;

	/** Name of the first person muzzle socket where projectiles will spawn */
	UPROPERTY(EditAnywhere, Category="Aim")
	FName MuzzleSocketName;

	/** Distance ahead of the muzzle that bullets will spawn at */
	UPROPERTY(EditAnywhere, Category="Aim", meta = (ClampMin = 0, ClampMax = 1000, Units = "cm"))
	float MuzzleOffset = 10.0f;

	/** If true, this weapon will automatically fire at the refire rate */
	UPROPERTY(EditAnywhere, Category="Refire")
	bool bFullAuto = false;

	/** Time between shots for this weapon. Affects both full auto and semi auto modes */
	UPROPERTY(EditAnywhere, Category="Refire", meta = (ClampMin = 0, ClampMax = 5, Units = "s"))
	float RefireRate = 0.5f;

	/** Game time of last shot fired, used to enforce refire rate on semi auto */
	float TimeOfLastShot = 0.0f;

	/** If true, the weapon is currently firing */
	bool bIsFiring = false;

	/** Timer to handle full auto refiring */
	FTimerHandle RefireTimer;

	/** Cast pawn pointer to the owner for AI perception system interactions */
	TObjectPtr<APawn> PawnOwner;

	/** Loudness of the shot for AI perception system interactions */
	UPROPERTY(EditAnywhere, Category="Perception", meta = (ClampMin = 0, ClampMax = 100))
	float ShotLoudness = 1.0f;

	/** Max range of shot AI perception noise */
	UPROPERTY(EditAnywhere, Category="Perception", meta = (ClampMin = 0, ClampMax = 100000, Units = "cm"))
	float ShotNoiseRange = 300.0f;

	/** Tag to apply to noise generated by shooting this weapon */
	UPROPERTY(EditAnywhere, Category="Perception")
	FName NoiseOwnerTag = FName("Shot");

public:	

	/** Constructor */
	AShooterWeapon();

protected:
	
	/** Gameplay initialization */
	virtual void BeginPlay() override;

	/** Gameplay Cleanup */
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

protected:

	/** Called when the weapon's owner is destroyed */
	UFUNCTION()
	void OnOwnerDestroyed(AActor* DestroyedActor);

public:

	/** Activates this weapon and gets it ready to fire */
	void ActivateWeapon(const FName& OwnerTag);

	/** Deactivates this weapon */
	void DeactivateWeapon();

	/** Start firing this weapon */
	void StartFiring();

	/** Stop firing this weapon */
	void StopFiring();

	/** Starts a reload if possible. Returns true if a reload was started */
	bool StartReload();

	/** Interrupts a reload in progress without refilling */
	void CancelReload();

protected:

	/** Completes the reload and refills the magazine */
	void FinishReload();

	/** True if the owner is currently aiming down sights with this weapon */
	bool IsOwnerAiming() const;

protected:

	/** Fire the weapon */
	virtual void Fire();

	/** Called when the refire rate time has passed while shooting semi auto weapons */
	void FireCooldownExpired();

	/** Fire a projectile towards the target location */
	virtual void FireProjectile(const FVector& TargetLocation);

	/** Calculates the spawn transform for projectiles shot by this weapon */
	FTransform CalculateProjectileSpawnTransform(const FVector& TargetLocation) const;

public:

	/** Returns the first person mesh */
	UFUNCTION(BlueprintPure, Category="Weapon")
	USkeletalMeshComponent* GetFirstPersonMesh() const { return FirstPersonMesh; };

	/** Returns the third person mesh */
	UFUNCTION(BlueprintPure, Category="Weapon")
	USkeletalMeshComponent* GetThirdPersonMesh() const { return ThirdPersonMesh; };

	/** Returns the first person anim instance class */
	const TSubclassOf<UAnimInstance>& GetFirstPersonAnimInstanceClass() const;

	/** Returns the third person anim instance class */
	const TSubclassOf<UAnimInstance>& GetThirdPersonAnimInstanceClass() const;

	/** Returns the magazine size */
	int32 GetMagazineSize() const { return MagazineSize; };

	/** Returns the current bullet count */
	int32 GetBulletCount() const { return CurrentBullets; }

	/** Returns the spare ammo count (-1 if infinite) */
	UFUNCTION(BlueprintPure, Category="Weapon")
	int32 GetReserveAmmo() const { return bInfiniteReserveAmmo ? -1 : ReserveAmmo; }

	/** Returns true while reloading */
	UFUNCTION(BlueprintPure, Category="Weapon")
	bool IsReloading() const { return bIsReloading; }

	/** Returns true if this weapon supports aiming down sights */
	bool CanAimDownSights() const { return bCanAimDownSights; }

	/** Returns the ADS camera FOV */
	float GetAimDownSightsFOV() const { return AimDownSightsFOV; }

	/** Returns the ADS eye distance (0 = disabled) */
	float GetAimDownSightsEyeDistance() const { return AimDownSightsEyeDistance; }

	/** Returns the procedural reload dip offset in view space */
	const FVector& GetReloadDipOffset() const { return ReloadDipOffset; }

	/** World location of the sight socket (or muzzle as fallback). Returns false if neither exists */
	bool GetSightLocation(FVector& OutLocation) const;

	/** World transform of the sight socket (or muzzle as fallback). Returns false if neither exists */
	bool GetSightTransform(FTransform& OutTransform) const;

	/** World location of the front sight socket. Returns false if the socket doesn't exist */
	bool GetFrontSightLocation(FVector& OutLocation) const;

	/** Returns true if the weapon should be rotated to line up with the view while aiming */
	bool ShouldAlignSightRotation() const { return bAlignSightRotation; }

	/** Max correction angle in degrees for the sight rotation alignment */
	float GetMaxSightAlignAngle() const { return MaxSightAlignAngle; }
};
