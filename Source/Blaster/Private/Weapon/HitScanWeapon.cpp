// Fill out your copyright notice in the Description page of Project Settings.

#include "Weapon/HitScanWeapon.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Character/BlasterCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundCue.h"

AHitScanWeapon::AHitScanWeapon()
	: Damage(20.f)
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
