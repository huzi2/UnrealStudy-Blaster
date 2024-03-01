// Fill out your copyright notice in the Description page of Project Settings.

#include "Weapon/HitScanWeapon.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Character/BlasterCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundCue.h"
#include "Kismet/KismetMathLibrary.h"
#include "Weapon/WeaponTypes.h"
#include "DrawDebugHelpers.h"

AHitScanWeapon::AHitScanWeapon()
	: Damage(20.f)
	, DistanceToSphere(800.f)
	, SphereRadius(75.f)
	, bUseScatter(false)
{
}

void AHitScanWeapon::Fire(const FVector& HitTarget)
{
	Super::Fire(HitTarget);

	if (GetWeaponMesh())
	{
		if (const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName(TEXT("MuzzleFlash")))
		{
			const FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
			const FVector Start = SocketTransform.GetLocation();

			FHitResult FireHit;
			WeaponTraceHit(Start, HitTarget, FireHit);

			// 데미지만 서버 확인
			if (HasAuthority())
			{
				if (ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(FireHit.GetActor()))
				{
					AController* InstigatorController = BlasterCharacter->GetController();
					UGameplayStatics::ApplyDamage(BlasterCharacter, Damage, InstigatorController, this, UDamageType::StaticClass());
				}
			}

			if (ImpactParticles)
			{
				UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactParticles, FireHit.ImpactPoint, FireHit.ImpactNormal.Rotation());
			}

			if (HitSound)
			{
				UGameplayStatics::PlaySoundAtLocation(this, HitSound, FireHit.ImpactPoint);
			}

			if (MuzzleFlash)
			{
				UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), MuzzleFlash, SocketTransform);
			}

			if (FireSound)
			{
				UGameplayStatics::PlaySoundAtLocation(this, FireSound, GetActorLocation());
			}
		}
	}
}

void AHitScanWeapon::WeaponTraceHit(const FVector& TraceStart, const FVector& HitTarget, FHitResult& OutHit)
{
	UWorld* World = GetWorld();
	if (!World) return;
	
	// 얻어온 시작점과 도착지점을 통해서 라인트레이스. 방사형 무기면 탄환을 방사시킴
	const FVector End = bUseScatter ? TraceEndWithScatter(TraceStart, HitTarget) : TraceStart + (HitTarget - TraceStart) * 1.25;
	World->LineTraceSingleByChannel(OutHit, TraceStart, End, ECollisionChannel::ECC_Visibility);

	const FVector BeamEnd = OutHit.bBlockingHit ? OutHit.ImpactPoint : End;

	if (BeamParticles)
	{
		if (UParticleSystemComponent* Beam = UGameplayStatics::SpawnEmitterAtLocation(World, BeamParticles, TraceStart, FRotator::ZeroRotator, true))
		{
			Beam->SetVectorParameter(FName(TEXT("Target")), BeamEnd);
		}
	}
}

FVector AHitScanWeapon::TraceEndWithScatter(const FVector& TraceStart, const FVector& HitTarget) const
{
	// 샷건처럼 탄환을 방사시키기 위해 타겟지점에 원형 구체를 생성
	const FVector ToTargetNormalized = (HitTarget - TraceStart).GetSafeNormal();
	const FVector SphereCenter = TraceStart + ToTargetNormalized * DistanceToSphere;
	// 구체 기준으로 랜덤한 벡터 하나를 고름
	const FVector RandVec = UKismetMathLibrary::RandomUnitVector() * FMath::FRandRange(0.f, SphereRadius);
	const FVector EndLoc = SphereCenter + RandVec;
	// 랜덤한 벡터까지의 거리
	const FVector ToEndLoc = EndLoc - TraceStart;

	/*DrawDebugSphere(GetWorld(), SphereCenter, SphereRadius, 12, FColor::Red, true);
	DrawDebugSphere(GetWorld(), EndLoc, 4.f, 12, FColor::Orange, true);
	DrawDebugLine(GetWorld(), TraceStart, TraceStart + ToEndLoc * TRACE_LENGTH / ToEndLoc.Size(), FColor::Cyan, true);*/

	// 시작지점에서 랜덤위치까지 Trace 거리/사이즈만큼이 엔드포인트
	// 거리/사이즈는 그냥 기존 거리가 너무 멀어서 정한 임의의 사정거리임
	return TraceStart + ToEndLoc * TRACE_LENGTH / ToEndLoc.Size();
}
