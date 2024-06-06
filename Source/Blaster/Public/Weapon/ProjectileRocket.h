// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon/Projectile.h"
#include "ProjectileRocket.generated.h"

class URocketMovementComponent;

/**
 * 범위 공격을 하는 로켓 발사체 클래스
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
	// 로켓용 이동 컴포넌트
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<URocketMovementComponent> RocketMovementComponent;

	// 로켓이 날아갈 때 사용할 사운드
	UPROPERTY(EditAnywhere)
	TObjectPtr<USoundCue> ProjectileLoop;
	UPROPERTY(EditAnywhere)
	TObjectPtr<USoundAttenuation> LoopingSoundAttenuation;
	UPROPERTY()
	TObjectPtr<UAudioComponent> ProjectileLoopComponent;
};
