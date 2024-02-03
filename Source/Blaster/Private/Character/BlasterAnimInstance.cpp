// Fill out your copyright notice in the Description page of Project Settings.

#include "Character/BlasterAnimInstance.h"
#include "Character/BlasterCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

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
	}
}
