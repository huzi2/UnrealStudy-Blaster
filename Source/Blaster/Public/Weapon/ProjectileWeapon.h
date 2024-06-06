// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon/Weapon.h"
#include "ProjectileWeapon.generated.h"

class AProjectile;

/**
 * 발사체를 사용할 무기 클래스
 */
UCLASS()
class BLASTER_API AProjectileWeapon : public AWeapon
{
	GENERATED_BODY()
	
private:
	virtual void Fire(const FVector& HitTarget) final;

private:
	// 사용할 발사체 클래스
	UPROPERTY(EditAnywhere)
	TSubclassOf<AProjectile> ProjectileClass;
	// 서버 되감기를 할 경우 사용할 발사체 클래스
	UPROPERTY(EditAnywhere)
	TSubclassOf<AProjectile> ServerSideRewindProjectileClass;
};
