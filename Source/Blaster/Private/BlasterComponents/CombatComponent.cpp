// Fill out your copyright notice in the Description page of Project Settings.

#include "BlasterComponents/CombatComponent.h"
#include "Weapon/Weapon.h"
#include "Character/BlasterCharacter.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "PlayerController/BlasterPlayerController.h"
#include "Camera/CameraComponent.h"
#include "TimerManager.h"
#include "Sound/SoundCue.h"
#include "Character/BlasterAnimInstance.h"
#include "Weapon/Projectile.h"
#include "Weapon/Shotgun.h"

UCombatComponent::UCombatComponent()
	: bAiming(false)
	, BaseWalkSpeed(600.f)
	, AimWalkSpeed(450.f)
	, ZoomedFOV(30.f)
	, ZoomInterpSpeed(20.f)
	, StartingARAmmo(30)
	, StartingRocketAmmo(0)
	, StartingPistolAmmo(0)
	, StartingSMGAmmo(0)
	, StartingShotgunAmmo(0)
	, StartingSniperAmmo(0)
	, StartingGrenaderLauncherAmmo(0)
	, MaxGrenades(4)
	, MaxCarriedAmmo(500)
	, CombatState(ECombatState::ECS_Unoccupied)
	, Grenades(4)
	, bHoldingTheFlag(false)
	, bCanFire(true)
	, bAimButtonPressed(false)
	, bLocallyReloading(false)
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	if (Character)
	{
		if (Character->GetCharacterMovement())
		{
			Character->GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;
		}

		if (Character->GetFollowCamera())
		{
			DefaultFOV = Character->GetFollowCamera()->FieldOfView;
			CurrentFOV = DefaultFOV;
		}

		if (Character->HasAuthority())
		{
			InitializeCarriedAmmo();
		}
	}
}

void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// �Ʒ� ��ɵ��� ���� Ŭ�� �����ϸ��
	if (Character && Character->IsLocallyControlled())
	{
		// ������ ����� ���� ������ �����ϱ� ���� ������ ������ ����
		FHitResult HitResult;
		TraceUnderCrosshairs(HitResult);
		HitTarget = HitResult.ImpactPoint;

		// ������ ����
		SetHUDCrosshairs(DeltaTime);

		// ���� ���� �ܾƿ�
		InterpFOV(DeltaTime);
	}
}

void UCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// EquippedWeapon�� ĳ���Ͱ� RPC�� ���������� ȣ���ϱ⿡ ������ Ŭ�󿡼��� nullptr�̴�
	// �ٵ� �ִ��ν��Ͻ����� �ش� ������ ���Ⱑ �����Ǿ����� Ȯ���ϱ⿡ �� Ŭ�� ���ø�����Ʈ�ؾ��Ѵ�.
	DOREPLIFETIME(UCombatComponent, EquippedWeapon);
	DOREPLIFETIME(UCombatComponent, SecondaryWeapon);
	DOREPLIFETIME(UCombatComponent, TheFlag);

	// ���� �ڼ��� �ٸ� Ŭ�� Ȯ���ؾ��ؼ� ���ø�����Ʈ
	DOREPLIFETIME(UCombatComponent, bAiming);

	// ��� ź���� �ٸ� Ŭ����� ���󵵵ǰ� �ڽŸ� �˰������� �Ǵϱ� COND_OwnerOnly
	DOREPLIFETIME_CONDITION(UCombatComponent, CarriedAmmo, COND_OwnerOnly);

	DOREPLIFETIME(UCombatComponent, CombatState);
	DOREPLIFETIME(UCombatComponent, Grenades);
	DOREPLIFETIME(UCombatComponent, bHoldingTheFlag);
}

void UCombatComponent::ServerSetAiming_Implementation(bool bIsAiming)
{
	bAiming = bIsAiming;

	if (Character && Character->GetCharacterMovement())
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
	}
}

void UCombatComponent::ServerFire_Implementation(const FVector_NetQuantize& TraceHitTarget, float FireDelay)
{
	// ��Ƽĳ��Ʈ�� ������ ȣ���� �� ����. �׷��� �������� ó��
	// ��Ƽĳ��Ʈ�� ���� ���������� ȣ��ǹǷ� ���⼭ ���� �� �߻縦 �� �ʿ�� ����.
	MulticastFire(TraceHitTarget);
}

bool UCombatComponent::ServerFire_Validate(const FVector_NetQuantize& TraceHitTarget, float FireDelay)
{
	if (EquippedWeapon)
	{
		// ���� �߻�����̿� ������ �Ѿ�� �߻������ ���� ���ؼ� ġ���� �־����� Ȯ��
		const bool bNearlyEqual = FMath::IsNearlyEqual(EquippedWeapon->GetFireDelay(), FireDelay, 0.001f);
		return bNearlyEqual;
	}
	return true;
}

void UCombatComponent::MulticastFire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{
	// �ٸ� Ŭ��鵵 �ش� ���� Ŭ�� ��� ��ǰ� ����Ʈ�� �����ϹǷ� �ٸ� ��� Ŭ��� �������� ���
	if (Character && Character->IsLocallyControlled() && !Character->HasAuthority()) return;

	// Ŭ���̾�Ʈ�� �����ϴ� ��쿡 ����
	LocalFire(TraceHitTarget);
}

