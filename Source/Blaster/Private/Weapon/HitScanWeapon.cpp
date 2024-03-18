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

			// ������ �ٷ� ������ Ȯ��
			if (HasAuthority() || !bUseServerSideRewind)
			{
				if (BlasterCharacter)
				{
					AController* InstigatorController = BlasterCharacter->GetController();
					UGameplayStatics::ApplyDamage(BlasterCharacter, Damage, InstigatorController, this, UDamageType::StaticClass());
				}
			}
			// Ŭ���� ��� ���� �ǰ��� ����� ����Ѵٸ� ���� �ǰ���� �浹 ���� Ȯ�� �� ������
			// ���� �ǰ���� ���� �������� ������� ��Ȯ�� Ÿ�� ������ ���� �� �ִ�.
			else if (!HasAuthority() && bUseServerSideRewind)
			{
				CheckInit();

				if (BlasterCharacter && BlasterOwnerCharacter && BlasterOwnerCharacter->IsLocallyControlled() && BlasterOwnerController && BlasterOwnerCharacter->GetLagCompensation())
				{
					// Ŭ�󿡼� ���� Ÿ���� ��ġ�� ServerTime - SingleTripTime���� ���� �ð����� �ѹ� ��Ŷ�� ���޵Ǵ� �ð���ŭ�� ���̰� ������ Ŭ���� ����
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
	
	// ���� �������� ���������� ���ؼ� ����Ʈ���̽�
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
