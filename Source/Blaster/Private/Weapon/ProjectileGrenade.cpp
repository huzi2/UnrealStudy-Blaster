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
	// ������ ����� �� ���� Ƣ����
	ProjectileMovementComponent->bShouldBounce = true;
}

void AProjectileGrenade::BeginPlay()
{
	// ���� �߻�ü�� �浹�̺�Ʈ�� ����µ�, ����ź�� �浹���� �ʰ� �����ۿ븸 �Ұ���. �׷��� Super ������ ȣ������ �ʴ´�.
	AActor::BeginPlay();

	SpawnTrailSystem();
	
	// �߻� �� �ð��ִٰ� ������ ����
	StartDestroyTimer();

	// �߻�ü�� �ٿ����� ���� �Լ� ���ε�
	if (ProjectileMovementComponent)
	{
		ProjectileMovementComponent->OnProjectileBounce.AddDynamic(this, &ThisClass::OnBounce);
	}
}

void AProjectileGrenade::Destroyed()
{
	// �ı��� �� ���� ������
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
