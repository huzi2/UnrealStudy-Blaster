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

	// 서버 되감기를 사용할 경우
	// 서버 자신이 발사하는 발사체는 레플리케이트, 다른 클라가 발사하는 발사체는 레플리케이트할 필요없고 서버 되감기만 수행한다.
	// 클라는 자신이 발사할 경우 서버 되감기를 사용하고, 레플리케이트는 하지 않는다. 다른 클라는 서버 되감기도, 레플리케이트도 하지 않는다.

	// 서버 되감기를 사용하지 않는 경우
	// 발사체는 서버에서만 발사하고 레플리케이트한다. 클라가 발사할 때도 서버를 통해서 발사하며 레플리케이트된다.

	// 생성 지점은 MuzzleFlash 소켓
	const FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
	// 도착 지점은 CombatComponent가 라인트레이스로 얻은 지점
	const FVector ToTarget = HitTarget - SocketTransform.GetLocation();
	const FRotator TargetRotation = ToTarget.Rotation();

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = GetOwner();
	SpawnParams.Instigator = InstigatorPawn;

	AProjectile* SpawnedProjectile = nullptr;
	// 서버 되감기를 확인. 서버 되감기를 사용한다면 클라이언트도 로컬로 발사체를 생성한다.
	if (bUseServerSideRewind)
	{
		// 서버의 경우
		if (InstigatorPawn->HasAuthority())
		{
			// 서버 본인이 발사한 경우 레플리케이트 되는 발사체를 발사한다.
			if (InstigatorPawn->IsLocallyControlled())
			{
				SpawnedProjectile = World->SpawnActor<AProjectile>(ProjectileClass, SocketTransform.GetLocation(), TargetRotation, SpawnParams);
				SpawnedProjectile->SetUseServerSideRewind(false);
				SpawnedProjectile->SetDamage(Damage);
				SpawnedProjectile->SetHeadShotDamage(HeadShotDamage);
			}
			// 서버가 아닌 다른 클라가 발사한걸 서버가 보는 경우는 레플리케이트하지 않는 발사체를 발사하고 서버 되감기 사용
			else
			{
				SpawnedProjectile = World->SpawnActor<AProjectile>(ServerSideRewindProjectileClass, SocketTransform.GetLocation(), TargetRotation, SpawnParams);
				SpawnedProjectile->SetUseServerSideRewind(true);
			}
		}
		// 클라이언트의 경우
		else
		{
			// 클라 본인이 발사한 경우, 레플리케이트 하지 않고 서버 되감기를 사용한다.
			if (InstigatorPawn->IsLocallyControlled())
			{
				SpawnedProjectile = World->SpawnActor<AProjectile>(ServerSideRewindProjectileClass, SocketTransform.GetLocation(), TargetRotation, SpawnParams);
				SpawnedProjectile->SetUseServerSideRewind(true);
				// 서버 되감기에 사용되는 변수들 설정. 시작위치와 방향과 속도
				SpawnedProjectile->SetTraceStart(SocketTransform.GetLocation());
				SpawnedProjectile->SetInitialVelocity(SpawnedProjectile->GetActorForwardVector() * SpawnedProjectile->GetInitialSpeed());
				SpawnedProjectile->SetDamage(Damage);
				SpawnedProjectile->SetHeadShotDamage(HeadShotDamage);
			}
			// 다른 클라가 발사한 경우, 레플리케이트도, 서버 되감기도 사용하지 않는다.
			else
			{
				SpawnedProjectile = World->SpawnActor<AProjectile>(ServerSideRewindProjectileClass, SocketTransform.GetLocation(), TargetRotation, SpawnParams);
				SpawnedProjectile->SetUseServerSideRewind(false);
			}
		}
	}
	// 서버 되감기를 사용하지 않는다면 발사체는 서버에서만 생성되고 클라들은 레플리케이트된다.
	else
	{
		// 레플리케이트 되는 발사체를 서버에서만 발사한다.
		if (InstigatorPawn->HasAuthority())
		{
			SpawnedProjectile = World->SpawnActor<AProjectile>(ProjectileClass, SocketTransform.GetLocation(), TargetRotation, SpawnParams);
			SpawnedProjectile->SetUseServerSideRewind(false);
			SpawnedProjectile->SetDamage(Damage);
			SpawnedProjectile->SetHeadShotDamage(HeadShotDamage);
		}
	}
}
