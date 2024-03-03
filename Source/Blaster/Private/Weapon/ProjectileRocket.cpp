// Fill out your copyright notice in the Description page of Project Settings.

#include "Weapon/ProjectileRocket.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraComponent.h"
#include "NiagaraSystemInstance.h"
#include "Sound/SoundCue.h"
#include "Components/BoxComponent.h"
#include "Components/AudioComponent.h"
#include "Weapon/RocketMovementComponent.h"

AProjectileRocket::AProjectileRocket()
{
	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RocketMesh"));
	ProjectileMesh->SetupAttachment(GetRootComponent());
	ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	RocketMovementComponent = CreateDefaultSubobject<URocketMovementComponent>(TEXT("RocketMovementComponent"));
	RocketMovementComponent->bRotationFollowsVelocity = true;
	RocketMovementComponent->SetIsReplicated(true);
}

void AProjectileRocket::BeginPlay()
{
	Super::BeginPlay();

	// Ŭ�󿡼��� Ÿ�̸Ӹ� �̿��� ������ ����Ʈ ����� �ؾ��ϱ� ������ ���⼭�� Ŭ�󿡵� �浹ó���� �Ѵ�.
	if (!HasAuthority() && CollisionBox)
	{
		CollisionBox->OnComponentHit.AddDynamic(this, &ThisClass::OnHit);
	}

	SpawnTrailSystem();

	if (ProjectileLoop && LoopingSoundAttenuation)
	{
		ProjectileLoopComponent = UGameplayStatics::SpawnSoundAttached(ProjectileLoop, GetRootComponent(), FName(), GetActorLocation(), EAttachLocation::KeepWorldPosition, false, 1.f, 1.f, 0.f, LoopingSoundAttenuation, nullptr, false);
	}
}

void AProjectileRocket::Destroyed()
{
	// ���Ͽ����� �ı��� �� ����� ����Ʈ�� ������� �ʱ� ���� �ƹ��͵� ����
}

void AProjectileRocket::OnHit(UPrimitiveComponent* HItComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	// �߻����ڸ��� �߻��ڰ� ���� �ʵ����Ѵ�. ������ ������ ���� �߻��ڵ� �´´�.
	if (OtherActor == GetOwner()) return;

	ExplodeDamage();

	// ���� ��� �޽��� ����
	if (ProjectileMesh)
	{
		ProjectileMesh->SetVisibility(false);
	}

	if (CollisionBox)
	{
		CollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	// ����Ʈ ������ ����
	if (TrailSystemComponent && TrailSystemComponent->GetSystemInstanceController())
	{
		TrailSystemComponent->GetSystemInstance()->Deactivate();
	}

	if (ProjectileLoopComponent && ProjectileLoopComponent->IsPlaying())
	{
		ProjectileLoopComponent->Stop();
	}

	// ���� �ı��Ǹ鼭 ���Դ� �ǵ� ������ �浹�ϰ� ���� �ð� ���� ���ŵǱ⿡ �浹���ڸ��� ���
	if (ImpactParticles)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactParticles, GetActorTransform());
	}

	if (ImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, GetActorLocation());
	}

	// ������ ����Ʈ�� ���尡 ��Ʈ ���Ŀ��� ��� ���;��ϹǷ� ��ü ���Ÿ� ���� �̷�
	StartDestroyTimer();
}
