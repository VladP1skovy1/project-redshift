// Melee weapon (stick, hammer, knife, axe...) built on top of the shooter weapon so it uses the same inventory,
// switching, HUD and procedural animation systems.

#pragma once

#include "CoreMinimal.h"
#include "ShooterWeapon.h"
#include "ShooterMeleeWeapon.generated.h"

class UStaticMeshComponent;
class UAnimMontage;
class UDamageType;

/**
 *  A melee weapon. Left mouse swings, holding it keeps swinging every Refire Rate seconds.
 *  The hit is checked once per swing, Hit Delay seconds after the click, with a sphere sweep from the camera.
 *  Static meshes (a stick, a hammer) go into the First/Third Person Static Mesh components.
 */
UCLASS(abstract)
class EMPTY_API AShooterMeleeWeapon : public AShooterWeapon
{
	GENERATED_BODY()

	/** First person static mesh (alternative to the skeletal First Person Mesh) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* FirstPersonStaticMesh;

	/** Third person static mesh (what other players see) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* ThirdPersonStaticMesh;

protected:

	/** Damage per hit */
	UPROPERTY(EditAnywhere, Category="Melee", meta = (ClampMin = 0))
	float MeleeDamage = 35.0f;

	/** How far the swing reaches from the camera */
	UPROPERTY(EditAnywhere, Category="Melee", meta = (ClampMin = 10, ClampMax = 500, Units = "cm"))
	float MeleeRange = 160.0f;

	/** Thickness of the swing (sphere sweep radius). Bigger = easier to hit */
	UPROPERTY(EditAnywhere, Category="Melee", meta = (ClampMin = 1, ClampMax = 100, Units = "cm"))
	float MeleeRadius = 15.0f;


	/** If true, one swing can hit several enemies standing in a line */
	UPROPERTY(EditAnywhere, Category="Melee")
	bool bHitMultipleTargets = false;

	/** Push applied to physics objects that get hit */
	UPROPERTY(EditAnywhere, Category="Melee", meta = (ClampMin = 0))
	float HitImpulse = 30000.0f;

	/** What kind of tool this is, passed to IShooterMeleeTarget (e.g. "Knife", "Hammer", "Stick", "Axe") */
	UPROPERTY(EditAnywhere, Category="Melee")
	FName ToolType = FName("Knife");

	/** Damage type for ApplyPointDamage */
	UPROPERTY(EditAnywhere, Category="Melee")
	TSubclassOf<UDamageType> MeleeDamageType;

	/** Optional swing montage, e.g. made from MM_Attack_01. Needs a Slot node (DefaultSlot) in the AnimBP it plays on */
	UPROPERTY(EditAnywhere, Category="Melee")
	UAnimMontage* SwingMontage = nullptr;

	/** Play the swing montage on the first person arms (off by default: the arms use the procedural swing) */
	UPROPERTY(EditAnywhere, Category="Melee")
	bool bSwingMontageFirstPerson = false;

	/** Play the swing montage on the third person body (what the shadow and other players see) */
	UPROPERTY(EditAnywhere, Category="Melee")
	bool bSwingMontageThirdPerson = true;

	// ---------------- Swing animation (Half-Life style) ----------------
	// Phases: wind up (pull back) -> strike (fast forward, the hit is checked here) -> hold (short pause) -> return

	/** Pull-back time */
	UPROPERTY(EditAnywhere, Category="Melee|Swing", meta = (ClampMin = 0.01, ClampMax = 1, Units = "s"))
	float SwingWindupTime = 0.14f;

	/** Strike time (fast). The hit is checked near the end of it */
	UPROPERTY(EditAnywhere, Category="Melee|Swing", meta = (ClampMin = 0.01, ClampMax = 1, Units = "s"))
	float SwingStrikeTime = 0.09f;

	/** Pause at the end of the strike */
	UPROPERTY(EditAnywhere, Category="Melee|Swing", meta = (ClampMin = 0, ClampMax = 1, Units = "s"))
	float SwingHoldTime = 0.1f;

	/** Time to come back to the idle pose */
	UPROPERTY(EditAnywhere, Category="Melee|Swing", meta = (ClampMin = 0.01, ClampMax = 1, Units = "s"))
	float SwingReturnTime = 0.22f;

	/** Arms offset at the end of the wind up, view space (X forward, Y right, Z up). Default: pulled back to the right and up */
	UPROPERTY(EditAnywhere, Category="Melee|Swing", meta = (Units = "cm"))
	FVector SwingWindupOffset = FVector(-4.0f, 6.0f, 2.0f);

