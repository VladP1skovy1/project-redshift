// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EmptyCharacter.h"
#include "ShooterWeaponHolder.h"
#include "ShooterCharacter.generated.h"

class AShooterWeapon;
class UInputAction;
class UInputComponent;
class UPawnNoiseEmitterComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FBulletCountUpdatedDelegate, int32, MagazineSize, int32, Bullets);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDamagedDelegate, float, LifePercent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAimDownSightsChangedDelegate, bool, bAiming);

/**
 *  A player controllable first person shooter character
 *  Manages a weapon inventory through the IShooterWeaponHolder interface
 *  Manages health and death
 */
UCLASS(abstract)
class EMPTY_API AShooterCharacter : public AEmptyCharacter, public IShooterWeaponHolder
{
	GENERATED_BODY()
	
	/** AI Noise emitter component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UPawnNoiseEmitterComponent* PawnNoiseEmitter;

protected:

	/** Fire weapon input action */
	UPROPERTY(EditAnywhere, Category ="Input")
	UInputAction* FireAction;

	/** Switch weapon input action */
	UPROPERTY(EditAnywhere, Category ="Input")
	UInputAction* SwitchWeaponAction;

	/** Aim down sights input action (hold) */
	UPROPERTY(EditAnywhere, Category ="Input")
	UInputAction* AimDownSightsAction;

	/** Reload input action */
	UPROPERTY(EditAnywhere, Category ="Input")
	UInputAction* ReloadAction;

	/** How fast the camera zooms in and out when aiming */
	UPROPERTY(EditAnywhere, Category ="Aim|Down Sights", meta = (ClampMin = 1, ClampMax = 50))
	float AimDownSightsInterpSpeed = 15.0f;

	/** Walk speed multiplier while aiming down sights */
	UPROPERTY(EditAnywhere, Category ="Aim|Down Sights", meta = (ClampMin = 0.1, ClampMax = 1))
	float AimDownSightsWalkSpeedMultiplier = 0.6f;

	/** True while the aim button is held */
	bool bWantsToAimDownSights = false;

	/** True while actually aiming (button held, weapon supports it, not reloading) */
	UPROPERTY(BlueprintReadOnly, Category ="Aim|Down Sights")
	bool bIsAimingDownSights = false;

	/** How fast the weapon dips down / comes back up while reloading */
	UPROPERTY(EditAnywhere, Category ="Ammo|Reload", meta = (ClampMin = 1, ClampMax = 50))
	float ReloadDipInterpSpeed = 8.0f;

	/** 0..1 blend of the aim down sights weapon offset */
	float AimDownSightsAlpha = 0.0f;

	/** 0..1 blend of the procedural reload motion */
	float ReloadAlpha = 0.0f;

	/** Default placement of the arms mesh and camera, cached on BeginPlay */
	FVector DefaultArmsRelativeLocation = FVector::ZeroVector;
	FRotator DefaultArmsRelativeRotation = FRotator::ZeroRotator;

	/** Handle for the arms "bone transforms finalized" callback */
	FDelegateHandle ArmsBonesFinalizedHandle;
	FVector DefaultCameraRelativeLocation = FVector::ZeroVector;

	/** Camera FOV and walk speed when not aiming, cached on BeginPlay */
	float DefaultFieldOfView = 90.0f;
	float DefaultMaxWalkSpeed = 600.0f;

	/** Name of the first person mesh weapon socket */
	UPROPERTY(EditAnywhere, Category ="Weapons")
	FName FirstPersonWeaponSocket = FName("HandGrip_R");

	/** Name of the third person mesh weapon socket */
	UPROPERTY(EditAnywhere, Category ="Weapons")
	FName ThirdPersonWeaponSocket = FName("HandGrip_R");

	/** Max distance to use for aim traces */
	UPROPERTY(EditAnywhere, Category ="Aim", meta = (ClampMin = 0, ClampMax = 100000, Units = "cm"))
	float MaxAimDistance = 10000.0f;

	/** Max HP this character can have */
	UPROPERTY(EditAnywhere, Category="Health")
	float MaxHP = 500.0f;

	/** Current HP remaining to this character */
	float CurrentHP = 0.0f;

	/** Team ID for this character*/
	UPROPERTY(EditAnywhere, Category="Team")
	uint8 TeamByte = 0;

	/** Actor tag to grant this character when it dies */
	UPROPERTY(EditAnywhere, Category="Team")
	FName DeathTag = FName("Dead");

	/** Tag to pass to weapons and projectiles to identify their AI perception noise as player-generated */
	UPROPERTY(EditAnywhere, Category="Tags")
	FName PlayerTag = FName("Player");

	/** List of weapons picked up by the character */
	TArray<AShooterWeapon*> OwnedWeapons;

	/** Weapon currently equipped and ready to shoot with */
	TObjectPtr<AShooterWeapon> CurrentWeapon;

	UPROPERTY(EditAnywhere, Category ="Destruction", meta = (ClampMin = 0, ClampMax = 10, Units = "s"))
	float RespawnTime = 5.0f;

