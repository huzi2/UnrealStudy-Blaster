// Fill out your copyright notice in the Description page of Project Settings.

#include "Weapon/ProjectileGrenade.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"

AProjectileGrenade::AProjectileGrenade()
{
	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GrenadeMesh"));
	ProjectileMesh->SetupAttachment(GetRootComponent());
	ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));
	ProjectileMovementComponent->bRotationFollowsVelocity = true;
	ProjectileMovementComponent->SetIsReplicated(true);
	// 지형에 닿았을 때 통통 튀도록
	ProjectileMovementComponent->bShouldBounce = true;
}

void AProjectileGrenade::BeginPlay()
{
	// 기존 발사체는 충돌이벤트를 만드는데, 수류탄은 충돌하지 않고 물리작용만 할것임. 그래서 Super 버전을 호출하지 않는다.
	AActor::BeginPlay();

	SpawnTrailSystem();
	
	// 발사 후 시간있다가 터지는 형태
	StartDestroyTimer();

	// 발사체가 바운드됬을 때의 함수 바인드
	if (ProjectileMovementComponent)
	{
		ProjectileMovementComponent->OnProjectileBounce.AddDynamic(this, &ThisClass::OnBounce);
	}
}

void AProjectileGrenade::Destroyed()
{
	// 파괴될 때 범위 데미지
	ExplodeDamage();

	Super::Destroyed();
}

void AProjectileGrenade::OnBounce(const FHitResult& ImpactResult, const FVector& ImpactVelocity)
{
	if (BounceSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, BounceSound, GetActorLocation());
	}
}
