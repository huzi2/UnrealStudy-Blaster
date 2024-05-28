// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Pickup.generated.h"

class USphereComponent;
class USoundCue;
class UNiagaraComponent;
class UNiagaraSystem;

/**
 * 아이템 기본 클래스
 */
UCLASS()
class BLASTER_API APickup : public AActor
{
	GENERATED_BODY()
	
protected:
	APickup();

private:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void Destroyed() override;

protected:
	// 아이템과 플레이어가 충돌했을 때 각 아이템의 역할 수행
	UFUNCTION()
	virtual void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

private:
	// 충돌 구체에 아이템 획득 함수 바인드. 생성되자마자 아이템이 먹히는 걸 막기 위해 따로 바인드함
	void BindOverlapTimerFinished();

private:
	// 충돌 구체
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USphereComponent> OverlapShpere;

	// 아이템 메시
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> PickupMesh;

	// 아이템 회전 속도
	UPROPERTY(EditAnywhere)
	float BaseTurnRate = 45.f;

	// 픽업 사운드
	UPROPERTY(EditAnywhere)
	TObjectPtr<USoundCue> PickupSound;

	// 픽업 이펙트
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UNiagaraComponent> PickupEffectComponent;
	UPROPERTY(EditAnywhere)
	TObjectPtr<UNiagaraSystem> PickupEffect;

	// 충돌 함수를 바인드할 타이머. 아이템 생성되자마자 먹는 걸 막기 위해 아주 약간의 시간을 가지는 타이머
	FTimerHandle BindOverlapTimer;
	float BindOverlapTime = 0.25f;
};
