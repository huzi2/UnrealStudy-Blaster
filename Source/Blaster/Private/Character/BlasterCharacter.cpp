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
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "InputActionValue.h"

ABlasterCharacter::ABlasterCharacter()
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

	TurningInPlace = ETurningInPlace::ETIP_NotTurning;

	NetUpdateFrequency = 66.f;
	MinNetUpdateFrequency = 33.f;

	DefaultInputMappingContext = CreateDefaultSubobject<UInputMappingContext>(TEXT("DefaultInputMappingContext"));
	JumpInputAction = CreateDefaultSubobject<UInputAction>(TEXT("JumpInputAction"));
	MoveForwardInputAction = CreateDefaultSubobject<UInputAction>(TEXT("MoveForwardInputAction"));
	MoveRightInputAction = CreateDefaultSubobject<UInputAction>(TEXT("MoveRightInputAction"));
	TurnInputAction = CreateDefaultSubobject<UInputAction>(TEXT("TurnInputAction"));
	LookUpInputAction = CreateDefaultSubobject<UInputAction>(TEXT("LookUpInputAction"));
	EquipInputAction = CreateDefaultSubobject<UInputAction>(TEXT("EquipInputAction"));
	CrouchInputAction = CreateDefaultSubobject<UInputAction>(TEXT("CrouchInputAction"));
	AimInputAction = CreateDefaultSubobject<UInputAction>(TEXT("AimInputAction"));
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

	// �ǽð����� ���ӿ������� ���
	AimOffset(DeltaTime);
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
	}
}

// ���ø�����Ʈ �� �� ȣ��Ǵ� �Լ�
void ABlasterCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// COND_OwnerOnly�� ��ü �����ڸ� ���ø�����Ʈ �ǵ�����
	DOREPLIFETIME_CONDITION(ABlasterCharacter, OverlappingWeapon, COND_OwnerOnly);
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

void ABlasterCharacter::ServerEquipButtonPressed_Implementation()
{
	if (Combat)
	{
		Combat->EquipWeapon(OverlappingWeapon);
	}
}

AWeapon* ABlasterCharacter::GetEquippedWeapon() const
{
	if (!Combat) return nullptr;

	return Combat->EquippedWeapon;
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

void ABlasterCharacter::AimOffset(float DeltaTime)
{
	// ���⸦ ����� ���� ����
	if (!Combat) return;
	if (!Combat->EquippedWeapon) return;
	if (!GetCharacterMovement()) return;

	FVector Velocity = GetVelocity();
	Velocity.Z = 0.0;
	const float Speed = Velocity.Size();
	const bool bIsInAir = GetCharacterMovement()->IsFalling();

	// ������ �� �������� �����¿� �� ���� ����
	if (Speed == 0.f && !bIsInAir)
	{
		bUseControllerRotationYaw = true;

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
	if (Speed > 0.f || bIsInAir)
	{
		bUseControllerRotationYaw = true;

		StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		AO_Yaw = 0.f;

		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
	}

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
