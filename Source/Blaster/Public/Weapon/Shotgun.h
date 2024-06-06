// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon/HitScanWeapon.h"
#include "Shotgun.generated.h"

/**
 * 한 공격에 여러 히트스캔을 사용하는 샷건 클래스
 */
UCLASS()
class BLASTER_API AShotgun : public AHitScanWeapon
{
	GENERATED_BODY()

public:
	// 샷건 발사
	void FireShotgun(const TArray<FVector_NetQuantize>& HitTargets);
	
	// 샷건 히트스캔의 타격지점들 확인
	void ShotgunTraceEndWithScatter(const FVector& HitTarget, TArray<FVector_NetQuantize>& HitTargets) const;

private:
	// 한 번에 공격(히트스캔) 횟수
	UPROPERTY(EditAnywhere, Category = "Weapon Scatter")
	uint32 NumberOfPellets = 10;
};
