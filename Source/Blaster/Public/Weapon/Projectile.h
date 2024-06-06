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
 * �߻�ü Ŭ����
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
	// �߻�ü ���󰡴� Ʈ���� ����Ʈ ����
	void SpawnTrailSystem();
	
	// ������ ���⿡�� ���� ������
	void ExplodeDamage();

	// �ڵ� �߻�ü ���� Ÿ�̸� ����
	void StartDestroyTimer();

private:
	// ���� Ÿ�̸Ӱ� ������ �߻�ü ����
	void DestroyTimerFinished();

protected:
	// ���� �޽�
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> ProjectileMesh;

	// �浹 �ڽ�
	UPROPERTY(EditAnywhere)
	TObjectPtr<UBoxComponent> CollisionBox;

	// �߻�ü �̵� ������Ʈ
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UProjectileMovementComponent> ProjectileMovementComponent;
	// �߻�ü �ӵ�
	UPROPERTY(EditAnywhere)
	float InitialSpeed = 15000.f;

	// Ʈ���� ����Ʈ �ý��� ������Ʈ
	UPROPERTY()
	TObjectPtr<UNiagaraComponent> TrailSystemComponent;

	// Ÿ�� ����Ʈ
	UPROPERTY(EditAnywhere)
	TObjectPtr<UParticleSystem> ImpactParticles;
	// Ÿ�� ����
	UPROPERTY(EditAnywhere)
	TObjectPtr<USoundCue> ImpactSound;

	// �߻�ü�� ���� �ǰ��� ��� ����
	UPROPERTY(EditAnywhere, Category = "Server Side Rewind")
	bool bUseServerSideRewind = false;

protected:
	// ������
	float Damage = 20.f;
	float HeadShotDamage = 40.f;

	// ���� �ǰ���� �߻�ü ������ ���Ǵ� ������
	FVector_NetQuantize TraceStart;
	FVector_NetQuantize100 InitialVelocity;

private:
	// �߻�ü ����Ʈ
	UPROPERTY(EditAnywhere)
	TObjectPtr<UParticleSystem> Tracer;
	UPROPERTY()
	TObjectPtr<UParticleSystemComponent> TracerComponent;

	// Ʈ���� ����Ʈ
	UPROPERTY(EditAnywhere)
	TObjectPtr<UNiagaraSystem> TrailSystem;

	// ������ ����
	UPROPERTY(EditAnywhere)
	float DamageInnerRadius = 200.f;
	UPROPERTY(EditAnywhere)
	float DamageOuterRadius = 500.f;

	// �ڵ� ���� �ð�
	UPROPERTY(EditAnywhere)
	float DestroyTime = 3.f;
	// �߻�ü�� �浹 ���ؼ� ���ŵ��� �ʾ��� �� �ڵ� ������ �� ����� Ÿ�̸�
	FTimerHandle DestroyTimer;
};
