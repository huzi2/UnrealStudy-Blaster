// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PickupSpawnPoint.generated.h"

class APickup;

/**
 * ������ ���� ���� Ŭ����
 */
UCLASS()
class BLASTER_API APickupSpawnPoint : public AActor
{
	GENERATED_BODY()
	
private:
	APickupSpawnPoint();

private:
	virtual void BeginPlay() final;

private:
	// ���� Ÿ�̸� ����
	UFUNCTION()
	void StartSpawnPickupTimer(AActor* DestroyedActor);
	// ���� Ÿ�̸Ӱ� ������ ������ ����
	void SpawnPickupTimerFinished();
	// �������� �����ϰ� ����
	void SpawnPickup();

private:
	// ������ ������ Ŭ������
	UPROPERTY(EditAnywhere)
	TArray<TSubclassOf<APickup>> PickupClasses;

	// ���� �ð�
	UPROPERTY(EditAnywhere)
	float SpawnPickupTimeMin;
	UPROPERTY(EditAnywhere)
	float SpawnPickupTimeMax;

	// ������ ������ Ŭ����
	UPROPERTY()
	TObjectPtr<APickup> SpawnedPickup;

private:
	// ������ ������Ǵ� �ð� Ÿ�̸�
	FTimerHandle SpawnPickupTimer;
};
