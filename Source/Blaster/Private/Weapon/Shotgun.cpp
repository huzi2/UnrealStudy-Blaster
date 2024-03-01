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
	// ������ ��Ʈ��ĵ����� �ٸ��� ����ؾߵǼ� Super�� �ƴ϶� AWeapn�� ���̾ ȣ��
	AWeapon::Fire(HitTarget);

	if (GetWeaponMesh())
	{
		if (const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName(TEXT("MuzzleFlash")))
		{
			const FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
			const FVector Start = SocketTransform.GetLocation();

			TMap<ABlasterCharacter*, uint32> HitMap;

			// ��ź
			for (uint32 i = 0; i < NumberOfPellets; ++i)
			{
				FHitResult FireHit;
				WeaponTraceHit(Start, HitTarget, FireHit);

				// �������� ���� Ȯ��
				if (HasAuthority())
				{
					// Ÿ�ٺ��� ��� �¾Ҵ��� Ȯ��. ���⼭ �ݺ������� �������� �ִ°ͺ��ٴ� Ƚ���� Ȯ���ϰ� �ѹ��� �������� �ִ°� ��Ŷ������ ������.
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

			// ���� Ƚ����ŭ �ѹ��� ������ ���
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
