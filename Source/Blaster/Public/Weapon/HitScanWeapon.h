// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon/Weapon.h"
#include "HitScanWeapon.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API AHitScanWeapon : public AWeapon
{
	GENERATED_BODY()
	
protected:
	AHitScanWeapon();

private:
	virtual void Fire(const FVector& HitTarget) override;

protected:
	void WeaponTraceHit(const FVector& TraceStart, const FVector& HitTarget, FHitResult& OutHit);

private:
	FVector TraceEndWithScatter(const FVector& TraceStart, const FVector& HitTarget) const;

protected:
	UPROPERTY(EditAnywhere, Category = "Hit Scan Weapon")
	float Damage;

	UPROPERTY(EditAnywhere, Category = "Hit Scan Weapon")
	TObjectPtr<UParticleSystem> ImpactParticles;

	UPROPERTY(EditAnywhere, Category = "Hit Scan Weapon")
	TObjectPtr<USoundCue> HitSound;

private:
	UPROPERTY(EditAnywhere, Category = "Hit Scan Weapon")
	TObjectPtr<UParticleSystem> BeamParticles;

	UPROPERTY(EditAnywhere, Category = "Hit Scan Weapon")
	TObjectPtr<UParticleSystem> MuzzleFlash;

	UPROPERTY(EditAnywhere, Category = "Hit Scan Weapon")
	TObjectPtr<USoundCue> FireSound;

	UPROPERTY(EditAnywhere, Category = "Weapon Scatter")
	float DistanceToSphere;

	UPROPERTY(EditAnywhere, Category = "Weapon Scatter")
	float SphereRadius;

	UPROPERTY(EditAnywhere, Category = "Weapon Scatter")
	bool bUseScatter;
};
