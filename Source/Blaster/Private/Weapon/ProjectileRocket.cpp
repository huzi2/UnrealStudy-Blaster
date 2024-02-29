// Fill out your copyright notice in the Description page of Project Settings.

#include "Weapon/ProjectileRocket.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "NiagaraSystemInstance.h"
#include "Sound/SoundCue.h"
#include "Components/BoxComponent.h"
#include "Components/AudioComponent.h"
#include "Weapon/RocketMovementComponent.h"

AProjectileRocket::AProjectileRocket()
	: DestroyTime(3.f)
{
	RocketMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RocketMesh"));
	RocketMesh->SetupAttachment(GetRootComponent());
	RocketMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

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

	if (TrailSystem)
	{
		TrailSystemComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(TrailSystem, GetRootComponent(), FName(), GetActorLocation(), GetActorRotation(), EAttachLocation::KeepWorldPosition, false);
	}

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
	// �߻��ڴ� �ȸµ���
	if (OtherActor == GetOwner()) return;

	if (HasAuthority())
	{
		if (APawn* FiringPawn = GetInstigator())
		{
			if (AController* FiringController = FiringPawn->GetController())
			{
				// �Ÿ��� ���� �ٸ� �������� ������ ������ �������� ����
				UGameplayStatics::ApplyRadialDamageWithFalloff(this, Damage, 10.f, GetActorLocation(), 200.f, 500.f, 1.f, UDamageType::StaticClass(), TArray<AActor*>(), this, FiringController);
			}
		}
	}

	// ���� ��� �޽��� ����
	if (RocketMesh)
	{
		RocketMesh->SetVisibility(false);
	}

	if (CollisionBox)
	{
		CollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	// ����Ʈ ������ ����
	if (TrailSystemComponent && TrailSystemComponent->GetSystemInstance())
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
	GetWorldTimerManager().SetTimer(DestroyTimer, this, &ThisClass::DestroyTimerFinished, DestroyTime);
}

void AProjectileRocket::DestroyTimerFinished()
{
	Destroy();
}
