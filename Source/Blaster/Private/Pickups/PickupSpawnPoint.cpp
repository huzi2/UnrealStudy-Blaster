// Fill out your copyright notice in the Description page of Project Settings.

#include "Pickups/PickupSpawnPoint.h"
#include "Pickups/Pickup.h"

APickupSpawnPoint::APickupSpawnPoint()
{
	PrimaryActorTick.bCanEverTick = false;

	// 픽업 생성을 서버에서만 하므로 레플리케이션 필요
	bReplicates = true;
}

void APickupSpawnPoint::BeginPlay()
{
	Super::BeginPlay();

	StartSpawnPickupTimer(nullptr);
}

void APickupSpawnPoint::StartSpawnPickupTimer(AActor* DestroyedActor)
{
	const float SpawnTime = FMath::FRandRange(SpawnPickupTimeMin, SpawnPickupTimeMax);
	GetWorldTimerManager().SetTimer(SpawnPickupTimer, this, &ThisClass::SpawnPickupTimerFinished, SpawnTime);
}

void APickupSpawnPoint::SpawnPickupTimerFinished()
{
	// 픽업 생성은 서버에서만
	if (HasAuthority())
	{
		SpawnPickup();
	}
}

void APickupSpawnPoint::SpawnPickup()
{
	if (!GetWorld()) return;

	const int32 NumPickupClasses = PickupClasses.Num();
	if (NumPickupClasses > 0)
	{
		const int32 Selection = FMath::RandRange(0, NumPickupClasses - 1);
		SpawnedPickup = GetWorld()->SpawnActor<APickup>(PickupClasses[Selection], GetActorTransform());

		// 스폰했던 픽업 액터가 파괴(누군가 먹으면)되면 다시 타이머 실행
		if (HasAuthority() && SpawnedPickup)
		{
			SpawnedPickup->OnDestroyed.AddDynamic(this, &ThisClass::StartSpawnPickupTimer);
		}
	}
}
