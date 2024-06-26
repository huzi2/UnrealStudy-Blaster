// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon/Projectile.h"
#include "ProjectileGrenade.generated.h"

/**
 * �ٴ��̳� ������ ƨ��� ��ź �߻�ü Ŭ����
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
	// �ٴ��̳� ���� ƨ�� �� ����
	UPROPERTY(EditAnywhere)
	TObjectPtr<USoundCue> BounceSound;
};
