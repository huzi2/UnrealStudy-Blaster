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

	// 아래 기능들은 본인 클라만 구현하면됨
	if (Character && Character->IsLocallyControlled())
	{
		// 조준점 방향과 무기 방향을 조절하기 위해 조준점 방향을 얻어내기
		FHitResult HitResult;
		TraceUnderCrosshairs(HitResult);
		HitTarget = HitResult.ImpactPoint;

		// 조준점 세팅
		SetHUDCrosshairs(DeltaTime);

		// 무기 줌인 줌아웃
		InterpFOV(DeltaTime);
	}
}

void UCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// EquippedWeapon은 캐릭터가 RPC로 서버에서만 호출하기에 각각의 클라에서는 nullptr이다
	// 근데 애님인스턴스에서 해당 변수로 무기가 장착되었는지 확인하기에 각 클라에 레플리케이트해야한다.
	DOREPLIFETIME(UCombatComponent, EquippedWeapon);
	DOREPLIFETIME(UCombatComponent, SecondaryWeapon);
	DOREPLIFETIME(UCombatComponent, TheFlag);

	// 조준 자세를 다른 클라도 확인해야해서 레플리케이트
	DOREPLIFETIME(UCombatComponent, bAiming);

	// 운반 탄약은 다른 클라들은 몰라도되고 자신만 알고있으면 되니까 COND_OwnerOnly
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
	// 멀티캐스트는 서버만 호출할 수 있음. 그래서 서버에서 처리
	// 멀티캐스트를 통해 서버에서도 호출되므로 여기서 따로 또 발사를 할 필요는 없다.
	MulticastFire(TraceHitTarget);
}

bool UCombatComponent::ServerFire_Validate(const FVector_NetQuantize& TraceHitTarget, float FireDelay)
{
	if (EquippedWeapon)
	{
		// 실제 발사딜레이와 서버로 넘어온 발사딜레이 값을 비교해서 치팅이 있었는지 확인
		const bool bNearlyEqual = FMath::IsNearlyEqual(EquippedWeapon->GetFireDelay(), FireDelay, 0.001f);
		return bNearlyEqual;
	}
	return true;
}

void UCombatComponent::MulticastFire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{
	// 다른 클라들도 해당 로컬 클라가 쏘는 모션과 이펙트를 봐야하므로 다른 모든 클라와 서버에서 재생
	if (Character && Character->IsLocallyControlled() && !Character->HasAuthority()) return;

	// 클라이언트가 조종하는 경우에 수행
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
		// 실제 발사딜레이와 서버로 넘어온 발사딜레이 값을 비교해서 치팅이 있었는지 확인
		const bool bNearlyEqual = FMath::IsNearlyEqual(EquippedWeapon->GetFireDelay(), FireDelay, 0.001f);
		return bNearlyEqual;
	}
	return true;
}

void UCombatComponent::MulticastShotgunFire_Implementation(const TArray<FVector_NetQuantize>& TraceHitTargets)
{
	// 다른 클라들도 해당 로컬 클라가 쏘는 모션과 이펙트를 봐야하므로 다른 모든 클라와 서버에서 재생
	if (Character && Character->IsLocallyControlled() && !Character->HasAuthority()) return;

	LocalShotgunFire(TraceHitTargets);
}

void UCombatComponent::ServerReload_Implementation()
{
	// 재장전시 서버에서 하는 행동들임
	// 클라는 CombatState를 수정하면서 OnRep_CombatState()를 통해 수행
	CombatState = ECombatState::ECS_Reloading;
	// 서버에서 조종하는 경우 두 번 호출되는 것 방지. 이미 Reload에서 호출 중임
	if(Character && !Character->IsLocallyControlled()) HandleReload();
}

