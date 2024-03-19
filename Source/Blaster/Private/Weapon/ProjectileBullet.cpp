// Fill out your copyright notice in the Description page of Project Settings.

#include "Weapon/ProjectileBullet.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Character/BlasterCharacter.h"
#include "PlayerController/BlasterPlayerController.h"
#include "BlasterComponents/LagCompensationComponent.h"

AProjectileBullet::AProjectileBullet()
{
	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));
	ProjectileMovementComponent->bRotationFollowsVelocity = true;
	ProjectileMovementComponent->SetIsReplicated(true);
	ProjectileMovementComponent->InitialSpeed = InitialSpeed;
	ProjectileMovementComponent->MaxSpeed = InitialSpeed;
}

#if WITH_EDITOR
// InitialSpeed�� �������Ʈ���� �����ص� �����Ϳ����� ProjectileMovementComponent�� ������ �ȵǼ� PostEditChangeProperty()�� ����
void AProjectileBullet::PostEditChangeProperty(FPropertyChangedEvent& Event)
{
	Super::PostEditChangeProperty(Event);

	if (Event.Property)
	{
		// InitialSpeed�� ����Ǿ����� Ȯ��
		const FName PropertyName = Event.Property->GetFName();
		if (PropertyName == GET_MEMBER_NAME_CHECKED(AProjectileBullet, InitialSpeed))
		{
			if (ProjectileMovementComponent)
			{
				ProjectileMovementComponent->InitialSpeed = InitialSpeed;
				ProjectileMovementComponent->MaxSpeed = InitialSpeed;
			}
		}
	}
}
#endif

void AProjectileBullet::BeginPlay()
{
	Super::BeginPlay();

	// �߻�ü ���� ��� �׽�Ʈ. �Ʒ� �ּ� Ǯ�� �߻�ü �߻��غ��� ���� ��ο� �Ȱ��� �߻�ü�� ���ư����Ѵ�.

	// �������� �����ϴ� �߻�ü ���� ����� ����ؼ� Ŭ���̾�Ʈ�� ���� ���ص� ���� �ǰ���� ��Ȯ�ϰ� �浹������ ����
	// ���� �߻�ü�� ���� �߻�ü�� ���ư� ���� ���� �����Ѵ�. �׷��� ���� ��Ȯ�ϰ� �Է������� ����׿��� �߻�ü�� �� ��ġ�� �̸� �����ְ� �߻�ü�� ���߿� ������ ��
	//FPredictProjectilePathParams PathParams;
	//// Ʈ���̽� ä���� ���
	//PathParams.bTraceWithChannel = true;
	//PathParams.TraceChannel = ECollisionChannel::ECC_Visibility;
	//// Ʈ���̽��� �浹ü�� �浹
	//PathParams.bTraceWithCollision = true;
	//// �߻�ü�� ����� �ӵ�
	//PathParams.LaunchVelocity = GetActorForwardVector() * InitialSpeed;
	//// �߻�ü�� ���ư��� �ð�. �� ���� ���� ����
	//PathParams.MaxSimTime = 4.f;
	//// �߻�ü�� ũ��
	//PathParams.ProjectileRadius = 5.f;
	//// �󸶳� �ùķ��̼��� ���ΰ�? 10 ~ 30. 30�� ���� �ڼ��ϰ� �ù�
	//PathParams.SimFrequency = 30.f;
	//// �߻�ü ���� ��ġ
	//PathParams.StartLocation = GetActorLocation();
	//// �ӽ� �߻�ü�� ������ ����. �ڽ��� �߻�ü�� �浹�ϸ� �ȵǴϱ� this
	//PathParams.ActorsToIgnore.Add(this);

	//// �߻�ü ������ ����׷� ǥ��
	//PathParams.DrawDebugTime = 5.f;
	//PathParams.DrawDebugType = EDrawDebugTrace::ForDuration;
	//
	//FPredictProjectilePathResult PathResult;
	//UGameplayStatics::PredictProjectilePath(this, PathParams, PathResult);
}

void AProjectileBullet::OnHit(UPrimitiveComponent* HItComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (!OtherActor) return;

	if (ABlasterCharacter* OwnerCharacter = Cast<ABlasterCharacter>(GetOwner()))
	{
		if (ABlasterPlayerController* OwnerController = Cast<ABlasterPlayerController>(OwnerCharacter->Controller))
		{
			// ������ �ٷ� ������ Ȯ��
			if (OwnerCharacter->HasAuthority())
			{
				UGameplayStatics::ApplyDamage(OtherActor, Damage, OwnerController, this, UDamageType::StaticClass());
				Super::OnHit(HItComp, OtherActor, OtherComp, NormalImpulse, Hit);
				return;
			}
			// Ŭ��鼭 ���� �ǰ��⸦ ���
			if (bUseServerSideRewind)
			{
				ABlasterCharacter* HitCharacter = Cast<ABlasterCharacter>(OtherActor);
				if (OwnerCharacter->IsLocallyControlled() && OwnerCharacter->GetLagCompensation() && HitCharacter)
				{
					OwnerCharacter->GetLagCompensation()->ServerProjectileScoreRequest(HitCharacter, TraceStart, InitialVelocity, static_cast<double>(OwnerController->GetServerTime() - OwnerController->GetSingleTripTime()), this);
				}
			}
		}
	}

	// Ÿ���� ��Ʈ ������ źȯ ����
	Super::OnHit(HItComp, OtherActor, OtherComp, NormalImpulse, Hit);
}
