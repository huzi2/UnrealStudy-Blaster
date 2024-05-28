// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Pickups/Pickup.h"
#include "ShieldPickup.generated.h"

/**
 * 실드 회복 아이템 클래스
 */
UCLASS()
class BLASTER_API AShieldPickup : public APickup
{
	GENERATED_BODY()

private:
	virtual void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) final;

private:
	// 실드 회복량
	UPROPERTY(EditAnywhere)
	float ShieldReplenishAmount = 100.f;
	// 실드 회복에 걸리는 시간
	UPROPERTY(EditAnywhere)
	float ShieldReplenishTime = 5.f;
};
