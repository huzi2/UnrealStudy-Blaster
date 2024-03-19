// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Projectile.generated.h"

class UBoxComponent;
class UProjectileMovementComponent;
class USoundCue;
class UNiagaraSystem;
class UNiagaraComponent;

UCLASS()
class BLASTER_API AProjectile : public AActor
{
	GENERATED_BODY()
	
protected:
	AProjectile();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void Destroyed() override;

protected:
	UFUNCTION()
	virtual void OnHit(UPrimitiveComponent* HItComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

public:
	FORCEINLINE float GetInitialSpeed() const { return InitialSpeed; }
	FORCEINLINE float GetDamage() const { return Damage; }
	FORCEINLINE void SetDamage(float WeaponDamage) { Damage = WeaponDamage; }
	FORCEINLINE void SetUseServerSideRewind(bool bServerSideRewind) { bUseServerSideRewind = bServerSideRewind; }
	FORCEINLINE void SetTraceStart(const FVector_NetQuantize& Start) { TraceStart = Start; }
	FORCEINLINE void SetInitialVelocity(const FVector_NetQuantize100& Velocity) { InitialVelocity = Velocity; }

protected:
	void SpawnTrailSystem();
	void StartDestroyTimer();
	void ExplodeDamage();

private:
	void DestroyTimerFinished();

protected:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> ProjectileMesh;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UProjectileMovementComponent> ProjectileMovementComponent;

	UPROPERTY(EditAnywhere)
	TObjectPtr<UBoxComponent> CollisionBox;

	UPROPERTY(EditAnywhere)
	TObjectPtr<UParticleSystem> ImpactParticles;

	UPROPERTY(EditAnywhere)
	TObjectPtr<USoundCue> ImpactSound;

	UPROPERTY()
	TObjectPtr<UNiagaraComponent> TrailSystemComponent;

	UPROPERTY(EditAnywhere, Category = "Server Side Rewind")
	bool bUseServerSideRewind;
	
	UPROPERTY(EditAnywhere)
	float InitialSpeed;

private:
	UPROPERTY(EditAnywhere)
	TObjectPtr<UParticleSystem> Tracer;
	
	UPROPERTY(EditAnywhere)
	TObjectPtr<UNiagaraSystem> TrailSystem;

	UPROPERTY()
	TObjectPtr<UParticleSystemComponent> TracerComponent;

	UPROPERTY(EditAnywhere)
	float DestroyTime;

	UPROPERTY(EditAnywhere)
	float DamageInnerRadius;

	UPROPERTY(EditAnywhere)
	float DamageOuterRadius;

protected:
	float Damage;

	// 서버 되감기로 발사체 예측에 사용되는 변수들
	FVector_NetQuantize TraceStart;
	FVector_NetQuantize100 InitialVelocity;

private:
	FTimerHandle DestroyTimer;
};
