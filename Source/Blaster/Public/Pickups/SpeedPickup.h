// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Pickups/Pickup.h"
#include "SpeedPickup.generated.h"

/**
 * �̵��ӵ� ���� ������ Ŭ����
 */
UCLASS()
class BLASTER_API ASpeedPickup : public APickup
{
	GENERATED_BODY()

private:
	virtual void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) final;

private:
	// �� ���� �� �ӵ� ������
	UPROPERTY(EditAnywhere)
	float BaseSpeedBuff = 1600.f;
	// �ɾ��� �� �ӵ� ������
	UPROPERTY(EditAnywhere)
	float CrouchSpeedBuff = 850.f;
	// �ӵ� ���� ���ӽð�
	UPROPERTY(EditAnywhere)
	float SpeedBuffTime = 30.f;
};
