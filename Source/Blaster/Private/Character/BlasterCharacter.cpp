// Fill out your copyright notice in the Description page of Project Settings.

#include "Character/BlasterCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/WidgetComponent.h"
#include "Net/UnrealNetwork.h"
#include "Weapon/Weapon.h"
#include "BlasterComponents/CombatComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Character/BlasterAnimInstance.h"
#include "Blaster/Blaster.h"
#include "PlayerController/BlasterPlayerController.h"
#include "GameMode/BlasterGameMode.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "InputActionValue.h"

ABlasterCharacter::ABlasterCharacter()
	: CameraThreshlod(200.0)
	, MaxHealth(100.f)
	, Health(100.f)
	, TurnThreshold(0.5f)
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetMesh());
	CameraBoom->TargetArmLength = 600.f;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;

	OverheadWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("OverheadWidget"));
	OverheadWidget->SetupAttachment(GetRootComponent());

	Combat = CreateDefaultSubobject<UCombatComponent>(TEXT("Combat"));
	Combat->SetIsReplicated(true);

	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0, 0.0, 850.0);

	// ī�޶� �޽��� ĸ���� �浹�ؼ� ���εǴ� ���� ����
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);
	// ĸ���� �ƴ� �޽ð� �浹�ϵ���(��弦 ���� ������ ����) Ŀ���� ������Ʈ ä�η� ����
	GetMesh()->SetCollisionObjectType(ECC_SkeletalMesh);

	TurningInPlace = ETurningInPlace::ETIP_NotTurning;

	NetUpdateFrequency = 66.f;
	MinNetUpdateFrequency = 33.f;
}

void ABlasterCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultInputMappingContext, 0);
		}
	}

	BlasterPlayerController = Cast<ABlasterPlayerController>(Controller);
	UpdateHUDHealth();

	// ������ ó���� ������
	if (HasAuthority())
	{
		OnTakeAnyDamage.AddDynamic(this, &ThisClass::ReceiveDamage);
	}
}

void ABlasterCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (Combat)
	{
		Combat->Character = this;
	}
}

void ABlasterCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// ������ �����ϰų� ������ ��� ���ӵ��� ��ü�� ������
	if (GetLocalRole() > ENetRole::ROLE_SimulatedProxy && IsLocallyControlled())
	{
		// �ǽð����� ���ӿ������� ���
		AimOffset(DeltaTime);
	}
	else
	{
		// �������� �ʹ� ���� ������ �ȵǸ� ���� ������
		TimeSinceLastMovementReplication += DeltaTime;
		if (TimeSinceLastMovementReplication > 0.25f)
		{
			OnRep_ReplicatedMovement();
		}
		CalculateAO_Pitch();
	}

	// ī�޶�� ĳ���Ͱ� �ʹ� ��������� ĳ���͸� ����
	HideCameraIfCharacterClose();
}

void ABlasterCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		Input->BindAction(JumpInputAction, ETriggerEvent::Triggered, this, &ThisClass::Jump);
		Input->BindAction(MoveForwardInputAction, ETriggerEvent::Triggered, this, &ThisClass::MoveForward);
		Input->BindAction(MoveRightInputAction, ETriggerEvent::Triggered, this, &ThisClass::MoveRight);
		Input->BindAction(TurnInputAction, ETriggerEvent::Triggered, this, &ThisClass::Turn);
		Input->BindAction(LookUpInputAction, ETriggerEvent::Triggered, this, &ThisClass::LookUp);
		Input->BindAction(EquipInputAction, ETriggerEvent::Triggered, this, &ThisClass::EquipButtonPressed);
		Input->BindAction(CrouchInputAction, ETriggerEvent::Started, this, &ThisClass::CrouchButtonPressed);
		Input->BindAction(AimInputAction, ETriggerEvent::Started, this, &ThisClass::AimButtonPressed);
		Input->BindAction(AimInputAction, ETriggerEvent::Completed, this, &ThisClass::AimButtonReleased);
		Input->BindAction(FireInputAction, ETriggerEvent::Started, this, &ThisClass::FireButtonPressed);
		Input->BindAction(FireInputAction, ETriggerEvent::Completed, this, &ThisClass::FireButtonReleased);
	}
}

// ���ø�����Ʈ �� �� ȣ��Ǵ� �Լ�
void ABlasterCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// COND_OwnerOnly�� ��ü �����ڸ� ���ø�����Ʈ �ǵ�����
	DOREPLIFETIME_CONDITION(ABlasterCharacter, OverlappingWeapon, COND_OwnerOnly);

	DOREPLIFETIME(ABlasterCharacter, Health);
}

