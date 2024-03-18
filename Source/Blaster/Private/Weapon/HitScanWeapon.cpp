// Fill out your copyright notice in the Description page of Project Settings.

#include "Weapon/HitScanWeapon.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Character/BlasterCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundCue.h"
#include "BlasterComponents/LagCompensationComponent.h"
#include "PlayerController/BlasterPlayerController.h"

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
			ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(FireHit.GetActor());

			// 서버는 바로 데미지 확인
			if (HasAuthority() || !bUseServerSideRewind)
			{
				if (BlasterCharacter)
				{
					AController* InstigatorController = BlasterCharacter->GetController();
					UGameplayStatics::ApplyDamage(BlasterCharacter, Damage, InstigatorController, this, UDamageType::StaticClass());
				}
			}
			// 클라의 경우 서버 되감기 기능을 사용한다면 서버 되감기로 충돌 판정 확인 후 데미지
			// 서버 되감기로 높은 렉에서도 어느정도 정확한 타격 판정을 얻을 수 있다.
			else if (!HasAuthority() && bUseServerSideRewind)
			{
				CheckInit();

				if (BlasterCharacter && BlasterOwnerCharacter && BlasterOwnerCharacter->IsLocallyControlled() && BlasterOwnerController && BlasterOwnerCharacter->GetLagCompensation())
				{
					// 클라에서 보는 타겟의 위치는 ServerTime - SingleTripTime으로 서버 시간에서 한번 패킷이 전달되는 시간만큼의 차이가 서버와 클라의 차이
					BlasterOwnerCharacter->GetLagCompensation()->ServerScoreRequest(BlasterCharacter, Start, HitTarget, static_cast<double>(BlasterOwnerController->GetServerTime() - BlasterOwnerController->GetSingleTripTime()), this);
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
	
	// 얻어온 시작점과 도착지점을 통해서 라인트레이스
	const FVector End = TraceStart + (HitTarget - TraceStart) * 1.25;
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
