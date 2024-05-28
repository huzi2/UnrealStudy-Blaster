// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Pickups/Pickup.h"
#include "ShieldPickup.generated.h"

/**
 * �ǵ� ȸ�� ������ Ŭ����
 */
UCLASS()
class BLASTER_API AShieldPickup : public APickup
{
	GENERATED_BODY()

private:
	virtual void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) final;

private:
	// �ǵ� ȸ����
	UPROPERTY(EditAnywhere)
	float ShieldReplenishAmount = 100.f;
	// �ǵ� ȸ���� �ɸ��� �ð�
	UPROPERTY(EditAnywhere)
	float ShieldReplenishTime = 5.f;
};
