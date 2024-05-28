// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Pickups/Pickup.h"
#include "HealthPickup.generated.h"

/**
 * 체력 회복 아이템 클래스
 */
UCLASS()
class BLASTER_API AHealthPickup : public APickup
{
	GENERATED_BODY()

private:
	virtual void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) final;

private:
	// 체력 회복량
	UPROPERTY(EditAnywhere)
	float HealAmount = 100.f;
	// 체력 회복에 걸리는 시간
	UPROPERTY(EditAnywhere)
	float HealingTime = 5.f;
};