void UCombatComponent::ServerThrowGrenade_Implementation()
{
	if (Grenades <= 0) return;
	if (!Character) return;

	// .상태를 변경해서 다른 클라들도 OnRep_CombatState를 통해서 수류탄을 던질 수 있게함
	CombatState = ECombatState::ECS_ThrowingGrenade;

	Character->PlayThrowGrenadeMontage();

	AttachActorToLeftHand(EquippedWeapon);

	ShowAttachedGrenade(true);

	// 서버에서는 추가로 수류탄의 개수를 조정한다. 이 변수는 레플리케이션되는 변수라 클라들도 알게됨
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
			// 방향은 십자선 기준
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
	// 애니메이션 블루프린트에서 리로드 모션 끝나면 호출할 함수
	if (!Character) return;

	bLocallyReloading = false;

	// 탄약 갱신은 서버에서만 작동
	if (Character->HasAuthority())
	{
		CombatState = ECombatState::ECS_Unoccupied;

		// 액션 끝나면서 탄약 수 갱신
		UpdateAmmoValues();
	}

	// CombatState 바뀌기전에 발사 버튼 눌렀을 때 문제가 생길 수 있어서 작성
	if (bFireButtonPressed)
	{
		Fire();
	}
}

void UCombatComponent::ShotgunShellReload()
{
	// 샷건 탄약 채우는 건 서버에서만 발동
	if (Character && Character->HasAuthority())
	{
		UpdateShotgunAmmoValues();
	}
}

void UCombatComponent::ThrowGrenadeFinished()
{
	CombatState = ECombatState::ECS_Unoccupied;

	// 왼손으로 잠시 들었던 무기 다시 오른손으로
	AttachActorToRightHand(EquippedWeapon);
}

void UCombatComponent::LaunchGrenade()
{
	ShowAttachedGrenade(false);

	// HitTarget 변수는 서버에서만, 그리고 로컬 클라에서만 계산된다.
	if (Character && Character->IsLocallyControlled())
	{
		// 수류탄 생성은 서버에서
		ServerLaunchGrenade(HitTarget);
	}
}

void UCombatComponent::FinishSwapAttachWeapons()
{
	if (!EquippedWeapon) return;
	if (!SecondaryWeapon) return;

	std::swap(EquippedWeapon, SecondaryWeapon);

	// 기존 보조무기를 오른손에 장착하고 HUD 업데이트
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
	AttachActorToRightHand(EquippedWeapon);
	EquippedWeapon->SetHUDAmmo();
	UpdateCarriedAmmo();
	PlayEquipWeaponSound(EquippedWeapon);

	// 기존 메인무기를 배낭에 장착
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

	// 깃발을 집었을 경우 처리
	if (WeaponToEquip->GetWeaponType() == EWeaponType::EWT_Flag)
	{
		// 깃발을 집었을 때는 플레이어를 강제로 숙인다.
		Character->Crouch();

		bHoldingTheFlag = true;
		WeaponToEquip->SetWeaponState(EWeaponState::EWS_Equipped);
		AttachFlagToLeftHand(WeaponToEquip);
		WeaponToEquip->SetOwner(Character);
		TheFlag = WeaponToEquip;
		return;
	}

	bCanFire = true;

	// 메인 무기가 있고 세컨 무기가 없으면 먹은 무기를 배낭에 장착
	if (EquippedWeapon && !SecondaryWeapon)
	{
		EquipSecondaryWeapon(WeaponToEquip);
	}
	// 메인 무기가 없으면 장착. 두 무기가 다 있으면 들고있는 무기를 떨어뜨리고 새 무기로 장착
	else
	{
		EquipPrimaryWeapon(WeaponToEquip);
	}

	// 무기를 낀 후에는 정면을 조준하면서 이동하도록 함
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

	// 서버에서 재생되는 것
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

	// 자신의 조준 자세는 자신이 확인 가능하나, 다른 클라에게는 보이지 않는다.
	bAiming = bIsAiming;

	// 클라이언트의 경우 다른 클라도 조준 자세를 확인하도록 서버가 변수를 처리하도록 요청
	// 해당 함수를 Server로 지정해서 클라에서 호출하면 서버에서 호출해줌
	ServerSetAiming(bIsAiming);

	if (Character->GetCharacterMovement())
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
	}

	// 컨트롤하는 본인에게만 스나이퍼 스코프를 보여줌
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
		// 로컬 클라에서 먼저해야할 일을 서버에게 맡기지 않고 바로함. 렉을 줄이기 위함
		HandleReload();
		// 서버에서 처리해야할 일을 요청
		ServerReload();

		// 애님 인스턴스에서 모션 처리를 위해 사용
		bLocallyReloading = true;
	}
}

