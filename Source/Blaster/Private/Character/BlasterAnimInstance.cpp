// Fill out your copyright notice in the Description page of Project Settings.

#include "Character/BlasterAnimInstance.h"
#include "Character/BlasterCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"

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

	FVector Velocity = BlasterCharacter->GetVelocity();
	Velocity.Z = 0.0;
	Speed = Velocity.Size();

	if (BlasterCharacter->GetCharacterMovement())
	{
		bIsInAir = BlasterCharacter->GetCharacterMovement()->IsFalling();
		bIsAccelerating = BlasterCharacter->GetCharacterMovement()->GetCurrentAcceleration().Size() > 0.0 ? true : false;
		bWeaponEquipped = BlasterCharacter->IsWeaponEquipped();
		bIsCrouched = BlasterCharacter->bIsCrouched;
		bAiming = BlasterCharacter->IsAiming();
		
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
	}
}
