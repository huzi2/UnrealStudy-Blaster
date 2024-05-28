// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Pickups/Pickup.h"
#include "JumpPickup.generated.h"

/**
 * 점프 버프 아이템 클래스
 */
UCLASS()
class BLASTER_API AJumpPickup : public APickup
{
	GENERATED_BODY()

private:
	virtual void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) final;

private:
	// 점프 버프량
	UPROPERTY(EditAnywhere)
	float JumpZVelocityBuff = 4000.f;
	// 점프 버프 지속시간
	UPROPERTY(EditAnywhere)
	float JumpBuffTime = 30.f;
};
