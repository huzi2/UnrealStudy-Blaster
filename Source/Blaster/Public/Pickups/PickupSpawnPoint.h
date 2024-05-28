// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PickupSpawnPoint.generated.h"

class APickup;

/**
 * 아이템 생성 지점 클래스
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
	// 생성 타이머 시작
	UFUNCTION()
	void StartSpawnPickupTimer(AActor* DestroyedActor);
	// 생성 타이머가 끝나면 아이템 생성
	void SpawnPickupTimerFinished();
	// 아이템을 랜덤하게 생성
	void SpawnPickup();

private:
	// 생성할 아이템 클래스들
	UPROPERTY(EditAnywhere)
	TArray<TSubclassOf<APickup>> PickupClasses;

	// 생성 시간
	UPROPERTY(EditAnywhere)
	float SpawnPickupTimeMin;
	UPROPERTY(EditAnywhere)
	float SpawnPickupTimeMax;

	// 생성된 아이템 클래스
	UPROPERTY()
	TObjectPtr<APickup> SpawnedPickup;

private:
	// 아이템 재생성되는 시간 타이머
	FTimerHandle SpawnPickupTimer;
};
