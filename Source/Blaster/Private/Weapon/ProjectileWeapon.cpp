// Fill out your copyright notice in the Description page of Project Settings.

#include "Weapon/ProjectileWeapon.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Weapon/Projectile.h"

void AProjectileWeapon::Fire(const FVector& HitTarget)
{
	Super::Fire(HitTarget);

	// �߻�ü ������ ���������� �Ѵ�.
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
			// ���� ������ MuzzleFlash ����
			const FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
			// ���� ������ CombatComponent�� ����Ʈ���̽��� ���� ����
			const FVector ToTarget = HitTarget - SocketTransform.GetLocation();
			const FRotator TargetRotation = ToTarget.Rotation();

			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = GetOwner();
			SpawnParams.Instigator = InstigatorPawn;

			World->SpawnActor<AProjectile>(ProjectileClass, SocketTransform.GetLocation(), TargetRotation, SpawnParams);
		}
	}
}
