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

	// 무기를 든 상태에서 좌우 이동 처리
	const FRotator& AimRotation = BlasterCharacter->GetBaseAimRotation();
	const FRotator& MovementRotation = UKismetMathLibrary::MakeRotFromX(BlasterCharacter->GetVelocity());
	FRotator DeltaRot = UKismetMathLibrary::NormalizedDeltaRotator(MovementRotation, AimRotation);
	// 블렌드 스페이스에서 보간하면 180 -> -180 이동할 때 문제가 됨
	// 그래서 RInterpTo으로 자체적으로 보간처리
	DeltaRotation = FMath::RInterpTo(DeltaRotation, DeltaRot, DeltaTime, 6.f);
	YawOffset = DeltaRotation.Yaw;

	// 캐릭터 기울기 처리
	// 이전 로테이션값과의 차이를 Interp로 보정함
	CharacterRotationLastFrame = CharacterRotation;
	CharacterRotation = BlasterCharacter->GetActorRotation();
	const FRotator Delta = UKismetMathLibrary::NormalizedDeltaRotator(CharacterRotation, CharacterRotationLastFrame);
	const float Target = Delta.Yaw / DeltaTime;
	const float Interp = FMath::FInterpTo(Lean, Target, DeltaTime, 6.f);
	Lean = FMath::Clamp(Interp, -90.f, 90.f);

	// 에임오프셋 처리
	AO_Yaw = BlasterCharacter->GetAOYaw();
	AO_Pitch = BlasterCharacter->GetAOPitch();

	// FABRIK에서 사용할 Effector Transform 계산
	if (bWeaponEquipped && EquippedWeapon && EquippedWeapon->GetWeaponMesh() && BlasterCharacter->GetMesh())
	{
		// 왼손이 무기를 잡을 위치를 월드 좌표로 구함
		LeftHandTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(TEXT("LeftHandSocket"), ERelativeTransformSpace::RTS_World);

		// 가져온 소켓의 위치를 오른손에 상대적인 위치로 계산
		FVector OutPosition;
		FRotator OutRotation;
		BlasterCharacter->GetMesh()->TransformToBoneSpace(TEXT("hand_r"), LeftHandTransform.GetLocation(), FRotator::ZeroRotator, OutPosition, OutRotation);

		// 계산된 상대적인 위치를 통해 왼손이 소켓의 위치에 정확히 맞도록 업데이트
		LeftHandTransform.SetLocation(OutPosition);
		LeftHandTransform.SetRotation(FQuat(OutRotation));

		// 오른손 본을 조준방향으로 맞춰서 총구를 조준방향과 똑같이 설정하도록함
		// RightHandRotation은 애님BP에서 오른손 본을 수정할 때 사용
		// 이런 세부설정은 컨트롤하는 클라만 하면됨. 나머지는 할 필요 없다.
		if (BlasterCharacter->IsLocallyControlled())
		{
			bLocallyControlled = true;
			const FTransform RightHandTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(TEXT("hand_r"), ERelativeTransformSpace::RTS_World);
			const FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(RightHandTransform.GetLocation(), RightHandTransform.GetLocation() + (RightHandTransform.GetLocation() - BlasterCharacter->GetHitTarget()));
			RightHandRotation = FMath::RInterpTo(RightHandRotation, LookAtRotation, DeltaTime, 30.f);
		}
	}

	// 재장전, 수류탄과 같은 상태 중에는 FABRIK, 에임오프셋 사용하지 않음
	bUseFABRIK = BlasterCharacter->GetCombatState() == ECombatState::ECS_Unoccupied;
	// 재장전 모션은 로컬 클라에서 렉을 방지하기 위해 즉각 시전함. 그래서 리로딩 모션을 방해하지 않기 위해 확인
	if (BlasterCharacter->IsLocallyControlled() && BlasterCharacter->GetCombatState() != ECombatState::ECS_ThrowingGrenade && BlasterCharacter->GetCombatState() != ECombatState::ECS_SwappingWeapons && BlasterCharacter->IsbFinishedSwapping())
	{
		bUseFABRIK = !BlasterCharacter->IsLocallyReloading();
	}

	bUseAimOffset = BlasterCharacter->GetCombatState() == ECombatState::ECS_Unoccupied && !BlasterCharacter->GetDisableGameplay();
	bTransformRightHand = BlasterCharacter->GetCombatState() == ECombatState::ECS_Unoccupied && !BlasterCharacter->GetDisableGameplay();
}
