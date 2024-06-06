// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon/Weapon.h"
#include "HitScanWeapon.generated.h"

/**
 * �������� �� ���� �浹üũ�ϴ� ���� Ŭ����
 */
UCLASS()
class BLASTER_API AHitScanWeapon : public AWeapon
{
	GENERATED_BODY()

private:
	virtual void Fire(const FVector& HitTarget) override;

protected:
	// �浹 üũ
	void WeaponTraceHit(const FVector& TraceStart, const FVector& HitTarget, FHitResult& OutHit);

protected:
	// �浹 �� ����Ʈ
	UPROPERTY(EditAnywhere, Category = "Hit Scan Weapon")
	TObjectPtr<UParticleSystem> ImpactParticles;
	// �浹 ����
	UPROPERTY(EditAnywhere, Category = "Hit Scan Weapon")
	TObjectPtr<USoundCue> HitSound;

private:
	// ���� ����Ʈ
	UPROPERTY(EditAnywhere, Category = "Hit Scan Weapon")
	TObjectPtr<UParticleSystem> BeamParticles;
	// ���� ����Ʈ
	UPROPERTY(EditAnywhere, Category = "Hit Scan Weapon")
	TObjectPtr<UParticleSystem> MuzzleFlash;
	// �߻� ����
	UPROPERTY(EditAnywhere, Category = "Hit Scan Weapon")
	TObjectPtr<USoundCue> FireSound;
};
