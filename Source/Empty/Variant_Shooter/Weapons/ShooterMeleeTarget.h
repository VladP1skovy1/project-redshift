// Interface for things that react to melee hits in a special way (trees to chop, ore to mine, breakables...)

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ShooterMeleeTarget.generated.h"

class AShooterWeapon;

UINTERFACE(MinimalAPI, Blueprintable)
class UShooterMeleeTarget : public UInterface
{
	GENERATED_BODY()
};

/**
 *  Implement this (in C++ or in a Blueprint: Class Settings -> Interfaces) on any actor that should react to melee hits.
 *  Example: a tree checks ToolType == "Axe" and drops wood.
 *  Regular damage (TakeDamage / Event AnyDamage) is applied as well, this is just extra information.
 */
class EMPTY_API IShooterMeleeTarget
{
	GENERATED_BODY()

public:

	/** Called when a melee weapon hits this actor */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Melee")
	void ReceiveMeleeHit(AShooterWeapon* Weapon, const FHitResult& Hit, float Damage, FName ToolType);
};