void ABlasterCharacter::Jump()
{
	if (bIsCrouched)
	{
		UnCrouch();
	}
	else
	{
		Super::Jump();
	}
}

void ABlasterCharacter::OnRep_ReplicatedMovement()
{
	// �� ƽ���� �ݾ�����Ʈ�� �̷�������ʴ´�.
	// �׷��� �̵��� ���õ� ƽ ���� ����� �̰����� �ؼ� �����Ʈ�� ���ŵ� ������ �����ų �� �ִ�.
	Super::OnRep_ReplicatedMovement();

	// Tick���� ������ �ڽ��� Ŭ��� ���ӵ��� ��ü�� ���� ����������
	// ������ Ŭ����� ��Ʈ���� �������� �ʰ� ���ӵ��� ��ü ȸ�� ��Ų��.
	// �����Ʈ�� ���õ� ����̹Ƿ� Tick���� �ϴ°� �ƴ϶� ���⼭ ����
	SimProxiesTurn();
	TimeSinceLastMovementReplication = 0.f;
}

void ABlasterCharacter::ServerEquipButtonPressed_Implementation()
{
	if (Combat)
	{
		Combat->EquipWeapon(OverlappingWeapon);
	}
}

void ABlasterCharacter::ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatorController, AActor* DamageCauser)
{
	// ������ ó���� ���������� �����. ���� ������� �������� ����ȴ�.
	Health = FMath::Clamp(Health - Damage, 0.f, MaxHealth);

	UpdateHUDHealth();
	PlayHitReactMontage();

	// ü���� 0�̵Ǹ� ���Ӹ�忡�� ĳ���� ����
	// ���Ӹ��� ������ �������� �����ϱ⿡ Ŭ�󿡼� ȣ���ϸ� null�̴�.
	// ������ ������ ó�� ��ü�� ���������� ȣ��Ǳ����..
	if (Health == 0.f)
	{
		if (GetWorld())
		{
			if (ABlasterGameMode* BlasterGameMode = GetWorld()->GetAuthGameMode<ABlasterGameMode>())
			{
				ABlasterPlayerController* AttackerController = Cast<ABlasterPlayerController>(InstigatorController);
				BlasterGameMode->PlayerEliminated(this, BlasterPlayerController, AttackerController);
			}
		}
	}
}

AWeapon* ABlasterCharacter::GetEquippedWeapon() const
{
	if (!Combat) return nullptr;

	return Combat->EquippedWeapon;
}

FVector ABlasterCharacter::GetHitTarget() const
{
	if(!Combat) return FVector();

	return Combat->HitTarget;
}

void ABlasterCharacter::SetOverlappingWeapon(AWeapon* Weapon)
{
	// �浹 ó���� ���Ӹ� �ϵ��� �ؼ� �� �Լ��� ������ ����
	// Ŭ����� ���ø�����Ʈ ó�������� ������ ���⼭ ó��
	if (OverlappingWeapon)
	{
		OverlappingWeapon->ShowPickupWidget(false);
	}

	OverlappingWeapon = Weapon;
	if (IsLocallyControlled())
	{
		if (OverlappingWeapon)
		{
			OverlappingWeapon->ShowPickupWidget(true);
		}
	}
}

bool ABlasterCharacter::IsWeaponEquipped() const
{
	return Combat && Combat->EquippedWeapon;
}

bool ABlasterCharacter::IsAiming() const
{
	return Combat && Combat->bAiming;
}

void ABlasterCharacter::PlayFireMontage(bool bAiming)
{
	if (!Combat) return;
	if (!Combat->EquippedWeapon) return;
	if (!GetMesh()) return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && FireWeaponMontage)
	{
		AnimInstance->Montage_Play(FireWeaponMontage);
		const FName SectionName = bAiming ? TEXT("RifleAim") : TEXT("RifleHip");
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void ABlasterCharacter::PlayHitReactMontage()
{
	if (!Combat) return;
	if (!Combat->EquippedWeapon) return;
	if (!GetMesh()) return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && HitReactMontage)
	{
		AnimInstance->Montage_Play(HitReactMontage);
		//const FName SectionName = bAiming ? TEXT("RifleAim") : TEXT("RifleHip");
		//AnimInstance->Montage_JumpToSection(SectionName);
		AnimInstance->Montage_JumpToSection(TEXT("FromFront"));
	}
}

