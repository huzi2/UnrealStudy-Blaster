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
 * ������ �⺻ Ŭ����
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
	// �����۰� �÷��̾ �浹���� �� �� �������� ���� ����
	UFUNCTION()
	virtual void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

private:
	// �浹 ��ü�� ������ ȹ�� �Լ� ���ε�. �������ڸ��� �������� ������ �� ���� ���� ���� ���ε���
	void BindOverlapTimerFinished();

private:
	// �浹 ��ü
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USphereComponent> OverlapShpere;

	// ������ �޽�
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> PickupMesh;

	// ������ ȸ�� �ӵ�
	UPROPERTY(EditAnywhere)
	float BaseTurnRate = 45.f;

	// �Ⱦ� ����
	UPROPERTY(EditAnywhere)
	TObjectPtr<USoundCue> PickupSound;

	// �Ⱦ� ����Ʈ
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UNiagaraComponent> PickupEffectComponent;
	UPROPERTY(EditAnywhere)
	TObjectPtr<UNiagaraSystem> PickupEffect;

	// �浹 �Լ��� ���ε��� Ÿ�̸�. ������ �������ڸ��� �Դ� �� ���� ���� ���� �ణ�� �ð��� ������ Ÿ�̸�
	FTimerHandle BindOverlapTimer;
	float BindOverlapTime = 0.25f;
};