void UCombatComponent::ServerShotgunFire_Implementation(const TArray<FVector_NetQuantize>& TraceHitTargets, float FireDelay)
{
	MulticastShotgunFire(TraceHitTargets);
}

bool UCombatComponent::ServerShotgunFire_Validate(const TArray<FVector_NetQuantize>& TraceHitTargets, float FireDelay)
{
	if (EquippedWeapon)
	{
		// ���� �߻�����̿� ������ �Ѿ�� �߻������ ���� ���ؼ� ġ���� �־����� Ȯ��
		const bool bNearlyEqual = FMath::IsNearlyEqual(EquippedWeapon->GetFireDelay(), FireDelay, 0.001f);
		return bNearlyEqual;
	}
	return true;
}

void UCombatComponent::MulticastShotgunFire_Implementation(const TArray<FVector_NetQuantize>& TraceHitTargets)
{
	// �ٸ� Ŭ��鵵 �ش� ���� Ŭ�� ��� ��ǰ� ����Ʈ�� �����ϹǷ� �ٸ� ��� Ŭ��� �������� ���
	if (Character && Character->IsLocallyControlled() && !Character->HasAuthority()) return;

	LocalShotgunFire(TraceHitTargets);
}

void UCombatComponent::ServerReload_Implementation()
{
	// �������� �������� �ϴ� �ൿ����
	// Ŭ��� CombatState�� �����ϸ鼭 OnRep_CombatState()�� ���� ����
	CombatState = ECombatState::ECS_Reloading;
	// �������� �����ϴ� ��� �� �� ȣ��Ǵ� �� ����. �̹� Reload���� ȣ�� ����
	if(Character && !Character->IsLocallyControlled()) HandleReload();
}

void UCombatComponent::ServerThrowGrenade_Implementation()
{
	if (Grenades <= 0) return;
	if (!Character) return;

	// .���¸� �����ؼ� �ٸ� Ŭ��鵵 OnRep_CombatState�� ���ؼ� ����ź�� ���� �� �ְ���
	CombatState = ECombatState::ECS_ThrowingGrenade;

	Character->PlayThrowGrenadeMontage();

	AttachActorToLeftHand(EquippedWeapon);

	ShowAttachedGrenade(true);

	// ���������� �߰��� ����ź�� ������ �����Ѵ�. �� ������ ���ø����̼ǵǴ� ������ Ŭ��鵵 �˰Ե�
	Grenades = FMath::Clamp(Grenades - 1, 0, MaxGrenades);
	UpdateHUDGrenades();
}

void UCombatComponent::ServerLaunchGrenade_Implementation(const FVector_NetQuantize& Target)
{
	if (Character && Character->GetAttachedGrenade() && GrenadeClass)
	{
		if (UWorld* World = GetWorld())
		{
			const FVector StartingLocation = Character->GetAttachedGrenade()->GetComponentLocation();
			// ������ ���ڼ� ����
			const FVector ToTarget = Target - StartingLocation;
			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = Character;
			SpawnParams.Instigator = Character;

			World->SpawnActor<AProjectile>(GrenadeClass, StartingLocation, ToTarget.Rotation(), SpawnParams);
		}
	}
}

void UCombatComponent::FinishReloading()
{
	// �ִϸ��̼� �������Ʈ���� ���ε� ��� ������ ȣ���� �Լ�
	if (!Character) return;

	bLocallyReloading = false;

	// ź�� ������ ���������� �۵�
	if (Character->HasAuthority())
	{
		CombatState = ECombatState::ECS_Unoccupied;

		// �׼� �����鼭 ź�� �� ����
		UpdateAmmoValues();
	}

	// CombatState �ٲ������ �߻� ��ư ������ �� ������ ���� �� �־ �ۼ�
	if (bFireButtonPressed)
	{
		Fire();
	}
}

void UCombatComponent::ShotgunShellReload()
{
	// ���� ź�� ä��� �� ���������� �ߵ�
	if (Character && Character->HasAuthority())
	{
		UpdateShotgunAmmoValues();
	}
}

void UCombatComponent::ThrowGrenadeFinished()
{
	CombatState = ECombatState::ECS_Unoccupied;

	// �޼����� ��� ����� ���� �ٽ� ����������
	AttachActorToRightHand(EquippedWeapon);
}

void UCombatComponent::LaunchGrenade()
{
	ShowAttachedGrenade(false);

	// HitTarget ������ ����������, �׸��� ���� Ŭ�󿡼��� ���ȴ�.
	if (Character && Character->IsLocallyControlled())
	{
		// ����ź ������ ��������
		ServerLaunchGrenade(HitTarget);
	}
}

