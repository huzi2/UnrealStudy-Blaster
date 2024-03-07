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

	// �Ⱦ� ������Ʈ �浹 ó���� ���������� �ؼ� �� ������ ������ ����ȴ�. �׷��� ��� Ŭ�󿡰Ե� �����ϱ����� ��Ƽĳ��Ʈ�� �Լ��� ȣ��
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
	// ���������� OnRep_Health()�� ȣ���� �ȵǹǷ� ���� HUD�� ǥ��
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

	// ���ǵ� ���ƿ��� Ÿ�̸ӵ� ���������� �����ؼ� Ŭ����� ��Ƽĳ��Ʈ�� ȣ��
	MulticastSpeedBuff(InitialBaseSpeed, InitialCrouchSpeed);
}

void UBuffComponent::ResetJump()
{
	if (!Character) return;
	if (!Character->GetCharacterMovement()) return;

	Character->GetCharacterMovement()->JumpZVelocity = InitialJumpVelocity;

	MulticastJumpBuff(InitialJumpVelocity);
}
