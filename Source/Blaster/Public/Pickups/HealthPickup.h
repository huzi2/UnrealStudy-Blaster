// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Pickups/Pickup.h"
#include "HealthPickup.generated.h"

/**
 * ü�� ȸ�� ������ Ŭ����
 */
UCLASS()
class BLASTER_API AHealthPickup : public APickup
{
	GENERATED_BODY()

private:
	virtual void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) final;

private:
	// ü�� ȸ����
	UPROPERTY(EditAnywhere)
	float HealAmount = 100.f;
	// ü�� ȸ���� �ɸ��� �ð�
	UPROPERTY(EditAnywhere)
	float HealingTime = 5.f;
};
