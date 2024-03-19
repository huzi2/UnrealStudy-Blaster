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
// InitialSpeed를 블루프린트에서 수정해도 에디터에서는 ProjectileMovementComponent에 적용이 안되서 PostEditChangeProperty()로 수정
void AProjectileBullet::PostEditChangeProperty(FPropertyChangedEvent& Event)
{
	Super::PostEditChangeProperty(Event);

	if (Event.Property)
	{
		// InitialSpeed가 변경되었는지 확인
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

	// 발사체 예측 기능 테스트. 아래 주석 풀고 발사체 발사해보면 예측 경로와 똑같이 발사체가 날아가야한다.

	// 엔진에서 제공하는 발사체 예측 기능을 사용해서 클라이언트가 렉이 심해도 서버 되감기로 정확하게 충돌판정을 수행
	// 예측 발사체는 실제 발사체가 나아갈 곳을 먼저 예측한다. 그래서 값을 정확하게 입력했으면 디버그에서 발사체가 갈 위치를 미리 보여주고 발사체가 나중에 도착할 것
	//FPredictProjectilePathParams PathParams;
	//// 트레이스 채널을 사용
	//PathParams.bTraceWithChannel = true;
	//PathParams.TraceChannel = ECollisionChannel::ECC_Visibility;
	//// 트레이스가 충돌체와 충돌
	//PathParams.bTraceWithCollision = true;
	//// 발사체의 방향과 속도
	//PathParams.LaunchVelocity = GetActorForwardVector() * InitialSpeed;
	//// 발사체가 날아가는 시간. 이 값은 대충 설정
	//PathParams.MaxSimTime = 4.f;
	//// 발사체의 크기
	//PathParams.ProjectileRadius = 5.f;
	//// 얼마나 시뮬레이션할 것인가? 10 ~ 30. 30이 제일 자세하게 시뮬
	//PathParams.SimFrequency = 30.f;
	//// 발사체 시작 위치
	//PathParams.StartLocation = GetActorLocation();
	//// 임시 발사체가 무시할 액터. 자신의 발사체와 충돌하면 안되니까 this
	//PathParams.ActorsToIgnore.Add(this);

	//// 발사체 예측을 디버그로 표시
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
			// 서버는 바로 데미지 확인
			if (OwnerCharacter->HasAuthority())
			{
				UGameplayStatics::ApplyDamage(OtherActor, Damage, OwnerController, this, UDamageType::StaticClass());
				Super::OnHit(HItComp, OtherActor, OtherComp, NormalImpulse, Hit);
				return;
			}
			// 클라면서 서버 되감기를 사용
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

	// 타겟의 히트 판정과 탄환 제거
	Super::OnHit(HItComp, OtherActor, OtherComp, NormalImpulse, Hit);
}