void UCombatComponent::FinishSwapAttachWeapons()
{
	if (!EquippedWeapon) return;
	if (!SecondaryWeapon) return;

	std::swap(EquippedWeapon, SecondaryWeapon);

	// ���� �������⸦ �����տ� �����ϰ� HUD ������Ʈ
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
	AttachActorToRightHand(EquippedWeapon);
	EquippedWeapon->SetHUDAmmo();
	UpdateCarriedAmmo();
	PlayEquipWeaponSound(EquippedWeapon);

	// ���� ���ι��⸦ �賶�� ����
	SecondaryWeapon->SetWeaponState(EWeaponState::EWS_EquippedSecondary);
	AttachActorToBackpack(SecondaryWeapon);
}

void UCombatComponent::FinishSwap()
{
	CombatState = ECombatState::ECS_Unoccupied;

	if (Character)
	{
		Character->SetIsbFinishedSwapping(true);
	}

	if (SecondaryWeapon)
	{
		SecondaryWeapon->EnableCustomDepth(true);
	}
}

void UCombatComponent::EquipWeapon(AWeapon* WeaponToEquip)
{
	if (!WeaponToEquip) return;
	if (!Character) return;
	if (CombatState != ECombatState::ECS_Unoccupied) return;

	// ����� ������ ��� ó��
	if (WeaponToEquip->GetWeaponType() == EWeaponType::EWT_Flag)
	{
		// ����� ������ ���� �÷��̾ ������ ���δ�.
		Character->Crouch();

		bHoldingTheFlag = true;
		WeaponToEquip->SetWeaponState(EWeaponState::EWS_Equipped);
		AttachFlagToLeftHand(WeaponToEquip);
		WeaponToEquip->SetOwner(Character);
		TheFlag = WeaponToEquip;
		return;
	}

	bCanFire = true;

	// ���� ���Ⱑ �ְ� ���� ���Ⱑ ������ ���� ���⸦ �賶�� ����
	if (EquippedWeapon && !SecondaryWeapon)
	{
		EquipSecondaryWeapon(WeaponToEquip);
	}
	// ���� ���Ⱑ ������ ����. �� ���Ⱑ �� ������ ����ִ� ���⸦ ����߸��� �� ����� ����
	else
	{
		EquipPrimaryWeapon(WeaponToEquip);
	}

	// ���⸦ �� �Ŀ��� ������ �����ϸ鼭 �̵��ϵ��� ��
	if (Character->GetCharacterMovement())
	{
		Character->GetCharacterMovement()->bOrientRotationToMovement = false;
		Character->bUseControllerRotationYaw = true;
	}
}

void UCombatComponent::SwapWeapons()
{
	if (!Character) return;
	if (CombatState != ECombatState::ECS_Unoccupied) return;

	// �������� ����Ǵ� ��
	Character->PlaySwapMontage();
	CombatState = ECombatState::ECS_SwappingWeapons;
	Character->SetIsbFinishedSwapping(false);

	if (SecondaryWeapon)
	{
		SecondaryWeapon->EnableCustomDepth(false);
	}
}

void UCombatComponent::SetAiming(bool bIsAiming)
{
	if (!Character) return;
	if (!EquippedWeapon) return;

	// �ڽ��� ���� �ڼ��� �ڽ��� Ȯ�� �����ϳ�, �ٸ� Ŭ�󿡰Դ� ������ �ʴ´�.
	bAiming = bIsAiming;

	// Ŭ���̾�Ʈ�� ��� �ٸ� Ŭ�� ���� �ڼ��� Ȯ���ϵ��� ������ ������ ó���ϵ��� ��û
	// �ش� �Լ��� Server�� �����ؼ� Ŭ�󿡼� ȣ���ϸ� �������� ȣ������
	ServerSetAiming(bIsAiming);

	if (Character->GetCharacterMovement())
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
	}

	// ��Ʈ���ϴ� ���ο��Ը� �������� �������� ������
	if (Character->IsLocallyControlled() && EquippedWeapon->GetWeaponType() == EWeaponType::EWT_SniperRifle)
	{
		Character->ShowSniperScopeWidget(bIsAiming);
	}

	if (Character->IsLocallyControlled()) bAimButtonPressed = bIsAiming;
}

void UCombatComponent::Reload()
{
	if (CarriedAmmo > 0 && CombatState == ECombatState::ECS_Unoccupied && EquippedWeapon && !EquippedWeapon->IsFull() && !bLocallyReloading)
	{
		// ���� Ŭ�󿡼� �����ؾ��� ���� �������� �ñ��� �ʰ� �ٷ���. ���� ���̱� ����
		HandleReload();
		// �������� ó���ؾ��� ���� ��û
		ServerReload();

		// �ִ� �ν��Ͻ����� ��� ó���� ���� ���
		bLocallyReloading = true;
	}
}

void UCombatComponent::FireButtonPressed(bool bPressed)
{
	bFireButtonPressed = bPressed;

	// �������� �߻縦 ó��. �׳� ���⿡�� ó���ϸ� �ش� Ŭ���̾�Ʈ������ ����
	if (bFireButtonPressed)
	{
		Fire();
	}
}

