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

	// ����ó�� źȯ�� ����Ű�� ���� Ÿ�������� ���� ��ü�� ����
	const FVector ToTargetNormalized = (HitTarget - TraceStart).GetSafeNormal();
	const FVector SphereCenter = TraceStart + ToTargetNormalized * DistanceToSphere;

	HitTargets.Reserve(NumberOfPellets);
	for (uint32 i = 0; i < NumberOfPellets; ++i)
	{
		// ��ü �������� ������ ���� �ϳ��� ��
		const FVector RandVec = UKismetMathLibrary::RandomUnitVector() * FMath::FRandRange(0.f, SphereRadius);
		const FVector EndLoc = SphereCenter + RandVec;
		// ������ ���ͱ����� �Ÿ�
		FVector ToEndLoc = EndLoc - TraceStart;

		// ������������ ������ġ���� Trace �Ÿ�/�����ŭ�� ��������Ʈ
		// �Ÿ�/������� �׳� ���� �Ÿ��� �ʹ� �־ ���� ������ �����Ÿ���
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

			// Ÿ�ٺ��� ��� �¾Ҵ��� Ȯ��. ���⼭ �ݺ������� �������� �ִ°ͺ��ٴ� Ƚ���� Ȯ���ϰ� �ѹ��� �������� �ִ°� ��Ŷ������ ������.
			if (ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(FireHit.GetActor()))
			{
				// ��弦 Ȯ��
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

		// ������ �ٷ� ������ Ȯ��
		CheckInit();
		const bool bCauseAuthDamage = !bUseServerSideRewind || BlasterOwnerCharacter->IsLocallyControlled();
		if (HasAuthority() && bCauseAuthDamage)
		{
			// ���� Ƚ����ŭ �ѹ��� ������ ���
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

			// ���ǿ� ���� ĳ���͵鿡�� ���� �������� ����
			AController* InstigatorController = BlasterOwnerCharacter->GetController();
			for (const auto& DamagePair : DamageMap)
			{
				if (!DamagePair.Key) continue;

				UGameplayStatics::ApplyDamage(DamagePair.Key, DamagePair.Value, InstigatorController, this, UDamageType::StaticClass());
			}
		}

		// Ŭ���� ��� ���� �ǰ��� ����� ����Ѵٸ� ���� �ǰ���� �浹 ���� Ȯ�� �� ������
		// ���� �ǰ���� ���� �������� ������� ��Ȯ�� Ÿ�� ������ ���� �� �ִ�.
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

				// Ŭ�󿡼� ���� Ÿ���� ��ġ�� ServerTime - SingleTripTime���� ���� �ð����� �ѹ� ��Ŷ�� ���޵Ǵ� �ð���ŭ�� ���̰� ������ Ŭ���� ����
				BlasterOwnerCharacter->GetLagCompensation()->ServerShotgunScoreRequest(HitCharacters, Start, HitTargets, static_cast<double>(BlasterOwnerController->GetServerTime() - BlasterOwnerController->GetSingleTripTime()), this);
			}
		}
	}
}