void UCombatComponent::FireButtonPressed(bool bPressed)
{
	bFireButtonPressed = bPressed;

	// 서버에서 발사를 처리. 그냥 여기에서 처리하면 해당 클라이언트에서만 보임
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
	
	// 캐릭터에서 버튼 누르면서 호출하는 함수로 로컬 클라이언트에서 실행됨
	CombatState = ECombatState::ECS_ThrowingGrenade;

	Character->PlayThrowGrenadeMontage();

	// 들고있던 무기를 왼손으로
	AttachActorToLeftHand(EquippedWeapon);

	ShowAttachedGrenade(true);

	// 클라에서 호출했다면 서버에서도 알도록 호출
	if (!Character->HasAuthority())
	{
		ServerThrowGrenade();
	}

	// 서버에서는 추가로 수류탄의 개수를 조정한다. 이 변수는 레플리케이션되는 변수라 클라들도 알게됨
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

	// 탄약을 먹었는데 탄약이 비어있는 상태면 자동 재장전
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

	// 뷰포트 좌표를 월드 좌표로 변환
	bool bScreenToWorld = UGameplayStatics::DeprojectScreenToWorld(UGameplayStatics::GetPlayerController(this, 0), CrosshairLocation, CrosshairWorldPosition, CrosshairWorldDirection);
	if (bScreenToWorld)
	{
		FVector Start = CrosshairWorldPosition;

		// 카메라와 캐릭터 사이의 타겟이 잡히는 문제 수정
		if (Character)
		{
			const double DistanceToCharacter = (Character->GetActorLocation() - Start).Size();
			Start += CrosshairWorldDirection * (DistanceToCharacter + 100.0);
		}

		const FVector End = Start + CrosshairWorldDirection * TRACE_LENGTH;

		GetWorld()->LineTraceSingleByChannel(TraceHitResult, Start, End, ECollisionChannel::ECC_Visibility);

		// 라인으로 찾은 액터에 인터페이스가 구현되어있는지 확인
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

				// 캐릭터의 움직임에 따라 조준선이 확산되도록 함
				if (Character->GetCharacterMovement())
				{
					// 이동 속도(0 ~ 600)를 (0 ~ 1) 사이의 값으로 보정
					const FVector2D WalkSpeedRange(0.f, Character->GetCharacterMovement()->MaxWalkSpeed);
					const FVector2D VelocityMultiplierRange(0.f, 1.f);
					FVector Velocity = Character->GetVelocity();
					Velocity.Z = 0.0;

					CrosshairVelocityFactor = FMath::GetMappedRangeValueClamped(WalkSpeedRange, VelocityMultiplierRange, Velocity.Size());

					// 공중에 있을 때는 더 벌어지도록 함
					if (Character->GetCharacterMovement()->IsFalling())
					{
						CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 2.25, DeltaTime, 2.25);
					}
					else
					{
						CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 0.0, DeltaTime, 30.0);
					}

					// 조준 중에는 더 좁혀지도록 함
					if (bAiming)
					{
						CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, 0.58, DeltaTime, 30.0);
					}
					else
					{
						CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, 0.0, DeltaTime, 30.0);
					}

					// 총을 쏘면 Fire에서 즉시 값을 늘리고 여기서 0까지 줄어들게함
					CrosshairShootingFactor = FMath::FInterpTo(CrosshairShootingFactor, 0.0, DeltaTime, 20.0);

					// 조준점 벌어짐은 기본값 + 이동으로 변한값 + 공중에 있을때 추가값 - 조준으로 줄어든값 + 총쏠때 추가값
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

	// 조준할 때는 무기의 Zoom 변수를 적용해서 확대
	if (bAiming)
	{
		CurrentFOV = FMath::FInterpTo(CurrentFOV, EquippedWeapon->GetZoomedFOV(), DeltaTime, EquippedWeapon->GetZoomInterpSpeed());
	}
	// 다시 돌아갈 때는 기본값으로 지정된 변수로 줌아웃
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

	// 총을 쏠 때 십자선이 벌어지는 기준
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

	// 총 발사 쿨타임 체크
	StartFireTimer();
}

