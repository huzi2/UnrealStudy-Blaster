// Fill out your copyright notice in the Description page of Project Settings.

#include "Weapon/ProjectileWeapon.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Weapon/Projectile.h"

void AProjectileWeapon::Fire(const FVector& HitTarget)
{
	Super::Fire(HitTarget);

	// 발사체 생성은 서버에서만 한다.
	if (!HasAuthority()) return;

	if (!ProjectileClass) return;

	UWorld* World = GetWorld();
	if (!World) return;

	APawn* InstigatorPawn = Cast<APawn>(GetOwner());
	if (!InstigatorPawn) return;

	if (GetWeaponMesh())
	{
		const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName(TEXT("MuzzleFlash"));
		if (MuzzleFlashSocket)
		{
			// 생성 지점은 MuzzleFlash 소켓
			const FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
			// 도착 지점은 CombatComponent가 라인트레이스로 얻은 지점
			const FVector ToTarget = HitTarget - SocketTransform.GetLocation();
			const FRotator TargetRotation = ToTarget.Rotation();

			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = GetOwner();
			SpawnParams.Instigator = InstigatorPawn;

			World->SpawnActor<AProjectile>(ProjectileClass, SocketTransform.GetLocation(), TargetRotation, SpawnParams);
		}
	}
}
