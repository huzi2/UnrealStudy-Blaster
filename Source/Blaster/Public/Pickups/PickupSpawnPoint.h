// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PickupSpawnPoint.generated.h"

class APickup;

UCLASS()
class BLASTER_API APickupSpawnPoint : public AActor
{
	GENERATED_BODY()
	
private:
	APickupSpawnPoint();

private:
	virtual void BeginPlay() final;

private:
	UFUNCTION()
	void StartSpawnPickupTimer(AActor* DestroyedActor);

private:
	void SpawnPickupTimerFinished();
	void SpawnPickup();

private:
	UPROPERTY(EditAnywhere)
	TArray<TSubclassOf<APickup>> PickupClasses;

	UPROPERTY(EditAnywhere)
	float SpawnPickupTimeMin;

	UPROPERTY(EditAnywhere)
	float SpawnPickupTimeMax;

	UPROPERTY()
	TObjectPtr<APickup> SpawnedPickup;

private:
	FTimerHandle SpawnPickupTimer;
};