void UCombatComponent::JumpToShotgunEnd()
{
	if (!Character) return;
	if (!Character->GetMesh()) return;
	if (!Character->GetReloadMontage()) return;

	UAnimInstance* AnimInstance = Character->GetMesh()->GetAnimInstance();
	if (AnimInstance && Character->GetReloadMontage())
	{
		AnimInstance->Montage_JumpToSection(TEXT("ShotgunEnd"));
	}
}

void UCombatComponent::ThrowGrenade()
{
	if (Grenades <= 0) return;
	if (!Character) return;
	if (!EquippedWeapon) return;
	if (CombatState != ECombatState::ECS_Unoccupied) return;
	
	// ĳ���Ϳ��� ��ư �����鼭 ȣ���ϴ� �Լ��� ���� Ŭ���̾�Ʈ���� �����
	CombatState = ECombatState::ECS_ThrowingGrenade;

	Character->PlayThrowGrenadeMontage();

	// ����ִ� ���⸦ �޼�����
	AttachActorToLeftHand(EquippedWeapon);

	ShowAttachedGrenade(true);

	// Ŭ�󿡼� ȣ���ߴٸ� ���������� �˵��� ȣ��
	if (!Character->HasAuthority())
	{
		ServerThrowGrenade();
	}

	// ���������� �߰��� ����ź�� ������ �����Ѵ�. �� ������ ���ø����̼ǵǴ� ������ Ŭ��鵵 �˰Ե�
	if (Character->HasAuthority())
	{
		Grenades = FMath::Clamp(Grenades - 1, 0, MaxGrenades);
		UpdateHUDGrenades();
	}
}

void UCombatComponent::PickupAmmo(EWeaponType WeaponType, int32 AmmoAmount)
{
	if (CarriedAmmoMap.Contains(WeaponType))
	{
		CarriedAmmoMap[WeaponType] = FMath::Clamp(CarriedAmmoMap[WeaponType] + AmmoAmount, 0, MaxCarriedAmmo);
		UpdateCarriedAmmo();
	}

	// ź���� �Ծ��µ� ź���� ����ִ� ���¸� �ڵ� ������
	if (EquippedWeapon && EquippedWeapon->IsEmpty() && EquippedWeapon->GetWeaponType() == WeaponType)
	{
		Reload();
	}
}

bool UCombatComponent::ShouldSwapWeapons() const
{
	return EquippedWeapon && SecondaryWeapon;
}

void UCombatComponent::TraceUnderCrosshairs(FHitResult& TraceHitResult)
{
	if (!GEngine) return;
	if (!GEngine->GameViewport) return;
	if (!GetWorld()) return;

	FVector2D ViewportSize;
	GEngine->GameViewport->GetViewportSize(ViewportSize);

	const FVector2D CrosshairLocation(ViewportSize.X / 2.0, ViewportSize.Y / 2.0);
	FVector CrosshairWorldPosition;
	FVector CrosshairWorldDirection;

	// ����Ʈ ��ǥ�� ���� ��ǥ�� ��ȯ
	bool bScreenToWorld = UGameplayStatics::DeprojectScreenToWorld(UGameplayStatics::GetPlayerController(this, 0), CrosshairLocation, CrosshairWorldPosition, CrosshairWorldDirection);
	if (bScreenToWorld)
	{
		FVector Start = CrosshairWorldPosition;

		// ī�޶�� ĳ���� ������ Ÿ���� ������ ���� ����
		if (Character)
		{
			const double DistanceToCharacter = (Character->GetActorLocation() - Start).Size();
			Start += CrosshairWorldDirection * (DistanceToCharacter + 100.0);
		}

		const FVector End = Start + CrosshairWorldDirection * TRACE_LENGTH;

		GetWorld()->LineTraceSingleByChannel(TraceHitResult, Start, End, ECollisionChannel::ECC_Visibility);

		// �������� ã�� ���Ϳ� �������̽��� �����Ǿ��ִ��� Ȯ��
		if (TraceHitResult.GetActor() && TraceHitResult.GetActor()->Implements<UInteractWithCrosshairsInterface>())
		{
			HUDPackage.CrosshairsColor = FLinearColor::Red;
		}
		else
		{
			HUDPackage.CrosshairsColor = FLinearColor::White;
		}
	}
}