void ABlasterCharacter::Elim()
{

}

void ABlasterCharacter::MoveForward(const FInputActionValue& Value)
{
	const float MovementValue = Value.Get<float>();

	if (Controller && MovementValue != 0.f)
	{
		const FRotator YawRotation(0.f, GetControlRotation().Yaw, 0.f);
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

		AddMovementInput(Direction, MovementValue);
	}
}

void ABlasterCharacter::MoveRight(const FInputActionValue& Value)
{
	const float MovementValue = Value.Get<float>();

	if (Controller && MovementValue != 0.f)
	{
		const FRotator YawRotation(0.f, GetControlRotation().Yaw, 0.f);
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		AddMovementInput(Direction, MovementValue);
	}
}

void ABlasterCharacter::Turn(const FInputActionValue& Value)
{
	const float MovementValue = Value.Get<float>();

	AddControllerYawInput(MovementValue);
}

void ABlasterCharacter::LookUp(const FInputActionValue& Value)
{
	const float MovementValue = Value.Get<float>();

	AddControllerPitchInput(MovementValue);
}

void ABlasterCharacter::EquipButtonPressed()
{
	if (Combat)
	{
		// ������ �ٷ� ���� ����
		if (HasAuthority())
		{
			Combat->EquipWeapon(OverlappingWeapon);
		}
		// Ŭ��� �������� ��û
		else
		{
			ServerEquipButtonPressed();
		}
	}
}

void ABlasterCharacter::CrouchButtonPressed()
{
	if (bIsCrouched)
	{
		UnCrouch();
	}
	else
	{
		Crouch();
	}
}

void ABlasterCharacter::AimButtonPressed()
{
	if (Combat)
	{
		Combat->SetAiming(true);
	}
}

void ABlasterCharacter::AimButtonReleased()
{
	if (Combat)
	{
		Combat->SetAiming(false);
	}
}

void ABlasterCharacter::FireButtonPressed()
{
	if (Combat)
	{
		Combat->FireButtonPressed(true);
	}
}

void ABlasterCharacter::FireButtonReleased()
{
	if (Combat)
	{
		Combat->FireButtonPressed(false);
	}
}

void ABlasterCharacter::AimOffset(float DeltaTime)
{
	// ���⸦ ����� ���� ����
	if (!Combat) return;
	if (!Combat->EquippedWeapon) return;
	if (!GetCharacterMovement()) return;

	const double Speed = CaculateSpeed();
	const bool bIsInAir = GetCharacterMovement()->IsFalling();

	// ������ �� �������� �����¿� �� ���� ����
	if (Speed == 0.0 && !bIsInAir)
	{
		bUseControllerRotationYaw = true;
		// ��Ʈ���� �������� ��ü�� ������ �ְ� ��ü�� �ٶ󺸴� �������� �����̵��� �Ѵ�.
		bRotateRootBone = true;

		FRotator CurrentAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		FRotator DeltaAimRotation = UKismetMathLibrary::NormalizedDeltaRotator(CurrentAimRotation, StartingAimRotation);
		AO_Yaw = DeltaAimRotation.Yaw;
		// ��ü�� ȸ���� �� ����ϱ� ���� ������ ���� ����
		if (TurningInPlace == ETurningInPlace::ETIP_NotTurning)
		{
			InterpAO_Yaw = AO_Yaw;
		}

		// -90�̳� 90 ���� �ٴٶ��� ��� �� ��ü�� ȸ�����Ѿ���
		TurnInPlace(DeltaTime);
	}
	// �ٰų� ������ ���� ���Ʒ��θ� ���� ����
	if (Speed > 0.0 || bIsInAir)
	{
		bUseControllerRotationYaw = true;
		bRotateRootBone = false;

		StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		AO_Yaw = 0.f;

		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
	}

	// ���Ʒ� ���
	CalculateAO_Pitch();
}

