// Fill out your copyright notice in the Description page of Project Settings.

#include "Weapon/ProjectileWeapon.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Weapon/Projectile.h"

void AProjectileWeapon::Fire(const FVector& HitTarget)
{
	Super::Fire(HitTarget);

	if (!ProjectileClass) return;
	if (!ServerSideRewindProjectileClass) return;

	UWorld* World = GetWorld();
	if (!World) return;

	APawn* InstigatorPawn = Cast<APawn>(GetOwner());
	if (!InstigatorPawn) return;

	if (!GetWeaponMesh()) return;

	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName(TEXT("MuzzleFlash"));
	if (!MuzzleFlashSocket) return;

	// ���� �ǰ��⸦ ����� ���
	// ���� �ڽ��� �߻��ϴ� �߻�ü�� ���ø�����Ʈ, �ٸ� Ŭ�� �߻��ϴ� �߻�ü�� ���ø�����Ʈ�� �ʿ���� ���� �ǰ��⸸ �����Ѵ�.
	// Ŭ��� �ڽ��� �߻��� ��� ���� �ǰ��⸦ ����ϰ�, ���ø�����Ʈ�� ���� �ʴ´�. �ٸ� Ŭ��� ���� �ǰ��⵵, ���ø�����Ʈ�� ���� �ʴ´�.

	// ���� �ǰ��⸦ ������� �ʴ� ���
	// �߻�ü�� ���������� �߻��ϰ� ���ø�����Ʈ�Ѵ�. Ŭ�� �߻��� ���� ������ ���ؼ� �߻��ϸ� ���ø�����Ʈ�ȴ�.

	// ���� ������ MuzzleFlash ����
	const FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
	// ���� ������ CombatComponent�� ����Ʈ���̽��� ���� ����
	const FVector ToTarget = HitTarget - SocketTransform.GetLocation();
	const FRotator TargetRotation = ToTarget.Rotation();

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = GetOwner();
	SpawnParams.Instigator = InstigatorPawn;

	AProjectile* SpawnedProjectile = nullptr;
	// ���� �ǰ��⸦ Ȯ��. ���� �ǰ��⸦ ����Ѵٸ� Ŭ���̾�Ʈ�� ���÷� �߻�ü�� �����Ѵ�.
	if (bUseServerSideRewind)
	{
		// ������ ���
		if (InstigatorPawn->HasAuthority())
		{
			// ���� ������ �߻��� ��� ���ø�����Ʈ �Ǵ� �߻�ü�� �߻��Ѵ�.
			if (InstigatorPawn->IsLocallyControlled())
			{
				SpawnedProjectile = World->SpawnActor<AProjectile>(ProjectileClass, SocketTransform.GetLocation(), TargetRotation, SpawnParams);
				SpawnedProjectile->SetUseServerSideRewind(false);
				SpawnedProjectile->SetDamage(Damage);
				SpawnedProjectile->SetHeadShotDamage(HeadShotDamage);
			}
			// ������ �ƴ� �ٸ� Ŭ�� �߻��Ѱ� ������ ���� ���� ���ø�����Ʈ���� �ʴ� �߻�ü�� �߻��ϰ� ���� �ǰ��� ���
			else
			{
				SpawnedProjectile = World->SpawnActor<AProjectile>(ServerSideRewindProjectileClass, SocketTransform.GetLocation(), TargetRotation, SpawnParams);
				SpawnedProjectile->SetUseServerSideRewind(true);
			}
		}
		// Ŭ���̾�Ʈ�� ���
		else
		{
			// Ŭ�� ������ �߻��� ���, ���ø�����Ʈ ���� �ʰ� ���� �ǰ��⸦ ����Ѵ�.
			if (InstigatorPawn->IsLocallyControlled())
			{
				SpawnedProjectile = World->SpawnActor<AProjectile>(ServerSideRewindProjectileClass, SocketTransform.GetLocation(), TargetRotation, SpawnParams);
				SpawnedProjectile->SetUseServerSideRewind(true);
				// ���� �ǰ��⿡ ���Ǵ� ������ ����. ������ġ�� ����� �ӵ�
				SpawnedProjectile->SetTraceStart(SocketTransform.GetLocation());
				SpawnedProjectile->SetInitialVelocity(SpawnedProjectile->GetActorForwardVector() * SpawnedProjectile->GetInitialSpeed());
				SpawnedProjectile->SetDamage(Damage);
				SpawnedProjectile->SetHeadShotDamage(HeadShotDamage);
			}
			// �ٸ� Ŭ�� �߻��� ���, ���ø�����Ʈ��, ���� �ǰ��⵵ ������� �ʴ´�.
			else
			{
				SpawnedProjectile = World->SpawnActor<AProjectile>(ServerSideRewindProjectileClass, SocketTransform.GetLocation(), TargetRotation, SpawnParams);
				SpawnedProjectile->SetUseServerSideRewind(false);
			}
		}
	}
	// ���� �ǰ��⸦ ������� �ʴ´ٸ� �߻�ü�� ���������� �����ǰ� Ŭ����� ���ø�����Ʈ�ȴ�.
	else
	{
		// ���ø�����Ʈ �Ǵ� �߻�ü�� ���������� �߻��Ѵ�.
		if (InstigatorPawn->HasAuthority())
		{
			SpawnedProjectile = World->SpawnActor<AProjectile>(ProjectileClass, SocketTransform.GetLocation(), TargetRotation, SpawnParams);
			SpawnedProjectile->SetUseServerSideRewind(false);
			SpawnedProjectile->SetDamage(Damage);
			SpawnedProjectile->SetHeadShotDamage(HeadShotDamage);
		}
	}
}