void UCombatComponent::SetHUDCrosshairs(float DeltaTime)
{
	if (!Character) return;
	if (!Character->Controller) return;

	CheckInit();

	if (Controller)
	{
		if (!HUD)
		{
			HUD = Cast<ABlasterHUD>(Controller->GetHUD());
		}

		if (HUD)
		{
			if (EquippedWeapon)
			{
				HUDPackage.CrosshairsCenter = EquippedWeapon->GetCrosshairsCenter();
				HUDPackage.CrosshairsLeft = EquippedWeapon->GetCrosshairsLeft();
				HUDPackage.CrosshairsRight = EquippedWeapon->GetCrosshairsRight();
				HUDPackage.CrosshairsTop = EquippedWeapon->GetCrosshairsTop();
				HUDPackage.CrosshairsBottom = EquippedWeapon->GetCrosshairsBottom();

				// ĳ������ �����ӿ� ���� ���ؼ��� Ȯ��ǵ��� ��
				if (Character->GetCharacterMovement())
				{
					// �̵� �ӵ�(0 ~ 600)�� (0 ~ 1) ������ ������ ����
					const FVector2D WalkSpeedRange(0.f, Character->GetCharacterMovement()->MaxWalkSpeed);
					const FVector2D VelocityMultiplierRange(0.f, 1.f);
					FVector Velocity = Character->GetVelocity();
					Velocity.Z = 0.0;

					CrosshairVelocityFactor = FMath::GetMappedRangeValueClamped(WalkSpeedRange, VelocityMultiplierRange, Velocity.Size());

					// ���߿� ���� ���� �� ���������� ��
					if (Character->GetCharacterMovement()->IsFalling())
					{
						CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 2.25, DeltaTime, 2.25);
					}
					else
					{
						CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 0.0, DeltaTime, 30.0);
					}

					// ���� �߿��� �� ���������� ��
					if (bAiming)
					{
						CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, 0.58, DeltaTime, 30.0);
					}
					else
					{
						CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, 0.0, DeltaTime, 30.0);
					}

					// ���� ��� Fire���� ��� ���� �ø��� ���⼭ 0���� �پ�����
					CrosshairShootingFactor = FMath::FInterpTo(CrosshairShootingFactor, 0.0, DeltaTime, 20.0);

					// ������ �������� �⺻�� + �̵����� ���Ѱ� + ���߿� ������ �߰��� - �������� �پ�簪 + �ѽ� �߰���
					HUDPackage.CrosshairSpread = 0.5f + CrosshairVelocityFactor + CrosshairInAirFactor - CrosshairAimFactor + CrosshairShootingFactor;
				}
			}
			else
			{
				HUDPackage.CrosshairsCenter = nullptr;
				HUDPackage.CrosshairsLeft = nullptr;
				HUDPackage.CrosshairsRight = nullptr;
				HUDPackage.CrosshairsTop = nullptr;
				HUDPackage.CrosshairsBottom = nullptr;
			}

			HUD->SetHUDPackage(HUDPackage);
		}
	}
}

void UCombatComponent::InterpFOV(float DeltaTime)
{
	if (!EquippedWeapon) return;
	if (!Character) return;
	if (!Character->GetFollowCamera()) return;

	// ������ ���� ������ Zoom ������ �����ؼ� Ȯ��
	if (bAiming)
	{
		CurrentFOV = FMath::FInterpTo(CurrentFOV, EquippedWeapon->GetZoomedFOV(), DeltaTime, EquippedWeapon->GetZoomInterpSpeed());
	}
	// �ٽ� ���ư� ���� �⺻������ ������ ������ �ܾƿ�
	else
	{
		CurrentFOV = FMath::FInterpTo(CurrentFOV, DefaultFOV, DeltaTime, ZoomInterpSpeed);
	}

	Character->GetFollowCamera()->SetFieldOfView(CurrentFOV);
}

void UCombatComponent::Fire()
{
	if (!CanFire()) return;

	bCanFire = false;

	// ���� �� �� ���ڼ��� �������� ����
	CrosshairShootingFactor = 0.75;

	if (EquippedWeapon)
	{
		switch (EquippedWeapon->GetFireType())
		{
		case EFireType::EFT_Projectile:
			FireProjectileWeapon();
			break;
		case EFireType::EFT_HitScan:
			FireHitScanWeapon();
			break;
		case EFireType::EFT_Shotgun:
			FireShotgun();
			break;
		default:
			break;
		}
	}

	// �� �߻� ��Ÿ�� üũ
	StartFireTimer();
}

void UCombatComponent::FireProjectileWeapon()
{
	if (EquippedWeapon)
	{
		// �л� ���� ����� �������� �ϰ� �� HitTarget�� ���������ν� ������ Ŭ�� ��� ���� ����� ������ �Ѵ�.
		// �߻�ü ���⵵ UseScatter�� üũ�ϸ� ����ϸ鼭 ������ �� �ִ�.
		HitTarget = EquippedWeapon->GetUseScatter() ? EquippedWeapon->TraceEndWithScatter(HitTarget) : HitTarget;
		
		// ���������� ���� �������. �Ʒ� ServerFire() -> MulticastFire()�ϸ鼭 ��Ƽĳ��Ʈ���� ������ �̹� ������
		if(Character && !Character->HasAuthority()) LocalFire(HitTarget);
		ServerFire(HitTarget, EquippedWeapon->GetFireDelay());
	}
}

void UCombatComponent::FireHitScanWeapon()
{
	if (EquippedWeapon)
	{
		// �л� ���� ����� �������� �ϰ� �� HitTarget�� ���������ν� ������ Ŭ�� ��� ���� ����� ������ �Ѵ�.
		HitTarget = EquippedWeapon->GetUseScatter() ? EquippedWeapon->TraceEndWithScatter(HitTarget) : HitTarget;
		
		if (Character && !Character->HasAuthority()) LocalFire(HitTarget);
		ServerFire(HitTarget, EquippedWeapon->GetFireDelay());
	}
}

