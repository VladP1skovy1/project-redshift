// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ShooterBulletCounterUI.generated.h"

/**
 *  Simple bullet counter UI widget for a first person shooter game
 */
UCLASS(abstract)
class EMPTY_API UShooterBulletCounterUI : public UUserWidget
{
	GENERATED_BODY()
	
public:

	/** Allows Blueprint to update sub-widgets with the new bullet count */
	UFUNCTION(BlueprintImplementableEvent, Category="Shooter", meta=(DisplayName = "UpdateBulletCounter"))
	void BP_UpdateBulletCounter(int32 MagazineSize, int32 BulletCount);

	/** Ammo as numbers for a text HUD. ReserveAmmo is -1 when the reserve is infinite */
	UFUNCTION(BlueprintImplementableEvent, Category="Shooter", meta=(DisplayName = "UpdateAmmoText"))
	void BP_UpdateAmmoText(int32 BulletCount, int32 MagazineSize, int32 ReserveAmmo);

	/** Called when the player starts / stops aiming down sights. Use it to hide the crosshair */
	UFUNCTION(BlueprintImplementableEvent, Category="Shooter", meta=(DisplayName = "AimDownSightsChanged"))
	void BP_AimDownSightsChanged(bool bAiming);

	/** Allows Blueprint to update sub-widgets with the new life total and play a damage effect on the HUD */
	UFUNCTION(BlueprintImplementableEvent, Category="Shooter", meta=(DisplayName = "Damaged"))
	void BP_Damaged(float LifePercent);
};