	/** Arms offset at the end of the strike. With Aim Strike At Crosshair only X (forward) really matters */
	UPROPERTY(EditAnywhere, Category="Melee|Swing", meta = (Units = "cm"))
	FVector SwingStrikeOffset = FVector(10.0f, 0.0f, 0.0f);

	/** During the strike pull the tip of the item exactly to the screen center (where the hit is checked) */
	UPROPERTY(EditAnywhere, Category="Melee|Swing")
	bool bAimStrikeAtCrosshair = true;

	/** Optional socket on the first person static mesh marking the tip. If missing, the point of the mesh farthest from the hand is used */
	UPROPERTY(EditAnywhere, Category="Melee|Swing", meta = (EditCondition = "bAimStrikeAtCrosshair"))
	FName TipSocketName = FName("Tip");

	/** Where the tip of the item points at the end of the wind up, relative to the view: Yaw (+ right) and Pitch (+ up), degrees */
	UPROPERTY(EditAnywhere, Category="Melee|Swing")
	FRotator SwingWindupDirection = FRotator(35.0f, 70.0f, 0.0f);

	/** At the end of the strike the item points from the hand to the point this far in front of the camera (at the crosshair) */
	UPROPERTY(EditAnywhere, Category="Melee|Swing", meta = (ClampMin = 10, ClampMax = 500, Units = "cm"))
	float SwingStrikeReach = 120.0f;

	/** "Hit stop": the swing freezes for this long when it connects, sells the impact */
	UPROPERTY(EditAnywhere, Category="Melee|Swing", meta = (ClampMin = 0, ClampMax = 0.5, Units = "s"))
	float HitStopTime = 0.07f;

	/** Camera punch when the swing connects (Pitch up, Yaw right, degrees) */
	UPROPERTY(EditAnywhere, Category="Melee|Swing")
	FRotator HitCameraPunch = FRotator(-1.2f, 0.6f, 0.0f);

	/** Current time in the swing, < 0 when not swinging */
	float SwingTime = -1.0f;

	/** Remaining hit stop time */
	float HitStopRemaining = 0.0f;

	/** Current arms offset from the swing */
	FVector CurrentSwingOffset = FVector::ZeroVector;

	/** Current 0..1 strength of the tip-to-crosshair pull */
	float CurrentStrikeAimAlpha = 0.0f;

	/** Current 0..1 weight of the swing direction override, and 0 (wind up direction) .. 1 (crosshair) blend */
	float CurrentSwingPoseWeight = 0.0f;
	float CurrentSwingStrikeBlend = 0.0f;

	/** Timer for the delayed hit check */
	FTimerHandle MeleeHitTimer;

public:

	AShooterMeleeWeapon();

protected:

	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

	/** Advances the swing animation */
	virtual void Tick(float DeltaSeconds) override;

	/** Total swing duration */
	float GetSwingDuration() const { return SwingWindupTime + SwingStrikeTime + SwingHoldTime + SwingReturnTime; }

	/** A swing instead of a shot */
	virtual void Fire() override;

	/** Sweeps in front of the camera and applies damage */
	void PerformMeleeHit();

	/** Blueprint hook for hit sounds / particles. HitActor may be a wall */
	UFUNCTION(BlueprintImplementableEvent, Category="Melee", meta = (DisplayName = "On Melee Hit"))
	void BP_OnMeleeHit(const FHitResult& Hit, AActor* HitActor);

	/** Blueprint hook for swing-and-miss sounds */
	UFUNCTION(BlueprintImplementableEvent, Category="Melee", meta = (DisplayName = "On Melee Miss"))
	void BP_OnMeleeMiss();

public:

	UFUNCTION(BlueprintPure, Category="Melee")
	FName GetToolType() const { return ToolType; }

	/** Arms offset from the swing animation */
	virtual FVector GetProceduralViewOffset() const override { return CurrentSwingOffset; }

	/** Tip-to-crosshair pull during the strike */
	virtual float GetStrikeAimAlpha() const override { return bAimStrikeAtCrosshair ? CurrentStrikeAimAlpha : 0.0f; }

	/** Tip of the item: Tip socket, or the far end of the mesh from the hand */
	virtual bool GetStrikeTipLocation(FVector& OutLocation) const override;

	/** Rotates the item around the hand so its tip points along the swing arc (view space) */
	virtual void UpdateProceduralPose(const FVector& ViewOrigin, const FRotator& ViewRotation) override;
};
