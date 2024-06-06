// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon/Weapon.h"
#include "ProjectileWeapon.generated.h"

class AProjectile;

/**
 * �߻�ü�� ����� ���� Ŭ����
 */
UCLASS()
class BLASTER_API AProjectileWeapon : public AWeapon
{
	GENERATED_BODY()
	
private:
	virtual void Fire(const FVector& HitTarget) final;

private:
	// ����� �߻�ü Ŭ����
	UPROPERTY(EditAnywhere)
	TSubclassOf<AProjectile> ProjectileClass;
	// ���� �ǰ��⸦ �� ��� ����� �߻�ü Ŭ����
	UPROPERTY(EditAnywhere)
	TSubclassOf<AProjectile> ServerSideRewindProjectileClass;
};
