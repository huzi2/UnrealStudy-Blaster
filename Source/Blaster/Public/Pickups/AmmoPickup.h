// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Pickups/Pickup.h"
#include "Weapon/WeaponTypes.h"
#include "AmmoPickup.generated.h"

/**
 * 탄약 아이템 클래스
 */
UCLASS()
class BLASTER_API AAmmoPickup : public APickup
{
	GENERATED_BODY()

private:
	virtual void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) final;

private:
	// 채워 줄 탄약량
	UPROPERTY(EditAnywhere)
	int32 AmmoAmount = 30;
	// 탄약 무기타입
	UPROPERTY(EditAnywhere)
	EWeaponType WeaponType;
};
