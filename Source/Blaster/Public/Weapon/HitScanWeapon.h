// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon/Weapon.h"
#include "HitScanWeapon.generated.h"

/**
 * 직선으로 선 쏴서 충돌체크하는 무기 클래스
 */
UCLASS()
class BLASTER_API AHitScanWeapon : public AWeapon
{
	GENERATED_BODY()

private:
	virtual void Fire(const FVector& HitTarget) override;

protected:
	// 충돌 체크
	void WeaponTraceHit(const FVector& TraceStart, const FVector& HitTarget, FHitResult& OutHit);

protected:
	// 충돌 시 이펙트
	UPROPERTY(EditAnywhere, Category = "Hit Scan Weapon")
	TObjectPtr<UParticleSystem> ImpactParticles;
	// 충돌 사운드
	UPROPERTY(EditAnywhere, Category = "Hit Scan Weapon")
	TObjectPtr<USoundCue> HitSound;

private:
	// 직선 이펙트
	UPROPERTY(EditAnywhere, Category = "Hit Scan Weapon")
	TObjectPtr<UParticleSystem> BeamParticles;
	// 머즐 이펙트
	UPROPERTY(EditAnywhere, Category = "Hit Scan Weapon")
	TObjectPtr<UParticleSystem> MuzzleFlash;
	// 발사 사운드
	UPROPERTY(EditAnywhere, Category = "Hit Scan Weapon")
	TObjectPtr<USoundCue> FireSound;
};
