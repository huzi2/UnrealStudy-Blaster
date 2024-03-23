// Fill out your copyright notice in the Description page of Project Settings.

#include "Weapon/Shotgun.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Character/BlasterCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundCue.h"
#include "Kismet/KismetMathLibrary.h"
#include "BlasterComponents/LagCompensationComponent.h"
#include "PlayerController/BlasterPlayerController.h"

AShotgun::AShotgun()
	: NumberOfPellets(10)
{
}

void AShotgun::ShotgunTraceEndWithScatter(const FVector& HitTarget, TArray<FVector_NetQuantize>& HitTargets) const
{
	if (!GetWeaponMesh()) return;

	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName(TEXT("MuzzleFlash"));
	if (!MuzzleFlashSocket) return;

	const FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
	const FVector TraceStart = SocketTransform.GetLocation();

	// 샷건처럼 탄환을 방사시키기 위해 타겟지점에 원형 구체를 생성
	const FVector ToTargetNormalized = (HitTarget - TraceStart).GetSafeNormal();
	const FVector SphereCenter = TraceStart + ToTargetNormalized * DistanceToSphere;

	HitTargets.Reserve(NumberOfPellets);
	for (uint32 i = 0; i < NumberOfPellets; ++i)
	{
		// 구체 기준으로 랜덤한 벡터 하나를 고름
		const FVector RandVec = UKismetMathLibrary::RandomUnitVector() * FMath::FRandRange(0.f, SphereRadius);
		const FVector EndLoc = SphereCenter + RandVec;
		// 랜덤한 벡터까지의 거리
		FVector ToEndLoc = EndLoc - TraceStart;

		// 시작지점에서 랜덤위치까지 Trace 거리/사이즈만큼이 엔드포인트
		// 거리/사이즈는 그냥 기존 거리가 너무 멀어서 정한 임의의 사정거리임
		ToEndLoc = TraceStart + ToEndLoc * TRACE_LENGTH / ToEndLoc.Size();

		HitTargets.Add(ToEndLoc);
	}
}

void AShotgun::FireShotgun(const TArray<FVector_NetQuantize>& HitTargets)
{
	AWeapon::Fire(FVector());

	if (const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName(TEXT("MuzzleFlash")))
	{
		const FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
		const FVector Start = SocketTransform.GetLocation();

		TMap<ABlasterCharacter*, uint32> HitMap;
		TMap<ABlasterCharacter*, uint32> HeadShotHitMap;
		for (const FVector_NetQuantize& HitTarget : HitTargets)
		{
			FHitResult FireHit;
			WeaponTraceHit(Start, HitTarget, FireHit);

			// 타겟별로 몇번 맞았는지 확인. 여기서 반복문으로 데미지를 주는것보다는 횟수를 확인하고 한번에 데미지를 주는게 패킷상으로 괜찮다.
			if (ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(FireHit.GetActor()))
			{
				// 헤드샷 확인
				const bool bHeadShot = FireHit.BoneName.ToString() == TEXT("head");
				if (bHeadShot)
				{
					HeadShotHitMap.Contains(BlasterCharacter) ? ++HeadShotHitMap[BlasterCharacter] : HeadShotHitMap.Emplace(BlasterCharacter, 1);
				}
				else
				{
					HitMap.Contains(BlasterCharacter) ? ++HitMap[BlasterCharacter] : HitMap.Emplace(BlasterCharacter, 1);
				}
			}

			if (ImpactParticles)
			{
				UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactParticles, FireHit.ImpactPoint, FireHit.ImpactNormal.Rotation());
			}

			if (HitSound)
			{
				UGameplayStatics::PlaySoundAtLocation(this, HitSound, FireHit.ImpactPoint, 0.5f, FMath::FRandRange(-0.5f, 0.5f));
			}
		}

		// 서버는 바로 데미지 확인
		CheckInit();
		const bool bCauseAuthDamage = !bUseServerSideRewind || BlasterOwnerCharacter->IsLocallyControlled();
		if (HasAuthority() && bCauseAuthDamage)
		{
			// 맞은 횟수만큼 한번에 데미지 계산
			TMap<ABlasterCharacter*, float> DamageMap;

			for (const auto& HitPair : HitMap)
			{
				if (!HitPair.Key) continue;

				DamageMap.Emplace(HitPair.Key, HitPair.Value * Damage);
			}

			for (const auto& HedShotHitPair : HeadShotHitMap)
			{
				if (!HedShotHitPair.Key) continue;

				DamageMap.Contains(HedShotHitPair.Key) ? DamageMap[HedShotHitPair.Key] += HedShotHitPair.Value * HeadShotDamage : DamageMap.Emplace(HedShotHitPair.Key, HedShotHitPair.Value * HeadShotDamage);
			}

			// 샷건에 맞은 캐릭터들에게 계산된 데미지를 입힘
			AController* InstigatorController = BlasterOwnerCharacter->GetController();
			for (const auto& DamagePair : DamageMap)
			{
				if (!DamagePair.Key) continue;

				UGameplayStatics::ApplyDamage(DamagePair.Key, DamagePair.Value, InstigatorController, this, UDamageType::StaticClass());
			}
		}

		// 클라의 경우 서버 되감기 기능을 사용한다면 서버 되감기로 충돌 판정 확인 후 데미지
		// 서버 되감기로 높은 렉에서도 어느정도 정확한 타격 판정을 얻을 수 있다.
		if (!HasAuthority() && bUseServerSideRewind)
		{
			if (BlasterOwnerCharacter && BlasterOwnerCharacter->IsLocallyControlled() && BlasterOwnerController && BlasterOwnerCharacter->GetLagCompensation())
			{
				TArray<ABlasterCharacter*> HitCharacters;
				HitCharacters.Reserve(HitTargets.Num());

				for (const auto& HitPair : HitMap)
				{
					if (!HitPair.Key) continue;
					HitCharacters.Add(HitPair.Key);
				}

				// 클라에서 보는 타겟의 위치는 ServerTime - SingleTripTime으로 서버 시간에서 한번 패킷이 전달되는 시간만큼의 차이가 서버와 클라의 차이
				BlasterOwnerCharacter->GetLagCompensation()->ServerShotgunScoreRequest(HitCharacters, Start, HitTargets, static_cast<double>(BlasterOwnerController->GetServerTime() - BlasterOwnerController->GetSingleTripTime()), this);
			}
		}
	}
}
