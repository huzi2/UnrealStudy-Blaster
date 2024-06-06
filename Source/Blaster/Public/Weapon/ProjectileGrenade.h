// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon/Projectile.h"
#include "ProjectileGrenade.generated.h"

/**
 * 바닥이나 벽에서 튕기는 유탄 발사체 클래스
 */
UCLASS()
class BLASTER_API AProjectileGrenade : public AProjectile
{
	GENERATED_BODY()
	
private:
	AProjectileGrenade();

private:
	virtual void BeginPlay() final;
	virtual void Destroyed() final;
	
private:
	void OnBounce(const FHitResult& ImpactResult, const FVector& ImpactVelocity);

private:
	// 바닥이나 벽에 튕길 때 사운드
	UPROPERTY(EditAnywhere)
	TObjectPtr<USoundCue> BounceSound;
};
