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

UCombatComponent::UCombatComponent()
	: BaseWalkSpeed(600.f)
	, AimWalkSpeed(450.f)
	, ZoomedFOV(30.f)
	, ZoomInterpSpeed(20.f)
	, bCanFire(true)
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	if (Character && Character->GetCharacterMovement())
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;

		if (Character->GetFollowCamera())
		{
			DefaultFOV = Character->GetFollowCamera()->FieldOfView;
			CurrentFOV = DefaultFOV;
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

	// ���� �ڼ��� �ٸ� Ŭ�� Ȯ���ؾ��ؼ� ���ø�����Ʈ
	DOREPLIFETIME(UCombatComponent, bAiming);
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
	// ��Ƽĳ��Ʈ�� ������ ȣ���� �� ����. �׷��� �������� ó��
	// ��Ƽĳ��Ʈ�� ���� ���������� ȣ��ǹǷ� ���⼭ ���� �� �߻縦 �� �ʿ�� ����.
	MulticastFire(TraceHitTarget);
}

void UCombatComponent::MulticastFire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{
	if (!Character) return;
	if (!EquippedWeapon) return;

	// �߻� ��ǰ� �߻� ����Ʈ�� ��� Ŭ���̾�Ʈ���� ���̵��� ��Ƽĳ��Ʈ
	// ��Ƽĳ��Ʈ�� ����, Ŭ�� ��ο��� �߻� ����
	Character->PlayFireMontage(bAiming);
	EquippedWeapon->Fire(TraceHitTarget);
}

void UCombatComponent::EquipWeapon(AWeapon* WeaponToEquip)
{
	if (!Character || !WeaponToEquip) return;

	bCanFire = true;

	// �Ʒ� ������ EquippedWeapon�� ���ø����̼ǵǸ鼭 Ŭ���̾�Ʈ���� ����ȴ�.
	// ������ �Ʒ� ������ ����ǰ� ����� ��, ���� �����Ǽ� ���� ������ �ȵ����� ���ø����̼� Ÿ�ֿ̹� ���� �ٸ�
	// �׷��� �Ʒ� ������ OnRep���� �ѹ� �� �����Ѵ�.
	EquippedWeapon = WeaponToEquip;
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);

	const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(TEXT("RightHandSocket"));
	if (HandSocket)
	{
		HandSocket->AttachActor(EquippedWeapon, Character->GetMesh());
	}

	EquippedWeapon->SetOwner(Character);

	// ���⸦ �� �Ŀ��� ������ �����ϸ鼭 �̵��ϵ��� ��
	if (Character->GetCharacterMovement())
	{
		Character->GetCharacterMovement()->bOrientRotationToMovement = false;
		Character->bUseControllerRotationYaw = true;
	}
}

void UCombatComponent::SetAiming(bool bIsAiming)
{
	// �ڽ��� ���� �ڼ��� �ڽ��� Ȯ�� �����ϳ�, �ٸ� Ŭ�󿡰Դ� ������ �ʴ´�.
	bAiming = bIsAiming;

	// Ŭ���̾�Ʈ�� ��� �ٸ� Ŭ�� ���� �ڼ��� Ȯ���ϵ��� ������ ������ ó���ϵ��� ��û
	// �ش� �Լ��� Server�� �����ؼ� Ŭ�󿡼� ȣ���ϸ� �������� ȣ������
	ServerSetAiming(bIsAiming);

	if (Character && Character->GetCharacterMovement())
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
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

	if (!Controller)
	{
		Controller = Cast<ABlasterPlayerController>(Character->Controller);
	}

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
	if (!bCanFire) return;

	bCanFire = false;

	ServerFire(HitTarget);

	// ���� �� �� ���ڼ��� �������� ����
	CrosshairShootingFactor = 0.75;

	// ���� �߻��ϸ� ���� �ð� ���� ���� �߻�
	StartFireTimer();
}

void UCombatComponent::StartFireTimer()
{
	if (!EquippedWeapon) return;
	if (!Character) return;

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
}

void UCombatComponent::OnRep_EquippedWeapon()
{
	bCanFire = true;
	// �ٸ� Ŭ���̾�Ʈ������ �������� �� ȸ������ �ʵ��� ��
	if (EquippedWeapon && Character)
	{
		// �Ʒ� ������ EquippedWeapon�� ���ø����̼��� �� ����Ǿ��� ���� ������ Ÿ�ֿ̹� ���� �ȵɼ��� �־ �ѹ� �� ����
		EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);

		const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(TEXT("RightHandSocket"));
		if (HandSocket)
		{
			HandSocket->AttachActor(EquippedWeapon, Character->GetMesh());
		}

		if (Character->GetCharacterMovement())
		{
			Character->GetCharacterMovement()->bOrientRotationToMovement = false;
			Character->bUseControllerRotationYaw = true;
		}
	}
}