void UCombatComponent::FireProjectileWeapon()
{
	if (EquippedWeapon)
	{
		// 분산 공격 계산을 서버에서 하고 그 HitTarget을 공유함으로써 서버와 클라 모두 같은 방사형 공격을 한다.
		// 발사체 무기도 UseScatter을 체크하면 방사하면서 공격할 수 있다.
		HitTarget = EquippedWeapon->GetUseScatter() ? EquippedWeapon->TraceEndWithScatter(HitTarget) : HitTarget;
		
		// 서버에서는 로컬 수행안함. 아래 ServerFire() -> MulticastFire()하면서 멀티캐스트에서 로컬을 이미 수행함
		if(Character && !Character->HasAuthority()) LocalFire(HitTarget);
		ServerFire(HitTarget, EquippedWeapon->GetFireDelay());
	}
}

void UCombatComponent::FireHitScanWeapon()
{
	if (EquippedWeapon)
	{
		// 분산 공격 계산을 서버에서 하고 그 HitTarget을 공유함으로써 서버와 클라 모두 같은 방사형 공격을 한다.
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
	// 공격을 직접하는 로컬 플레이어의 처리
	// 클라이언트의 반응성을 높이기위해 클라에서 자체 처리해도 되는 것들을 수행
	if (!Character) return;
	if (!EquippedWeapon) return;
	if (CombatState != ECombatState::ECS_Unoccupied) return;

	// 발사 모션과 발사 이펙트는 각자 클라에서 재생. 서버를 기다릴 필요가 없음
	Character->PlayFireMontage(bAiming);
	EquippedWeapon->Fire(TraceHitTarget);
}

void UCombatComponent::LocalShotgunFire(const TArray<FVector_NetQuantize>& TraceHitTargets)
{
	AShotgun* Shotgun = Cast<AShotgun>(EquippedWeapon);
	if (!Shotgun) return;
	if (!Character) return;
	// 샷건은 장전 중에 쏠 수 있음
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

	// 버튼을 누르는 동안 연사하도록함
	Character->GetWorldTimerManager().SetTimer(FireTimer, this, &ThisClass::FireTimerFinished, EquippedWeapon->GetFireDelay());
}

void UCombatComponent::FireTimerFinished()
{
	if (!EquippedWeapon) return;
	// FireDelay가 끝나고 버튼을 누른 상태면 다시 쏜다. 그래서 연사가 됨
	bCanFire = true;
	if (bFireButtonPressed && EquippedWeapon->IsAutomatic())
	{
		Fire();
	}

	// 무기 발사 후 탄약이 없으면 자동 재장전
	ReloadEmptyWeapon();
}

bool UCombatComponent::CanFire() const
{
	if (!EquippedWeapon) return false;
	if (EquippedWeapon->IsEmpty()) return false;
	if (!bCanFire) return false;

	// 샷건은 장전 중에 쏠 수 있음
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
	// 리로드하면서 서버, 클라가 다 해야하는 작업 정리
	Character->PlayReloadMontage();
}

int32 UCombatComponent::AmountToReload() const
{
	if (!EquippedWeapon) return 0;

	// 장전했을 때 장전할 탄약 수 계산

	// 현재 탄창에 남은 공간
	const int32 RoomInMsg = EquippedWeapon->GetMagCapacity() - EquippedWeapon->GetAmmo();

	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		// 플레이어가 현재 무기 타입의 탄약을 가지고 있는 양
		const int32 AmountCarried = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
		const int32 Least = FMath::Min(RoomInMsg, AmountCarried);
		return FMath::Clamp(RoomInMsg, 0, Least);
	}
	return 0;
}

void UCombatComponent::UpdateAmmoValues()
{
	if (!EquippedWeapon) return;

	// 재장전할 탄약 수를 계산
	const int32 ReloadAmount = AmountToReload();

	// 휴대하고 있는 탄약에서 제거
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

	// 무기의 Ammo 변수는 레플리케이션되서 클라에 자동 적용됨
	EquippedWeapon->AddAmmo(ReloadAmount);
}