void UCombatComponent::FireShotgun()
{
	if (AShotgun* Shotgun = Cast<AShotgun>(EquippedWeapon))
	{
		TArray<FVector_NetQuantize> HitTargets;
		Shotgun->ShotgunTraceEndWithScatter(HitTarget, HitTargets);
		
		if (Character && !Character->HasAuthority()) LocalShotgunFire(HitTargets);
		ServerShotgunFire(HitTargets, Shotgun->GetFireDelay());
	}
}

void UCombatComponent::LocalFire(const FVector_NetQuantize& TraceHitTarget)
{
	// ������ �����ϴ� ���� �÷��̾��� ó��
	// Ŭ���̾�Ʈ�� �������� ���̱����� Ŭ�󿡼� ��ü ó���ص� �Ǵ� �͵��� ����
	if (!Character) return;
	if (!EquippedWeapon) return;
	if (CombatState != ECombatState::ECS_Unoccupied) return;

	// �߻� ��ǰ� �߻� ����Ʈ�� ���� Ŭ�󿡼� ���. ������ ��ٸ� �ʿ䰡 ����
	Character->PlayFireMontage(bAiming);
	EquippedWeapon->Fire(TraceHitTarget);
}

void UCombatComponent::LocalShotgunFire(const TArray<FVector_NetQuantize>& TraceHitTargets)
{
	AShotgun* Shotgun = Cast<AShotgun>(EquippedWeapon);
	if (!Shotgun) return;
	if (!Character) return;
	// ������ ���� �߿� �� �� ����
	if (CombatState == ECombatState::ECS_Unoccupied || CombatState == ECombatState::ECS_Reloading)
	{
		Character->PlayFireMontage(bAiming);
		Shotgun->FireShotgun(TraceHitTargets);
		CombatState = ECombatState::ECS_Unoccupied;
		bLocallyReloading = false;
	}
}

void UCombatComponent::StartFireTimer()
{
	if (!Character) return;
	if (!EquippedWeapon) return;

	// ��ư�� ������ ���� �����ϵ�����
	Character->GetWorldTimerManager().SetTimer(FireTimer, this, &ThisClass::FireTimerFinished, EquippedWeapon->GetFireDelay());
}

void UCombatComponent::FireTimerFinished()
{
	if (!EquippedWeapon) return;
	// FireDelay�� ������ ��ư�� ���� ���¸� �ٽ� ���. �׷��� ���簡 ��
	bCanFire = true;
	if (bFireButtonPressed && EquippedWeapon->IsAutomatic())
	{
		Fire();
	}

	// ���� �߻� �� ź���� ������ �ڵ� ������
	ReloadEmptyWeapon();
}

bool UCombatComponent::CanFire() const
{
	if (!EquippedWeapon) return false;
	if (EquippedWeapon->IsEmpty()) return false;
	if (!bCanFire) return false;

	// ������ ���� �߿� �� �� ����
	if (CombatState == ECombatState::ECS_Reloading && EquippedWeapon->GetWeaponType() == EWeaponType::EWT_ShotGun) return true;

	if (bLocallyReloading) return false;
	if (CombatState != ECombatState::ECS_Unoccupied) return false;
	return true;
}

void UCombatComponent::InitializeCarriedAmmo()
{
	CarriedAmmoMap.Emplace(EWeaponType::EWT_AssaultRifle, StartingARAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_RocketLauncher, StartingRocketAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_Pistol, StartingPistolAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_SubmachineGun, StartingSMGAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_ShotGun, StartingShotgunAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_SniperRifle, StartingSniperAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_GrenaderLauncher, StartingGrenaderLauncherAmmo);
}

void UCombatComponent::CheckInit()
{
	if (!Controller)
	{
		Controller = Cast<ABlasterPlayerController>(Character->Controller);
	}
}

void UCombatComponent::HandleReload()
{
	if (!Character) return;
	// ���ε��ϸ鼭 ����, Ŭ�� �� �ؾ��ϴ� �۾� ����
	Character->PlayReloadMontage();
}

int32 UCombatComponent::AmountToReload() const
{
	if (!EquippedWeapon) return 0;

	// �������� �� ������ ź�� �� ���

	// ���� źâ�� ���� ����
	const int32 RoomInMsg = EquippedWeapon->GetMagCapacity() - EquippedWeapon->GetAmmo();

	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		// �÷��̾ ���� ���� Ÿ���� ź���� ������ �ִ� ��
		const int32 AmountCarried = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
		const int32 Least = FMath::Min(RoomInMsg, AmountCarried);
		return FMath::Clamp(RoomInMsg, 0, Least);
	}
	return 0;
}

void UCombatComponent::UpdateAmmoValues()
{
	if (!EquippedWeapon) return;

	// �������� ź�� ���� ���
	const int32 ReloadAmount = AmountToReload();

	// �޴��ϰ� �ִ� ź�࿡�� ����
	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		CarriedAmmoMap[EquippedWeapon->GetWeaponType()] -= ReloadAmount;
		CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];

		CheckInit();
		if (Controller)
		{
			Controller->SetHUDCarriedAmmo(CarriedAmmo);
		}
	}

	// ������ Ammo ������ ���ø����̼ǵǼ� Ŭ�� �ڵ� �����
	EquippedWeapon->AddAmmo(ReloadAmount);
}

