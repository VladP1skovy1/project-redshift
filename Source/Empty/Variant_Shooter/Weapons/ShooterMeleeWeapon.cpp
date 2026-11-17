// Melee weapon implementation

#include "ShooterMeleeWeapon.h"
#include "ShooterMeleeTarget.h"
#include "ShooterWeaponHolder.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Controller.h"
#include "GameFramework/DamageType.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

AShooterMeleeWeapon::AShooterMeleeWeapon()
{
	// first person static mesh, follows the (possibly empty) skeletal first person mesh which is attached to the hand
	FirstPersonStaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("First Person Static Mesh"));
	FirstPersonStaticMesh->SetupAttachment(GetFirstPersonMesh());
	FirstPersonStaticMesh->SetCollisionProfileName(FName("NoCollision"));
	FirstPersonStaticMesh->SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType::FirstPerson);
	FirstPersonStaticMesh->bOnlyOwnerSee = true;
	FirstPersonStaticMesh->CastShadow = false;
	FirstPersonStaticMesh->LightingChannels.bChannel0 = false;
	FirstPersonStaticMesh->LightingChannels.bChannel1 = true;

	// third person static mesh
	ThirdPersonStaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Third Person Static Mesh"));
	ThirdPersonStaticMesh->SetupAttachment(GetThirdPersonMesh());
	ThirdPersonStaticMesh->SetCollisionProfileName(FName("NoCollision"));
	ThirdPersonStaticMesh->SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType::WorldSpaceRepresentation);
	ThirdPersonStaticMesh->bOwnerNoSee = true;

	// melee defaults: no ammo, no aiming, no flash, hold to keep swinging, number key 1
	MagazineSize = 0;
	bCanAimDownSights = false;
	bMuzzleFlash = false;
	bFullAuto = true;
	RefireRate = 0.6f;
	InventorySlot = 1;

	// the swing has its own animation, the camera only gets a punch when the swing connects (fully recovers)
	RecoilRecoveryFraction = 1.0f;
	RecoilRecoveryDelay = 0.05f;
	RecoilRecoverySpeed = 20.0f;
	WeaponKickOffset = FVector::ZeroVector;

	MeleeDamageType = UDamageType::StaticClass();
}

void AShooterMeleeWeapon::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	GetWorld()->GetTimerManager().ClearTimer(MeleeHitTimer);
}

void AShooterMeleeWeapon::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// put away / not swinging: idle pose
	if (IsHidden() || SwingTime < 0.0f)
	{
		SwingTime = -1.0f;
		CurrentSwingOffset = FVector::ZeroVector;
		CurrentStrikeAimAlpha = 0.0f;
		CurrentSwingPoseWeight = 0.0f;
		CurrentSwingStrikeBlend = 0.0f;

		if (IsHidden())
		{
			GetFirstPersonMesh()->SetRelativeRotation(FRotator::ZeroRotator);
		}

		return;
	}

	// hit stop freezes the swing for a moment
	if (HitStopRemaining > 0.0f)
	{
		HitStopRemaining -= DeltaSeconds;
	}
	else
	{
		SwingTime += DeltaSeconds;
	}

	const float T1 = SwingWindupTime;
	const float T2 = T1 + SwingStrikeTime;
	const float T3 = T2 + SwingHoldTime;
	const float T4 = T3 + SwingReturnTime;

	FVector Offset = FVector::ZeroVector;
	float PoseWeight = 0.0f;
	float StrikeBlend = 0.0f;

	if (SwingTime < T1)
	{
		// wind up: pull back and point the item up to the right
		const float Alpha = FMath::SmoothStep(0.0f, 1.0f, SwingTime / SwingWindupTime);
		Offset = FMath::Lerp(FVector::ZeroVector, SwingWindupOffset, Alpha);
		PoseWeight = Alpha;
		StrikeBlend = 0.0f;
	}
	else if (SwingTime < T2)
	{
		// strike: sweep the tip in an arc from the right into the crosshair, accelerating
		const float Alpha = FMath::InterpEaseIn(0.0f, 1.0f, (SwingTime - T1) / SwingStrikeTime, 1.6f);
		Offset = FMath::Lerp(SwingWindupOffset, SwingStrikeOffset, Alpha);
		PoseWeight = 1.0f;
		StrikeBlend = Alpha;
	}
	else if (SwingTime < T3)
	{
		// hold on the target
		Offset = SwingStrikeOffset;
		PoseWeight = 1.0f;
		StrikeBlend = 1.0f;
	}
	else if (SwingTime < T4)
	{
		// smoothly come back to the idle pose
		const float Alpha = FMath::SmoothStep(0.0f, 1.0f, (SwingTime - T3) / SwingReturnTime);
		Offset = FMath::Lerp(SwingStrikeOffset, FVector::ZeroVector, Alpha);
		PoseWeight = 1.0f - Alpha;
		StrikeBlend = 1.0f;
	}
	else
	{
		SwingTime = -1.0f;
	}

	CurrentSwingOffset = Offset;
	CurrentSwingPoseWeight = PoseWeight;
	CurrentSwingStrikeBlend = StrikeBlend;

	// the tip is pulled exactly onto the crosshair while the swing points at it
	CurrentStrikeAimAlpha = PoseWeight * StrikeBlend;
}