	FTimerHandle RespawnTimer;

public:

	/** Bullet count updated delegate */
	FBulletCountUpdatedDelegate OnBulletCountUpdated;

	/** Damaged delegate */
	FDamagedDelegate OnDamaged;

	/** Aim down sights started / stopped (used by the HUD to hide the crosshair) */
	FAimDownSightsChangedDelegate OnAimDownSightsChanged;

public:

	/** Constructor */
	AShooterCharacter();

protected:

	/** Gameplay initialization */
	virtual void BeginPlay() override;

	/** Gameplay cleanup */
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

	/** Updates aim down sights state and camera zoom */
	virtual void Tick(float DeltaSeconds) override;

	/** Moves the arms so the sight lines up with the screen center (ADS) and dips the weapon while reloading.
	 *  The camera is kept in place, so only the arms and weapon move on screen */
	void UpdateFirstPersonWeaponOffset();

	/** Called every frame right after the arms animation has finished, so the offsets use this frame's pose (no jitter) */
	UFUNCTION()
	void OnArmsBoneTransformsFinalized();

	/** Set up input action bindings */
	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;

public:

	/** Handle incoming damage */
	virtual float TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

public:

	/** Handles aim inputs from either controls or UI interfaces */
	virtual void DoAim(float Yaw, float Pitch) override;

	/** Handles move inputs from either controls or UI interfaces */
	virtual void DoMove(float Right, float Forward)  override;

	/** Handles jump start inputs from either controls or UI interfaces */
	virtual void DoJumpStart()  override;

	/** Handles jump end inputs from either controls or UI interfaces */
	virtual void DoJumpEnd()  override;

	/** Handles start firing input */
	UFUNCTION(BlueprintCallable, Category="Input")
	void DoStartFiring();

	/** Handles stop firing input */
	UFUNCTION(BlueprintCallable, Category="Input")
	void DoStopFiring();

	/** Handles switch weapon input */
	UFUNCTION(BlueprintCallable, Category="Input")
	void DoSwitchWeapon();

	/** Handles aim down sights press */
	UFUNCTION(BlueprintCallable, Category="Input")
	void DoStartAimDownSights();

	/** Handles aim down sights release */
	UFUNCTION(BlueprintCallable, Category="Input")
	void DoStopAimDownSights();

	/** Handles reload input */
	UFUNCTION(BlueprintCallable, Category="Input")
	void DoReload();

public:

	//~Begin IShooterWeaponHolder interface

	/** Attaches a weapon's meshes to the owner */
	virtual void AttachWeaponMeshes(AShooterWeapon* Weapon) override;

	/** Plays the firing montage for the weapon */
	virtual void PlayFiringMontage(UAnimMontage* Montage) override;

	/** Applies weapon recoil to the owner */
	virtual void AddWeaponRecoil(float Recoil) override;

	/** Updates the weapon's HUD with the current ammo count */
	virtual void UpdateWeaponHUD(int32 CurrentAmmo, int32 MagazineSize) override;

	/** Calculates and returns the aim location for the weapon */
	virtual FVector GetWeaponTargetLocation() override;

	/** Gives a weapon of this class to the owner */
	virtual void AddWeaponClass(const TSubclassOf<AShooterWeapon>& WeaponClass) override;

	/** Activates the passed weapon */
	virtual void OnWeaponActivated(AShooterWeapon* Weapon) override;

	/** Deactivates the passed weapon */
	virtual void OnWeaponDeactivated(AShooterWeapon* Weapon) override;

	/** Notifies the owner that the weapon cooldown has expired and it's ready to shoot again */
	virtual void OnSemiWeaponRefire() override;

	/** Plays the reload montage, if any */
	virtual void OnWeaponReloadStarted(AShooterWeapon* Weapon, UAnimMontage* Montage, float Duration) override;

	/** Stops the reload montage */
	virtual void OnWeaponReloadCancelled(AShooterWeapon* Weapon, UAnimMontage* Montage) override;

	/** Returns true while aiming down sights (Blueprints / AnimBP can read bIsAimingDownSights) */
	virtual bool IsAimingDownSights() const override { return bIsAimingDownSights; }

	//~End IShooterWeaponHolder interface

protected:

	/** Returns true if the character already owns a weapon of the given class */
	AShooterWeapon* FindWeaponOfType(TSubclassOf<AShooterWeapon> WeaponClass) const;

	/** Called when this character's HP is depleted */
	void Die();

	/** Called to allow Blueprint code to react to this character's death */
	UFUNCTION(BlueprintImplementableEvent, Category="Shooter", meta = (DisplayName = "On Death"))
	void BP_OnDeath();

	/** Called from the respawn timer to destroy this character and force the PC to respawn */
	void OnRespawn();

public:

	/** Returns the currently equipped weapon (may be null) */
	AShooterWeapon* GetCurrentWeapon() const { return CurrentWeapon; }

	/** Returns true if the character is dead */
	bool IsDead() const;

	/** Sets the team ID for this character */
	void SetTeam(uint8 Team);
};