void UCombatComponent::UpdateShotgunAmmoValues()
{
	if (!EquippedWeapon) return;

	// 샷건은 천천히 1발씩 장전하므로 따로 구현
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

	// 샷건은 한발 충전할 때마다 바로 쏠 수 있음
	bCanFire = true;

	// 샷건은 한발씩 장전하다가 다차거나 장전할 게 없으면 바로 End 몽타주로 이동
	// 이 부분은 서버만 적용되면 안됨. 그래서 클라에서도 따로 호출
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

	// 무기를 이미 가지고 있는 경우 이전 무기는 드랍시킨다.
	DropEquippedWedapon();

	// 아래 내용은 EquippedWeapon이 레플리케이션되면서 클라이언트에게 복사된다.
	// 하지만 아래 내용이 적용되고 복사될 지, 복사 먼저되서 내용 적용이 안될지는 레플리케이션 타이밍에 따라 다름
	// 그래서 아래 내용은 OnRep에서 한번 더 수행한다.
	EquippedWeapon = WeaponToEquip;
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);

	// 무기를 오른손에 붙임
	AttachActorToRightHand(EquippedWeapon);

	// Owner은 서버에서 세팅하고 클라에 레플리케이션되는 변수
	EquippedWeapon->SetOwner(Character);
	// 서버에 탄약 적용
	EquippedWeapon->SetHUDAmmo();

	// 무기 타입에 따라 가지고 있는 탄약 표시
	UpdateCarriedAmmo();

	PlayEquipWeaponSound(EquippedWeapon);

	// 무기 장착 후 탄약이 없으면 자동 재장전
	ReloadEmptyWeapon();
}

void UCombatComponent::EquipSecondaryWeapon(AWeapon* WeaponToEquip)
{
	if (!WeaponToEquip) return;

	SecondaryWeapon = WeaponToEquip;
	SecondaryWeapon->SetWeaponState(EWeaponState::EWS_EquippedSecondary);

	// 무기를 백팩에 붙임
	AttachActorToBackpack(WeaponToEquip);

	SecondaryWeapon->SetOwner(Character);

	PlayEquipWeaponSound(SecondaryWeapon);
}

void UCombatComponent::OnRep_EquippedWeapon()
{
	bCanFire = true;
	// 다른 클라이언트에서도 무기들었을 때 회전하지 않도록 함
	if (EquippedWeapon && Character)
	{
		// 아래 내용은 EquippedWeapon을 레플리케이션할 때 적용되었을 수도 있지만 타이밍에 따라 안될수도 있어서 한번 더 적용
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
	// 클라이언트가 렉이 심할 경우 에임 버튼이 계속 눌리는 문제 수정
	if (Character && Character->IsLocallyControlled())
	{
		bAiming = bAimButtonPressed;
	}
}

void UCombatComponent::OnRep_CarriedAmmo()
{
	// 클라에 무기 맥시멈 탄약수 설정
	CheckInit();
	if (Controller)
	{
		Controller->SetHUDCarriedAmmo(CarriedAmmo);
	}

	// 샷건 장전 중 장전할 탄약이 없으면 End 몽타주로 이동하는 부분 클라에서 수행
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
		// 조종하고 있는 클라는 이미 Reload()에서 호출 중임. 여기는 다른 클라들이 장전 모션을 보도록 다른 클라들이 호출
		if (Character && !Character->IsLocallyControlled()) HandleReload();
		break;
	case ECombatState::ECS_ThrowingGrenade:
		// 로컬 클라는 이미 버튼 누르면서 수행했고, 다른 클라들이 알 수 있도록 애니메이션 재생
		if (Character && !Character->IsLocallyControlled())
		{
			Character->PlayThrowGrenadeMontage();

			AttachActorToLeftHand(EquippedWeapon);

			ShowAttachedGrenade(true);
		}
		break;
	case ECombatState::ECS_SwappingWeapons:
		// 다른 클라들이 무기 스왑 모션을 볼 수 있도록
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
