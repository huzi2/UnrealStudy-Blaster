// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BuffComponent.generated.h"

class ABlasterCharacter;

/**
 * ĳ������ ���� ���� ������Ʈ
 */
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class BLASTER_API UBuffComponent : public UActorComponent
{
	GENERATED_BODY()

private:
	UBuffComponent();
	friend ABlasterCharacter;

private:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) final;

public:
	// ü�� ȸ��
	void Heal(float HealAmount, float HealingTime);
	// �ǵ� ȸ��
	void ReplenishShield(float ShieldReplenishAmount, float ShieldReplenishTime);
	// ���ǵ�� ����
	void BuffSpeed(float BaseSpeedBuff, float CrouchSpeedBuff, float SpeedBuffTime);
	UFUNCTION(NetMulticast, Reliable)
	void MulticastSpeedBuff(float BaseSpeedBuff, float CrouchSpeedBuff);
	// ���ǵ�� �������
	void SetInitialSpeeds(float BaseSpeed, float CrouchSpeed);
	// ���� ����
	void BuffJump(float JumpZVelocityBuff, float JumpBuffTime);
	UFUNCTION(NetMulticast, Reliable)
	void MulticastJumpBuff(float JumpZVelocityBuff);
	// ���� �������
	void SetInitialJumpVelocity(float Velocity);

private:
	// �ð��� ġ��
	void HealRampUp(float DeltaTime);
	// �ð��� �ǵ�ȸ��
	void ShieldReplenishRampUp(float DeltaTime);
	// ���ǵ�� ����
	void ResetSpeed();
	// ������ ����
	void ResetJump();

private:
	// �� ���� ����
	bool bHealing = false;
	float HealingRate = 0.f;
	float AmountToHeal = 0.f;

	// �ǵ� ���� ����
	bool bShieldReplenish = false;
	float ShieldReplenishRate = 0.f;
	float AmountToShieldReplenish = 0.f;

	// ���ǵ� ���� ����
	FTimerHandle SpeedBuffTimer;
	float InitialBaseSpeed;
	float InitialCrouchSpeed;

	// ���� ���� ����
	FTimerHandle JumpBuffTimer;
	float InitialJumpVelocity;

	// ĳ���� ����
	UPROPERTY()
	TObjectPtr<ABlasterCharacter> Character;
};
