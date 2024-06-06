// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon/HitScanWeapon.h"
#include "Shotgun.generated.h"

/**
 * �� ���ݿ� ���� ��Ʈ��ĵ�� ����ϴ� ���� Ŭ����
 */
UCLASS()
class BLASTER_API AShotgun : public AHitScanWeapon
{
	GENERATED_BODY()

public:
	// ���� �߻�
	void FireShotgun(const TArray<FVector_NetQuantize>& HitTargets);
	
	// ���� ��Ʈ��ĵ�� Ÿ�������� Ȯ��
	void ShotgunTraceEndWithScatter(const FVector& HitTarget, TArray<FVector_NetQuantize>& HitTargets) const;

private:
	// �� ���� ����(��Ʈ��ĵ) Ƚ��
	UPROPERTY(EditAnywhere, Category = "Weapon Scatter")
	uint32 NumberOfPellets = 10;
};
