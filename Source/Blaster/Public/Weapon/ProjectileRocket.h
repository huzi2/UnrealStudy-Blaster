// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon/Projectile.h"
#include "ProjectileRocket.generated.h"

class URocketMovementComponent;

/**
 * ���� ������ �ϴ� ���� �߻�ü Ŭ����
 */
UCLASS()
class BLASTER_API AProjectileRocket : public AProjectile
{
	GENERATED_BODY()
	
private:
	AProjectileRocket();

private:
	virtual void BeginPlay() final;
	virtual void Destroyed() final;

private:
	virtual void OnHit(UPrimitiveComponent* HItComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit) final;

private:
	// ���Ͽ� �̵� ������Ʈ
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<URocketMovementComponent> RocketMovementComponent;

	// ������ ���ư� �� ����� ����
	UPROPERTY(EditAnywhere)
	TObjectPtr<USoundCue> ProjectileLoop;
	UPROPERTY(EditAnywhere)
	TObjectPtr<USoundAttenuation> LoopingSoundAttenuation;
	UPROPERTY()
	TObjectPtr<UAudioComponent> ProjectileLoopComponent;
};
