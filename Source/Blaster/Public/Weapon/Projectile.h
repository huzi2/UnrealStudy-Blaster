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

/**
 * 발사체 클래스
 */
UCLASS()
class BLASTER_API AProjectile : public AActor
{
	GENERATED_BODY()
	
protected:
	AProjectile();

protected:
	virtual void BeginPlay() override;
	virtual void Destroyed() override;

protected:
	UFUNCTION()
	virtual void OnHit(UPrimitiveComponent* HItComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

public:
	FORCEINLINE float GetInitialSpeed() const { return InitialSpeed; }
	FORCEINLINE float GetDamage() const { return Damage; }
	FORCEINLINE float GetHeadShotDamage() const { return HeadShotDamage; }
	FORCEINLINE void SetDamage(float WeaponDamage) { Damage = WeaponDamage; }
	FORCEINLINE void SetHeadShotDamage(float WeaponHeadShotDamage) { HeadShotDamage = WeaponHeadShotDamage; }
	FORCEINLINE void SetUseServerSideRewind(bool bServerSideRewind) { bUseServerSideRewind = bServerSideRewind; }
	FORCEINLINE void SetTraceStart(const FVector_NetQuantize& Start) { TraceStart = Start; }
	FORCEINLINE void SetInitialVelocity(const FVector_NetQuantize100& Velocity) { InitialVelocity = Velocity; }

protected:
	// 발사체 따라가는 트레일 이펙트 생성
	void SpawnTrailSystem();
	
	// 폭발형 무기에서 범위 데미지
	void ExplodeDamage();

	// 자동 발사체 제거 타이머 시작
	void StartDestroyTimer();

private:
	// 제거 타이머가 끝나면 발사체 제거
	void DestroyTimerFinished();

protected:
	// 무기 메쉬
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> ProjectileMesh;

	// 충돌 박스
	UPROPERTY(EditAnywhere)
	TObjectPtr<UBoxComponent> CollisionBox;

	// 발사체 이동 컴포넌트
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UProjectileMovementComponent> ProjectileMovementComponent;
	// 발사체 속도
	UPROPERTY(EditAnywhere)
	float InitialSpeed = 15000.f;

	// 트레일 이펙트 시스템 컴포넌트
	UPROPERTY()
	TObjectPtr<UNiagaraComponent> TrailSystemComponent;

	// 타격 이펙트
	UPROPERTY(EditAnywhere)
	TObjectPtr<UParticleSystem> ImpactParticles;
	// 타격 사운드
	UPROPERTY(EditAnywhere)
	TObjectPtr<USoundCue> ImpactSound;

	// 발사체의 서버 되감기 사용 유무
	UPROPERTY(EditAnywhere, Category = "Server Side Rewind")
	bool bUseServerSideRewind = false;

protected:
	// 데미지
	float Damage = 20.f;
	float HeadShotDamage = 40.f;

	// 서버 되감기로 발사체 예측에 사용되는 변수들
	FVector_NetQuantize TraceStart;
	FVector_NetQuantize100 InitialVelocity;

private:
	// 발사체 이펙트
	UPROPERTY(EditAnywhere)
	TObjectPtr<UParticleSystem> Tracer;
	UPROPERTY()
	TObjectPtr<UParticleSystemComponent> TracerComponent;

	// 트레일 이펙트
	UPROPERTY(EditAnywhere)
	TObjectPtr<UNiagaraSystem> TrailSystem;

	// 데미지 범위
	UPROPERTY(EditAnywhere)
	float DamageInnerRadius = 200.f;
	UPROPERTY(EditAnywhere)
	float DamageOuterRadius = 500.f;

	// 자동 제거 시간
	UPROPERTY(EditAnywhere)
	float DestroyTime = 3.f;
	// 발사체가 충돌 안해서 제거되지 않았을 때 자동 제거할 때 사용할 타이머
	FTimerHandle DestroyTimer;
};
