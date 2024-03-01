// Fill out your copyright notice in the Description page of Project Settings.

#include "Weapon/Shotgun.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Character/BlasterCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundCue.h"

AShotgun::AShotgun()
	: NumberOfPellets(10)
{
}

void AShotgun::Fire(const FVector& HitTarget)
{
	// 기존의 히트스캔무기와 다르게 사용해야되서 Super가 아니라 AWeapn의 파이어를 호출
	AWeapon::Fire(HitTarget);

	if (GetWeaponMesh())
	{
		if (const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName(TEXT("MuzzleFlash")))
		{
			const FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
			const FVector Start = SocketTransform.GetLocation();

			TMap<ABlasterCharacter*, uint32> HitMap;

			// 산탄
			for (uint32 i = 0; i < NumberOfPellets; ++i)
			{
				FHitResult FireHit;
				WeaponTraceHit(Start, HitTarget, FireHit);

				// 데미지만 서버 확인
				if (HasAuthority())
				{
					// 타겟별로 몇번 맞았는지 확인. 여기서 반복문으로 데미지를 주는것보다는 횟수를 확인하고 한번에 데미지를 주는게 패킷상으로 괜찮다.
					if (ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(FireHit.GetActor()))
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

			// 맞은 횟수만큼 한번에 데미지 계산
			for (auto HitPair : HitMap)
			{
				if (!HitPair.Key) continue;

				if (AController* InstigatorController = HitPair.Key->GetController())
				{
					UGameplayStatics::ApplyDamage(HitPair.Key, Damage * HitPair.Value, InstigatorController, this, UDamageType::StaticClass());
				}
			}
		}
	}
}
