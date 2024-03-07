// Fill out your copyright notice in the Description page of Project Settings.

#include "BlasterComponents/BuffComponent.h"
#include "Character/BlasterCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

UBuffComponent::UBuffComponent()
	: bHealing(false)
	, HealingRate(0.f)
	, AmountToHeal(0.f)
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UBuffComponent::BeginPlay()
{
	Super::BeginPlay();	
}

void UBuffComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	HealRampUp(DeltaTime);
}

void UBuffComponent::MulticastSpeedBuff_Implementation(float BaseSpeedBuff, float CrouchSpeedBuff)
{
	if (!Character) return;
	if (!Character->GetCharacterMovement()) return;

	Character->GetCharacterMovement()->MaxWalkSpeed = BaseSpeedBuff;
	Character->GetCharacterMovement()->MaxWalkSpeedCrouched = CrouchSpeedBuff;
}

void UBuffComponent::MulticastJumpBuff_Implementation(float JumpZVelocityBuff)
{
	if (!Character) return;
	if (!Character->GetCharacterMovement()) return;

	Character->GetCharacterMovement()->JumpZVelocity = JumpZVelocityBuff;
}

void UBuffComponent::Heal(float HealAmount, float HealingTime)
{
	bHealing = true;

	if (HealingTime == 0.f) HealingTime = 1.f;

	HealingRate = HealAmount / HealingTime;
	AmountToHeal += HealAmount;
}

void UBuffComponent::BuffSpped(float BaseSpeedBuff, float CrouchSpeedBuff, float SpeedBuffTime)
{
	if (!Character) return;
	if (!Character->GetCharacterMovement()) return;

	Character->GetWorldTimerManager().SetTimer(SpeedBuffTimer, this, &ThisClass::ResetSpeed, SpeedBuffTime);

	Character->GetCharacterMovement()->MaxWalkSpeed = BaseSpeedBuff;
	Character->GetCharacterMovement()->MaxWalkSpeedCrouched = CrouchSpeedBuff;

	// 픽업 오브젝트 충돌 처리는 서버에서만 해서 위 내용은 서버만 적용된다. 그래서 모든 클라에게도 적용하기위해 멀티캐스트로 함수를 호출
	MulticastSpeedBuff(BaseSpeedBuff, CrouchSpeedBuff);
}

void UBuffComponent::SetInitialSpeeds(float BaseSpeed, float CrouchSpeed)
{
	InitialBaseSpeed = BaseSpeed;
	InitialCrouchSpeed = CrouchSpeed;
}

void UBuffComponent::BuffJump(float JumpZVelocityBuff, float JumpBuffTime)
{
	if (!Character) return;
	if (!Character->GetCharacterMovement()) return;

	Character->GetWorldTimerManager().SetTimer(JumpBuffTimer, this, &ThisClass::ResetJump, JumpBuffTime);

	Character->GetCharacterMovement()->JumpZVelocity = JumpZVelocityBuff;

	MulticastJumpBuff(JumpZVelocityBuff);
}

void UBuffComponent::SetInitialJumpVelocity(float Velocity)
{
	InitialJumpVelocity = Velocity;
}

void UBuffComponent::HealRampUp(float DeltaTime)
{
	if (!bHealing) return;
	if (!Character) return;
	if (Character->IsElimmed()) return;

	const float HealThisFrame = HealingRate * DeltaTime;
	Character->SetHealth(FMath::Clamp(Character->GetHealth() + HealThisFrame, 0.f, Character->GetMaxHealth()));
	// 서버에서는 OnRep_Health()가 호출이 안되므로 직접 HUD에 표시
	Character->UpdateHUDHealth();

	AmountToHeal -= HealThisFrame;

	if (AmountToHeal <= 0.f || Character->GetHealth() >= Character->GetMaxHealth())
	{
		bHealing = false;
		AmountToHeal = 0.f;
	}
}

void UBuffComponent::ResetSpeed()
{
	if (!Character) return;
	if (!Character->GetCharacterMovement()) return;

	Character->GetCharacterMovement()->MaxWalkSpeed = InitialBaseSpeed;
	Character->GetCharacterMovement()->MaxWalkSpeedCrouched = InitialCrouchSpeed;

	// 스피드 돌아오는 타이머도 서버에서만 세팅해서 클라들은 멀티캐스트로 호출
	MulticastSpeedBuff(InitialBaseSpeed, InitialCrouchSpeed);
}

void UBuffComponent::ResetJump()
{
	if (!Character) return;
	if (!Character->GetCharacterMovement()) return;

	Character->GetCharacterMovement()->JumpZVelocity = InitialJumpVelocity;

	MulticastJumpBuff(InitialJumpVelocity);
}