void ABlasterCharacter::SimProxiesTurn()
{
	if (!Combat) return;
	if (!Combat->EquippedWeapon) return;

	bRotateRootBone = false;

	const double Speed = CaculateSpeed();
	if (Speed > 0.0)
	{
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		return;
	}

	// ���� ���� �̻� ȸ���ϸ� ȸ�� �ִϸ��̼��� ����
	ProxyRotationLastFrame = ProxyRotation;
	ProxyRotation = GetActorRotation();
	ProxyYaw = UKismetMathLibrary::NormalizedDeltaRotator(ProxyRotation, ProxyRotationLastFrame).Yaw;

	if (FMath::Abs(ProxyYaw) > TurnThreshold)
	{
		if (ProxyYaw > TurnThreshold)
		{
			TurningInPlace = ETurningInPlace::ETIP_Right;
		}
		else if (ProxyYaw < -TurnThreshold)
		{
			TurningInPlace = ETurningInPlace::ETIP_Left;
		}
		else
		{
			TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		}
		return;
	}
	TurningInPlace = ETurningInPlace::ETIP_NotTurning;
}

void ABlasterCharacter::TurnInPlace(float DeltaTime)
{
	// ���� ȸ���� �� ���ϰ� �ִϸ��̼� �������Ʈ���� Ȯ��
	if (AO_Yaw > 90.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_Right;
	}
	else if (AO_Yaw < -90.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_Left;
	}

	// ���� ȸ���� ����
	if (TurningInPlace != ETurningInPlace::ETIP_NotTurning)
	{
		// ȸ���ϱ����� �������� 15������ ����
		InterpAO_Yaw = FMath::FInterpTo(InterpAO_Yaw, 0.f, DeltaTime, 4.f);
		AO_Yaw = InterpAO_Yaw;

		if (FMath::Abs(AO_Yaw) < 15.f)
		{
			TurningInPlace = ETurningInPlace::ETIP_NotTurning;
			StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		}
	}
}

void ABlasterCharacter::HideCameraIfCharacterClose()
{
	// ���� Ŭ�� �����ϸ��. �ٸ� Ŭ����� ĳ���Ͱ� �������� �ȵ�
	if (!IsLocallyControlled()) return;
	if (!FollowCamera) return;
	if (!GetMesh()) return;

	// ī�޶� �ʹ� ������
	if ((FollowCamera->GetComponentLocation() - GetActorLocation()).Size() < CameraThreshlod)
	{
		// �޽��� ����
		GetMesh()->SetVisibility(false);

		// ���⵵ ����
		if (Combat && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponMesh())
		{
			// Owner�� ������ �ɼ�
			Combat->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = true;
		}
	}
	else
	{
		GetMesh()->SetVisibility(true);

		// ���⵵ ����
		if (Combat && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponMesh())
		{
			Combat->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = false;
		}
	}
}

void ABlasterCharacter::CalculateAO_Pitch()
{
	AO_Pitch = GetBaseAimRotation().Pitch;

	// ��ġ���� ��� -90 ~ 0���� ���� ��Ŷ���� ���۵Ǵ� �������� 270 ~ 360���� ����ȴ�.
	// �׷��� �ڽ��� ��Ʈ���ϴ� ĳ���� ���� ĳ������ ��ġ���� ������ ���ش�.
	if (AO_Pitch > 90.f && !IsLocallyControlled())
	{
		const FVector2D InRange(270.f, 360.f);
		const FVector2D OutRange(-90.f, 0.f);
		AO_Pitch = FMath::GetMappedRangeValueClamped(InRange, OutRange, AO_Pitch);
	}
}

double ABlasterCharacter::CaculateSpeed() const
{
	FVector Velocity = GetVelocity();
	Velocity.Z = 0.0;
	return Velocity.Size();
}

void ABlasterCharacter::UpdateHUDHealth()
{
	if (BlasterPlayerController)
	{
		BlasterPlayerController->SetHUDHealth(Health, MaxHealth);
	}
}

void ABlasterCharacter::OnRep_OverlappingWeapon(AWeapon* LastWeapon)
{
	// ���ø�����Ʈ�Ǳ� �� ����� �����
	if (LastWeapon)
	{
		LastWeapon->ShowPickupWidget(false);
	}

	// ���ø�����Ʈ�� ���⸦ ������
	if (OverlappingWeapon)
	{
		OverlappingWeapon->ShowPickupWidget(true);
	}
}

void ABlasterCharacter::OnRep_Health()
{
	// ������ ó���Ǹ鼭 Health�� ����ǰ� ���ø����̼� �Ǹ鼭 ���� �Լ��� ȣ��ȴ�.
	// �������� ������ ó���ϸ鼭 �ϴ� ������� Ŭ�󿡼��� �Ȱ��� ��
	UpdateHUDHealth();
	PlayHitReactMontage();
}
