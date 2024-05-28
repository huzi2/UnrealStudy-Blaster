// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Pickups/Pickup.h"
#include "Weapon/WeaponTypes.h"
#include "AmmoPickup.generated.h"

/**
 * ź�� ������ Ŭ����
 */
UCLASS()
class BLASTER_API AAmmoPickup : public APickup
{
	GENERATED_BODY()

private:
	virtual void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) final;

private:
	// ä�� �� ź�෮
	UPROPERTY(EditAnywhere)
	int32 AmmoAmount = 30;
	// ź�� ����Ÿ��
	UPROPERTY(EditAnywhere)
	EWeaponType WeaponType;
};
