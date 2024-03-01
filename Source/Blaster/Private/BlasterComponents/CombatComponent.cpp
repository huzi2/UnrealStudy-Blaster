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

UCombatComponent::UCombatComponent()
	: BaseWalkSpeed(600.f)
	, AimWalkSpeed(450.f)
	, ZoomedFOV(30.f)
	, ZoomInterpSpeed(20.f)
	, StartingARAmmo(30)
	, StartingRocketAmmo(0)
	, StartingPistolAmmo(0)
	, StartingSMGAmmo(0)
	, StartingShotgunAmmo(0)
	, StartingSniperAmmo(0)
	, CombatState(ECombatState::ECS_Unoccupied)
	, bCanFire(true)
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

	// 조준 자세를 다른 클라도 확인해야해서 레플리케이트
	DOREPLIFETIME(UCombatComponent, bAiming);

	// 운반 탄약은 다른 클라들은 몰라도되고 자신만 알고있으면 되니까 COND_OwnerOnly
	DOREPLIFETIME_CONDITION(UCombatComponent, CarriedAmmo, COND_OwnerOnly);

	DOREPLIFETIME(UCombatComponent, CombatState);
}

void UCombatComponent::ServerSetAiming_Implementation(bool bIsAiming)
{
	bAiming = bIsAiming;

	if (Character && Character->GetCharacterMovement())
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
	}
}

void UCombatComponent::ServerFire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{
	// 멀티캐스트는 서버만 호출할 수 있음. 그래서 서버에서 처리
	// 멀티캐스트를 통해 서버에서도 호출되므로 여기서 따로 또 발사를 할 필요는 없다.
	MulticastFire(TraceHitTarget);
}

void UCombatComponent::MulticastFire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{
	if (!Character) return;
	if (!EquippedWeapon) return;
	if (CombatState != ECombatState::ECS_Unoccupied) return;

	// 발사 모션과 발사 이펙트가 모든 클라이언트에서 보이도록 멀티캐스트
	// 멀티캐스트로 서버, 클라 모두에서 발사 실행
	Character->PlayFireMontage(bAiming);
	EquippedWeapon->Fire(TraceHitTarget);
}

void UCombatComponent::ServerReload_Implementation()
{
	// 재장전시 서버에서 하는 행동들임
	// 클라는 CombatState를 수정하면서 OnRep_CombatState()를 통해 수행
	CombatState = ECombatState::ECS_Reloading;
	HandleReload();
}

void UCombatComponent::FinishReloading()
{
	// 애니메이션 블루프린트에서 리로드 모션 끝나면 호출할 함수
	if (!Character) return;
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

void UCombatComponent::EquipWeapon(AWeapon* WeaponToEquip)
{
	if (!Character || !WeaponToEquip) return;

	// 무기를 이미 가지고 있는 경우 이전 무기는 드랍시킨다.
	if (EquippedWeapon)
	{
		EquippedWeapon->Dropped();
	}

	bCanFire = true;

	// 아래 내용은 EquippedWeapon이 레플리케이션되면서 클라이언트에게 복사된다.
	// 하지만 아래 내용이 적용되고 복사될 지, 복사 먼저되서 내용 적용이 안될지는 레플리케이션 타이밍에 따라 다름
	// 그래서 아래 내용은 OnRep에서 한번 더 수행한다.
	EquippedWeapon = WeaponToEquip;
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);

	const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(TEXT("RightHandSocket"));
	if (HandSocket)
	{
		HandSocket->AttachActor(EquippedWeapon, Character->GetMesh());
	}

	// Owner은 서버에서 세팅하고 클라에 레플리케이션되는 변수
	EquippedWeapon->SetOwner(Character);
	// 서버에 탄약 적용
	EquippedWeapon->SetHUDAmmo();

	// 서버에 무기의 맥시멈 탄약 수 설정
	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
	}

	CheckInit();
	if (Controller)
	{
		Controller->SetHUDCarriedAmmo(CarriedAmmo);
	}

	if (EquippedWeapon->GetEquipSound())
	{
		UGameplayStatics::PlaySoundAtLocation(this, EquippedWeapon->GetEquipSound(), Character->GetActorLocation());
	}

	// 무기 장착 후 탄약이 없으면 자동 재장전
	if (EquippedWeapon->IsEmpty())
	{
		Reload();
	}

	// 무기를 낀 후에는 정면을 조준하면서 이동하도록 함
	if (Character->GetCharacterMovement())
	{
		Character->GetCharacterMovement()->bOrientRotationToMovement = false;
		Character->bUseControllerRotationYaw = true;
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
}

void UCombatComponent::Reload()
{
	if (CarriedAmmo > 0 && CombatState != ECombatState::ECS_Reloading)
	{
		ServerReload();
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

	ServerFire(HitTarget);

	// 총을 쏠 때 십자선이 벌어지는 기준
	CrosshairShootingFactor = 0.75;

	// 총을 발사하면 일정 시간 동안 지속 발사
	StartFireTimer();
}

void UCombatComponent::StartFireTimer()
{
	if (!EquippedWeapon) return;
	if (!Character) return;

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
	if (EquippedWeapon->IsEmpty())
	{
		Reload();
	}
}

bool UCombatComponent::CanFire() const
{
	if (!EquippedWeapon) return false;
	if (EquippedWeapon->IsEmpty()) return false;
	if (!bCanFire) return false;
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

void UCombatComponent::OnRep_EquippedWeapon()
{
	bCanFire = true;
	// 다른 클라이언트에서도 무기들었을 때 회전하지 않도록 함
	if (EquippedWeapon && Character)
	{
		// 아래 내용은 EquippedWeapon을 레플리케이션할 때 적용되었을 수도 있지만 타이밍에 따라 안될수도 있어서 한번 더 적용
		EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);

		const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(TEXT("RightHandSocket"));
		if (HandSocket)
		{
			HandSocket->AttachActor(EquippedWeapon, Character->GetMesh());
		}

		if (EquippedWeapon->GetEquipSound())
		{
			UGameplayStatics::PlaySoundAtLocation(this, EquippedWeapon->GetEquipSound(), Character->GetActorLocation());
		}

		if (Character->GetCharacterMovement())
		{
			Character->GetCharacterMovement()->bOrientRotationToMovement = false;
			Character->bUseControllerRotationYaw = true;
		}
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
		HandleReload();
		break;
	default:
		break;
	}
}
