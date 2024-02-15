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
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "InputActionValue.h"

ABlasterCharacter::ABlasterCharacter()
	: CameraThreshlod(200.0)
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

	// 카메라가 메쉬나 캡슐과 충돌해서 줌인되는 것을 방지
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);
	// 캡슐이 아닌 메시가 충돌하도록(헤드샷 등의 구현을 위해) 커스텀 오브젝트 채널로 설정
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

	// 실시간으로 에임오프셋을 계산
	AimOffset(DeltaTime);

	// 카메라와 캐릭터가 너무 가까워지면 캐릭터를 숨김
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

// 레플리케이트 될 때 호출되는 함수
void ABlasterCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// COND_OwnerOnly은 객체 소유자만 레플리케이트 되도록함
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

void ABlasterCharacter::MulticastHit_Implementation()
{
	PlayHitReactMontage();
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

FVector ABlasterCharacter::GetHitTarget() const
{
	if(!Combat) return FVector();

	return Combat->HitTarget;
}

void ABlasterCharacter::SetOverlappingWeapon(AWeapon* Weapon)
{
	// 충돌 처리는 서머만 하도록 해서 이 함수는 서버만 들어옴
	// 클라들은 레플리케이트 처리되지만 서버는 여기서 처리
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
		// 서버면 바로 무기 장착
		if (HasAuthority())
		{
			Combat->EquipWeapon(OverlappingWeapon);
		}
		// 클라면 서버에게 요청
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
	// 무기를 들었을 때만 적용
	if (!Combat) return;
	if (!Combat->EquippedWeapon) return;
	if (!GetCharacterMovement()) return;

	FVector Velocity = GetVelocity();
	Velocity.Z = 0.0;
	const float Speed = Velocity.Size();
	const bool bIsInAir = GetCharacterMovement()->IsFalling();

	// 가만히 서 있을때는 상하좌우 다 조준 가능
	if (Speed == 0.f && !bIsInAir)
	{
		bUseControllerRotationYaw = true;

		FRotator CurrentAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		FRotator DeltaAimRotation = UKismetMathLibrary::NormalizedDeltaRotator(CurrentAimRotation, StartingAimRotation);
		AO_Yaw = DeltaAimRotation.Yaw;
		// 몸체를 회전할 때 사용하기 위해 보정용 변수 저장
		if (TurningInPlace == ETurningInPlace::ETIP_NotTurning)
		{
			InterpAO_Yaw = AO_Yaw;
		}

		// -90이나 90 끝에 다다랐을 경우 몸 자체를 회전시켜야함
		TurnInPlace(DeltaTime);
	}
	// 뛰거나 점프할 때는 위아래로만 조준 가능
	if (Speed > 0.f || bIsInAir)
	{
		bUseControllerRotationYaw = true;

		StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		AO_Yaw = 0.f;

		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
	}

	AO_Pitch = GetBaseAimRotation().Pitch;

	// 피치값의 경우 -90 ~ 0도는 서버 패킷으로 전송되는 과정에서 270 ~ 360도로 변경된다.
	// 그래서 자신이 컨트롤하는 캐릭터 외의 캐릭터의 피치값은 보정을 해준다.
	if (AO_Pitch > 90.f && !IsLocallyControlled())
	{
		const FVector2D InRange(270.f, 360.f);
		const FVector2D OutRange(-90.f, 0.f);
		AO_Pitch = FMath::GetMappedRangeValueClamped(InRange, OutRange, AO_Pitch);
	}
}

void ABlasterCharacter::TurnInPlace(float DeltaTime)
{
	// 어디로 회전할 지 정하고 애니메이션 블루프린트에서 확인
	if (AO_Yaw > 90.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_Right;
	}
	else if (AO_Yaw < -90.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_Left;
	}

	// 실제 회전값 수정
	if (TurningInPlace != ETurningInPlace::ETIP_NotTurning)
	{
		// 회전하기위해 보정값을 15도까지 낮춤
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
	// 본인 클라만 구현하면됨. 다른 클라들은 캐릭터가 숨겨지면 안됨
	if (!IsLocallyControlled()) return;
	if (!FollowCamera) return;
	if (!GetMesh()) return;

	// 카메라가 너무 가까우면
	if ((FollowCamera->GetComponentLocation() - GetActorLocation()).Size() < CameraThreshlod)
	{
		// 메쉬를 숨김
		GetMesh()->SetVisibility(false);

		// 무기도 숨김
		if (Combat && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponMesh())
		{
			// Owner가 못보는 옵션
			Combat->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = true;
		}
	}
	else
	{
		GetMesh()->SetVisibility(true);

		// 무기도 숨김
		if (Combat && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponMesh())
		{
			Combat->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = false;
		}
	}
}

void ABlasterCharacter::OnRep_OverlappingWeapon(AWeapon* LastWeapon)
{
	// 레플리케이트되기 전 무기는 감춘다
	if (LastWeapon)
	{
		LastWeapon->ShowPickupWidget(false);
	}

	// 레플리케이트된 무기를 보여줌
	if (OverlappingWeapon)
	{
		OverlappingWeapon->ShowPickupWidget(true);
	}
}