void UCombatComponent::UpdateShotgunAmmoValues()
{
	if (!EquippedWeapon) return;

	// ������ õõ�� 1�߾� �����ϹǷ� ���� ����
	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		CarriedAmmoMap[EquippedWeapon->GetWeaponType()] -= 1;
		CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];

		CheckInit();
		if (Controller)
		{
			Controller->SetHUDCarriedAmmo(CarriedAmmo);
		}
	}

	EquippedWeapon->AddAmmo(1);

	// ������ �ѹ� ������ ������ �ٷ� �� �� ����
	bCanFire = true;

	// ������ �ѹ߾� �����ϴٰ� �����ų� ������ �� ������ �ٷ� End ��Ÿ�ַ� �̵�
	// �� �κ��� ������ ����Ǹ� �ȵ�. �׷��� Ŭ�󿡼��� ���� ȣ��
	if (EquippedWeapon->IsFull() || CarriedAmmo == 0)
	{
		JumpToShotgunEnd();
	}
}

void UCombatComponent::DropEquippedWedapon()
{
	if (EquippedWeapon)
	{
		EquippedWeapon->Dropped();
	}
}

void UCombatComponent::AttachActorToRightHand(AActor* ActorToAttach)
{
	if (!ActorToAttach) return;
	if (!Character) return;
	if (!Character->GetMesh()) return;

	const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(TEXT("RightHandSocket"));
	if (HandSocket)
	{
		HandSocket->AttachActor(ActorToAttach, Character->GetMesh());
	}
}

void UCombatComponent::AttachActorToLeftHand(AActor* ActorToAttach)
{
	if (!ActorToAttach) return;
	if (!Character) return;
	if (!Character->GetMesh()) return;

	const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(TEXT("LeftHandSocket"));
	if (HandSocket)
	{
		HandSocket->AttachActor(ActorToAttach, Character->GetMesh());
	}
}

void UCombatComponent::AttachActorToBackpack(AActor* ActorToAttach)
{
	if (!ActorToAttach) return;
	if (!Character) return;
	if (!Character->GetMesh()) return;

	const USkeletalMeshSocket* BackpackSocket = Character->GetMesh()->GetSocketByName(TEXT("BackpackSocket"));
	if (BackpackSocket)
	{
		BackpackSocket->AttachActor(ActorToAttach, Character->GetMesh());
	}
}

void UCombatComponent::AttachFlagToLeftHand(AWeapon* Flag)
{
	if (!Flag) return;
	if (!Character) return;
	if (!Character->GetMesh()) return;

	const USkeletalMeshSocket* FlagSocket = Character->GetMesh()->GetSocketByName(TEXT("FlagSocket"));
	if (FlagSocket)
	{
		FlagSocket->AttachActor(Flag, Character->GetMesh());
	}
}

void UCombatComponent::UpdateCarriedAmmo()
{
	if (!EquippedWeapon) return;

	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
	}

	CheckInit();
	if (Controller)
	{
		Controller->SetHUDCarriedAmmo(CarriedAmmo);
	}
}

void UCombatComponent::PlayEquipWeaponSound(AWeapon* WeaponToEquip)
{
	if (!WeaponToEquip) return;
	if (!Character) return;

	if (WeaponToEquip->GetEquipSound())
	{
		UGameplayStatics::PlaySoundAtLocation(this, WeaponToEquip->GetEquipSound(), Character->GetActorLocation());
	}
}

void UCombatComponent::ReloadEmptyWeapon()
{
	if (EquippedWeapon && EquippedWeapon->IsEmpty())
	{
		Reload();
	}
}

void UCombatComponent::ShowAttachedGrenade(bool bShowGrenade)
{
	if (Character && Character->GetAttachedGrenade())
	{
		Character->GetAttachedGrenade()->SetVisibility(bShowGrenade);
	}
}

void UCombatComponent::UpdateHUDGrenades()
{
	CheckInit();
	if (Controller)
	{
		Controller->SetHUDGrenades(Grenades);
	}
}

void UCombatComponent::EquipPrimaryWeapon(AWeapon* WeaponToEquip)
{
	if (!WeaponToEquip) return;

	// ���⸦ �̹� ������ �ִ� ��� ���� ����� �����Ų��.
	DropEquippedWedapon();

	// �Ʒ� ������ EquippedWeapon�� ���ø����̼ǵǸ鼭 Ŭ���̾�Ʈ���� ����ȴ�.
	// ������ �Ʒ� ������ ����ǰ� ����� ��, ���� �����Ǽ� ���� ������ �ȵ����� ���ø����̼� Ÿ�ֿ̹� ���� �ٸ�
	// �׷��� �Ʒ� ������ OnRep���� �ѹ� �� �����Ѵ�.
	EquippedWeapon = WeaponToEquip;
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);

	// ���⸦ �����տ� ����
	AttachActorToRightHand(EquippedWeapon);

	// Owner�� �������� �����ϰ� Ŭ�� ���ø����̼ǵǴ� ����
	EquippedWeapon->SetOwner(Character);
	// ������ ź�� ����
	EquippedWeapon->SetHUDAmmo();

	// ���� Ÿ�Կ� ���� ������ �ִ� ź�� ǥ��
	UpdateCarriedAmmo();

	PlayEquipWeaponSound(EquippedWeapon);

	// ���� ���� �� ź���� ������ �ڵ� ������
	ReloadEmptyWeapon();
}

