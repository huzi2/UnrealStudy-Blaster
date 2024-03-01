// Fill out your copyright notice in the Description page of Project Settings.

#include "Weapon/Projectile.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"
#include "Character/BlasterCharacter.h"
#include "Blaster/Blaster.h"

AProjectile::AProjectile()
	: Damage(20.f)
{
	PrimaryActorTick.bCanEverTick = true;
	// ������ ���������� �����ϰ� Ŭ����� ���纻�� ����
	bReplicates = true;
	// �̰� ���ϴϱ� �̵��� ����ȵǴ��� �ϴ� ����
	SetReplicateMovement(true);

	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	SetRootComponent(CollisionBox);
	CollisionBox->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	CollisionBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);
	CollisionBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Block);
	// ĳ������ ĸ���� �ƴ� �޽ø� Ÿ������ �ϱ����� ���� Ŀ���� ������Ʈ ä��
	CollisionBox->SetCollisionResponseToChannel(ECC_SkeletalMesh, ECollisionResponse::ECR_Block);
}

void AProjectile::BeginPlay()
{
	Super::BeginPlay();
	
	if (Tracer)
	{
		TracerComponent = UGameplayStatics::SpawnEmitterAttached(Tracer, CollisionBox, FName(), GetActorLocation(), GetActorRotation(), EAttachLocation::KeepWorldPosition);
	}

	// �浹 ó�� �̺�Ʈ�� ���������� Ȯ��
	if (HasAuthority() && CollisionBox)
	{
		CollisionBox->OnComponentHit.AddDynamic(this, &ThisClass::OnHit);
	}
}

void AProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AProjectile::Destroyed()
{
	Super::Destroyed();

	// ���� ���Ű� ȣ��Ǹ鼭 ��ε�ĳ��Ʈ�� ���� ��� Ŭ���̾�Ʈ�� Destroyed()�� ȣ���Ѵ�.
	// �׷��� ���⼭ ����Ʈ�� ���带 ����ϸ� ��� Ŭ���̾�Ʈ�� Ȯ���� �� ����
	if (ImpactParticles)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactParticles, GetActorTransform());
	}

	if (ImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, GetActorLocation());
	}
}

void AProjectile::OnHit(UPrimitiveComponent* HItComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	// ���������� ���� ���Ÿ� ȣ���Ѵ�.
	Destroy();
}
