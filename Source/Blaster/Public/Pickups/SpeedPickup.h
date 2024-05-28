// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Pickups/Pickup.h"
#include "SpeedPickup.generated.h"

/**
 * 이동속도 버프 아이템 클래스
 */
UCLASS()
class BLASTER_API ASpeedPickup : public APickup
{
	GENERATED_BODY()

private:
	virtual void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) final;

private:
	// 서 있을 때 속도 버프량
	UPROPERTY(EditAnywhere)
	float BaseSpeedBuff = 1600.f;
	// 앉았을 때 속도 버프량
	UPROPERTY(EditAnywhere)
	float CrouchSpeedBuff = 850.f;
	// 속도 버프 지속시간
	UPROPERTY(EditAnywhere)
	float SpeedBuffTime = 30.f;
};