void AShooterMeleeWeapon::UpdateProceduralPose(const FVector& ViewOrigin, const FRotator& ViewRotation)
{
	USkeletalMeshComponent* Grip = GetFirstPersonMesh();

	// start from the pose the hand gives us
	Grip->SetRelativeRotation(FRotator::ZeroRotator);

	if (CurrentSwingPoseWeight <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	FVector TipLocation;

	if (!GetStrikeTipLocation(TipLocation))
	{
		return;
	}

	const FVector GripLocation = Grip->GetComponentLocation();
	const FVector TipDirection = (TipLocation - GripLocation).GetSafeNormal();

	if (TipDirection.IsNearlyZero())
	{
		return;
	}

	// wind up direction: relative to the view (up and to the right by default)
	const FVector WindupDirection = FRotator(ViewRotation.Pitch + SwingWindupDirection.Pitch, ViewRotation.Yaw + SwingWindupDirection.Yaw, 0.0f).Vector();

	// strike direction: from the hand to the point under the crosshair
	const FVector AimPoint = ViewOrigin + ViewRotation.Vector() * SwingStrikeReach;
	const FVector StrikeDirection = (AimPoint - GripLocation).GetSafeNormal();

	// arc between the two
	const FQuat ToWindup = FQuat::FindBetweenNormals(TipDirection, WindupDirection);
	const FQuat ToStrike = FQuat::FindBetweenNormals(TipDirection, StrikeDirection);
	const FQuat Full = FQuat::Slerp(ToWindup, ToStrike, CurrentSwingStrikeBlend);
	const FQuat Applied = FQuat::Slerp(FQuat::Identity, Full, CurrentSwingPoseWeight);

	// rotate the item around the hand
	Grip->SetWorldRotation(Applied * Grip->GetComponentQuat());
}

void AShooterMeleeWeapon::Fire()
{
	// the player may have let go of the button
	if (!bIsFiring)
	{
		return;
	}

	TimeOfLastShot = GetWorld()->GetTimeSeconds();

	// swing animation: montage if we have one, plus the procedural Half-Life style swing
	if (SwingMontage && WeaponOwner)
	{
		WeaponOwner->PlayWeaponMontage(SwingMontage, bSwingMontageFirstPerson, bSwingMontageThirdPerson);
	}

	SwingTime = 0.0f;
	HitStopRemaining = 0.0f;

	// check for a hit near the end of the strike
	const float HitTime = SwingWindupTime + SwingStrikeTime * 0.8f;
	GetWorld()->GetTimerManager().SetTimer(MeleeHitTimer, this, &AShooterMeleeWeapon::PerformMeleeHit, HitTime, false);

	// AI hearing
	if (PawnOwner)
	{
		MakeNoise(ShotLoudness * 0.3f, PawnOwner, PawnOwner->GetActorLocation(), ShotNoiseRange * 0.3f, NoiseOwnerTag);
	}

	// keep swinging while the button is held, otherwise allow the next click after the cooldown.
	// Never faster than the swing animation itself (+ hit stop)
	const float Cooldown = FMath::Max(RefireRate, GetSwingDuration() + HitStopTime);

	if (bFullAuto)
	{
		GetWorld()->GetTimerManager().SetTimer(RefireTimer, this, &AShooterMeleeWeapon::Fire, Cooldown, false);
	}
	else
	{
		GetWorld()->GetTimerManager().SetTimer(RefireTimer, this, &AShooterMeleeWeapon::FireCooldownExpired, Cooldown, false);
	}
}

bool AShooterMeleeWeapon::GetStrikeTipLocation(FVector& OutLocation) const
{
	// explicit tip socket on the static mesh
	if (FirstPersonStaticMesh->GetStaticMesh() && FirstPersonStaticMesh->DoesSocketExist(TipSocketName))
	{
		OutLocation = FirstPersonStaticMesh->GetSocketLocation(TipSocketName);
		return true;
	}

	// tip socket on the skeletal mesh
	if (GetFirstPersonMesh()->GetSkeletalMeshAsset() && GetFirstPersonMesh()->DoesSocketExist(TipSocketName))
	{
		OutLocation = GetFirstPersonMesh()->GetSocketLocation(TipSocketName);
		return true;
	}

	// automatic: the end of the static mesh's longest axis that is farthest from the hand
	if (UStaticMesh* Mesh = FirstPersonStaticMesh->GetStaticMesh())
	{
		const FBox LocalBox = Mesh->GetBoundingBox();
		const FVector Center = LocalBox.GetCenter();
		const FVector Extent = LocalBox.GetExtent();
		const FTransform& MeshTM = FirstPersonStaticMesh->GetComponentTransform();

		// pick the longest axis AFTER scaling (a cylinder stretched into a stick is long along Z only once scaled)
		const FVector AxisX = FVector::ForwardVector * Extent.X;
		const FVector AxisY = FVector::RightVector * Extent.Y;
		const FVector AxisZ = FVector::UpVector * Extent.Z;

		const float LenX = MeshTM.TransformVector(AxisX).Size();
		const float LenY = MeshTM.TransformVector(AxisY).Size();
		const float LenZ = MeshTM.TransformVector(AxisZ).Size();

		FVector Axis = AxisX;

		if (LenY > LenX && LenY >= LenZ)
		{
			Axis = AxisY;
		}
		else if (LenZ > LenX && LenZ > LenY)
		{
			Axis = AxisZ;
		}

		const FVector EndA = MeshTM.TransformPosition(Center + Axis);
		const FVector EndB = MeshTM.TransformPosition(Center - Axis);

		// the hand is where the first person skeletal mesh is attached
		const FVector Hand = GetFirstPersonMesh()->GetComponentLocation();

		OutLocation = FVector::DistSquared(EndA, Hand) > FVector::DistSquared(EndB, Hand) ? EndA : EndB;
		return true;
	}

	return false;
}

void AShooterMeleeWeapon::PerformMeleeHit()
{
	if (!PawnOwner || IsHidden())
	{
		return;
	}

	// sweep from the eyes / camera along the view
	FVector EyeLocation;
	FRotator EyeRotation;
	PawnOwner->GetActorEyesViewPoint(EyeLocation, EyeRotation);

	const FVector Direction = EyeRotation.Vector();
	const FVector End = EyeLocation + Direction * MeleeRange;

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(MeleeSwing), false, this);
	QueryParams.AddIgnoredActor(PawnOwner);

	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);
	ObjectParams.AddObjectTypesToQuery(ECC_WorldStatic);
	ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	ObjectParams.AddObjectTypesToQuery(ECC_PhysicsBody);
	ObjectParams.AddObjectTypesToQuery(ECC_Destructible);

	TArray<FHitResult> Hits;
	GetWorld()->SweepMultiByObjectType(Hits, EyeLocation, End, FQuat::Identity, ObjectParams, FCollisionShape::MakeSphere(MeleeRadius), QueryParams);

	AController* InstigatorController = PawnOwner->GetController();
	TSet<AActor*> AlreadyHit;
	bool bHitSomething = false;

	// hits come sorted by distance
	for (const FHitResult& Hit : Hits)
	{
		AActor* HitActor = Hit.GetActor();

		if (!HitActor || AlreadyHit.Contains(HitActor))
		{
			continue;
		}

		AlreadyHit.Add(HitActor);
		bHitSomething = true;

		// regular damage (characters, anything with TakeDamage / Event AnyDamage)
		UGameplayStatics::ApplyPointDamage(HitActor, MeleeDamage, Direction, Hit, InstigatorController, this, MeleeDamageType);

		// special reactions (trees, ore, breakables...)
		if (HitActor->Implements<UShooterMeleeTarget>())
		{
			IShooterMeleeTarget::Execute_ReceiveMeleeHit(HitActor, this, Hit, MeleeDamage, ToolType);
		}

		// push physics objects
		if (UPrimitiveComponent* HitComponent = Hit.GetComponent())
		{
			if (HitComponent->IsSimulatingPhysics())
			{
				HitComponent->AddImpulseAtLocation(Direction * HitImpulse, Hit.ImpactPoint);
			}
		}

		// sounds / particles
		BP_OnMeleeHit(Hit, HitActor);

		// a wall or one enemy stops the swing unless we allow hitting several targets
		if (!bHitMultipleTargets || !HitActor->IsA<APawn>())
		{
			break;
		}
	}

	if (bHitSomething)
	{
		// impact feel: freeze the swing for a moment and punch the camera
		HitStopRemaining = HitStopTime;

		if (WeaponOwner && !HitCameraPunch.IsNearlyZero())
		{
			WeaponOwner->ApplyWeaponRecoil(HitCameraPunch, this);
		}
	}
	else
	{
		BP_OnMeleeMiss();
	}
}