void UCombatComponent::EquipSecondaryWeapon(AWeapon* WeaponToEquip)
{
	if (!WeaponToEquip) return;

	SecondaryWeapon = WeaponToEquip;
	SecondaryWeapon->SetWeaponState(EWeaponState::EWS_EquippedSecondary);

	// ���⸦ ���ѿ� ����
	AttachActorToBackpack(WeaponToEquip);

	SecondaryWeapon->SetOwner(Character);

	PlayEquipWeaponSound(SecondaryWeapon);
}

void UCombatComponent::OnRep_EquippedWeapon()
{
	bCanFire = true;
	// �ٸ� Ŭ���̾�Ʈ������ �������� �� ȸ������ �ʵ��� ��
	if (EquippedWeapon && Character)
	{
		// �Ʒ� ������ EquippedWeapon�� ���ø����̼��� �� ����Ǿ��� ���� ������ Ÿ�ֿ̹� ���� �ȵɼ��� �־ �ѹ� �� ����
		EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);

		AttachActorToRightHand(EquippedWeapon);

		PlayEquipWeaponSound(EquippedWeapon);

		EquippedWeapon->SetHUDAmmo();

		if (Character->GetCharacterMovement())
		{
			Character->GetCharacterMovement()->bOrientRotationToMovement = false;
			Character->bUseControllerRotationYaw = true;
		}
	}
}

void UCombatComponent::OnRep_SecondaryWeapon()
{
	if (SecondaryWeapon && Character)
	{
		SecondaryWeapon->SetWeaponState(EWeaponState::EWS_EquippedSecondary);

		AttachActorToBackpack(SecondaryWeapon);

		PlayEquipWeaponSound(SecondaryWeapon);
	}
}

void UCombatComponent::OnRep_TheFlag()
{
	if (TheFlag)
	{
		TheFlag->SetWeaponState(EWeaponState::EWS_Equipped);
		AttachFlagToLeftHand(TheFlag);
	}
}

void UCombatComponent::OnRep_Aiming()
{
	// Ŭ���̾�Ʈ�� ���� ���� ��� ���� ��ư�� ��� ������ ���� ����
	if (Character && Character->IsLocallyControlled())
	{
		bAiming = bAimButtonPressed;
	}
}

void UCombatComponent::OnRep_CarriedAmmo()
{
	// Ŭ�� ���� �ƽø� ź��� ����
	CheckInit();
	if (Controller)
	{
		Controller->SetHUDCarriedAmmo(CarriedAmmo);
	}

	// ���� ���� �� ������ ź���� ������ End ��Ÿ�ַ� �̵��ϴ� �κ� Ŭ�󿡼� ����
	if (CarriedAmmo == 0 && CombatState == ECombatState::ECS_Reloading && EquippedWeapon && EquippedWeapon->GetWeaponType() == EWeaponType::EWT_ShotGun)
	{
		JumpToShotgunEnd();
	}
}

void UCombatComponent::OnRep_CombatState()
{
	switch (CombatState)
	{
	case ECombatState::ECS_Unoccupied:
		if (bFireButtonPressed)
		{
			Fire();
		}
		break;
	case ECombatState::ECS_Reloading:
		// �����ϰ� �ִ� Ŭ��� �̹� Reload()���� ȣ�� ����. ����� �ٸ� Ŭ����� ���� ����� ������ �ٸ� Ŭ����� ȣ��
		if (Character && !Character->IsLocallyControlled()) HandleReload();
		break;
	case ECombatState::ECS_ThrowingGrenade:
		// ���� Ŭ��� �̹� ��ư �����鼭 �����߰�, �ٸ� Ŭ����� �� �� �ֵ��� �ִϸ��̼� ���
		if (Character && !Character->IsLocallyControlled())
		{
			Character->PlayThrowGrenadeMontage();

			AttachActorToLeftHand(EquippedWeapon);

			ShowAttachedGrenade(true);
		}
		break;
	case ECombatState::ECS_SwappingWeapons:
		// �ٸ� Ŭ����� ���� ���� ����� �� �� �ֵ���
		if (Character && !Character->IsLocallyControlled())
		{
			Character->PlaySwapMontage();
		}
		break;
	default:
		break;
	}
}

void UCombatComponent::OnRep_Grenades()
{
	UpdateHUDGrenades();
}

void UCombatComponent::OnRep_HoldingTheFlag()
{
	if (bHoldingTheFlag && Character && Character->IsLocallyControlled())
	{
		Character->Crouch();
	}
}
