// Fill out your copyright notice in the Description page of Project Settings.

#include "Character/BlasterAnimInstance.h"
#include "Character/BlasterCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Weapon/Weapon.h"
#include "BlasterTypes/CombatState.h"

void UBlasterAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	BlasterCharacter = Cast<ABlasterCharacter>(TryGetPawnOwner());
}

void UBlasterAnimInstance::NativeUpdateAnimation(float DeltaTime)
{
	Super::NativeUpdateAnimation(DeltaTime);

	if (!BlasterCharacter)
	{
		BlasterCharacter = Cast<ABlasterCharacter>(TryGetPawnOwner());
	}

	if (!BlasterCharacter) return;
	if (!BlasterCharacter->GetCharacterMovement()) return;

	FVector Velocity = BlasterCharacter->GetVelocity();
	Velocity.Z = 0.0;
	Speed = Velocity.Size();

	bIsInAir = BlasterCharacter->GetCharacterMovement()->IsFalling();
	bIsAccelerating = BlasterCharacter->GetCharacterMovement()->GetCurrentAcceleration().Size() > 0.0 ? true : false;
	bWeaponEquipped = BlasterCharacter->IsWeaponEquipped();
	bIsCrouched = BlasterCharacter->bIsCrouched;
	bAiming = BlasterCharacter->IsAiming();
	TurningInPlace = BlasterCharacter->GetTurningInPlace();
	EquippedWeapon = BlasterCharacter->GetEquippedWeapon();
	bRotateRootBone = BlasterCharacter->ShouldRotateRootBone();
	bElimmed = BlasterCharacter->IsElimmed();

	// ���⸦ �� ���¿��� �¿� �̵� ó��
	const FRotator& AimRotation = BlasterCharacter->GetBaseAimRotation();
	const FRotator& MovementRotation = UKismetMathLibrary::MakeRotFromX(BlasterCharacter->GetVelocity());
	FRotator DeltaRot = UKismetMathLibrary::NormalizedDeltaRotator(MovementRotation, AimRotation);
	// ���� �����̽����� �����ϸ� 180 -> -180 �̵��� �� ������ ��
	// �׷��� RInterpTo���� ��ü������ ����ó��
	DeltaRotation = FMath::RInterpTo(DeltaRotation, DeltaRot, DeltaTime, 6.f);
	YawOffset = DeltaRotation.Yaw;

	// ĳ���� ���� ó��
	// ���� �����̼ǰ����� ���̸� Interp�� ������
	CharacterRotationLastFrame = CharacterRotation;
	CharacterRotation = BlasterCharacter->GetActorRotation();
	const FRotator Delta = UKismetMathLibrary::NormalizedDeltaRotator(CharacterRotation, CharacterRotationLastFrame);
	const float Target = Delta.Yaw / DeltaTime;
	const float Interp = FMath::FInterpTo(Lean, Target, DeltaTime, 6.f);
	Lean = FMath::Clamp(Interp, -90.f, 90.f);

	// ���ӿ����� ó��
	AO_Yaw = BlasterCharacter->GetAOYaw();
	AO_Pitch = BlasterCharacter->GetAOPitch();

	// FABRIK���� ����� Effector Transform ���
	if (bWeaponEquipped && EquippedWeapon && EquippedWeapon->GetWeaponMesh() && BlasterCharacter->GetMesh())
	{
		// �޼��� ���⸦ ���� ��ġ�� ���� ��ǥ�� ����
		LeftHandTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(TEXT("LeftHandSocket"), ERelativeTransformSpace::RTS_World);

		// ������ ������ ��ġ�� �����տ� ������� ��ġ�� ���
		FVector OutPosition;
		FRotator OutRotation;
		BlasterCharacter->GetMesh()->TransformToBoneSpace(TEXT("hand_r"), LeftHandTransform.GetLocation(), FRotator::ZeroRotator, OutPosition, OutRotation);

		// ���� ������� ��ġ�� ���� �޼��� ������ ��ġ�� ��Ȯ�� �µ��� ������Ʈ
		LeftHandTransform.SetLocation(OutPosition);
		LeftHandTransform.SetRotation(FQuat(OutRotation));

		// ������ ���� ���ع������� ���缭 �ѱ��� ���ع���� �Ȱ��� �����ϵ�����
		// RightHandRotation�� �ִ�BP���� ������ ���� ������ �� ���
		// �̷� ���μ����� ��Ʈ���ϴ� Ŭ�� �ϸ��. �������� �� �ʿ� ����.
		if (BlasterCharacter->IsLocallyControlled())
		{
			bLocallyControlled = true;
			const FTransform RightHandTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(TEXT("hand_r"), ERelativeTransformSpace::RTS_World);
			const FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(RightHandTransform.GetLocation(), RightHandTransform.GetLocation() + (RightHandTransform.GetLocation() - BlasterCharacter->GetHitTarget()));
			RightHandRotation = FMath::RInterpTo(RightHandRotation, LookAtRotation, DeltaTime, 30.f);
		}
	}

	// ������, ����ź�� ���� ���� �߿��� FABRIK, ���ӿ����� ������� ����
	bUseFABRIK = BlasterCharacter->GetCombatState() == ECombatState::ECS_Unoccupied;
	// ������ ����� ���� Ŭ�󿡼� ���� �����ϱ� ���� �ﰢ ������. �׷��� ���ε� ����� �������� �ʱ� ���� Ȯ��
	if (BlasterCharacter->IsLocallyControlled() && BlasterCharacter->GetCombatState() != ECombatState::ECS_ThrowingGrenade && BlasterCharacter->GetCombatState() != ECombatState::ECS_SwappingWeapons && BlasterCharacter->IsbFinishedSwapping())
	{
		bUseFABRIK = !BlasterCharacter->IsLocallyReloading();
	}

	bUseAimOffset = BlasterCharacter->GetCombatState() == ECombatState::ECS_Unoccupied && !BlasterCharacter->GetDisableGameplay();
	bTransformRightHand = BlasterCharacter->GetCombatState() == ECombatState::ECS_Unoccupied && !BlasterCharacter->GetDisableGameplay();
}
