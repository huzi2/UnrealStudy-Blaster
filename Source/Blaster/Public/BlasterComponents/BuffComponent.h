// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BuffComponent.generated.h"

class ABlasterCharacter;

/**
 * 캐릭터의 버프 관리 컴포넌트
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
	// 체력 회복
	void Heal(float HealAmount, float HealingTime);
	// 실드 회복
	void ReplenishShield(float ShieldReplenishAmount, float ShieldReplenishTime);
	// 스피드업 버프
	void BuffSpeed(float BaseSpeedBuff, float CrouchSpeedBuff, float SpeedBuffTime);
	UFUNCTION(NetMulticast, Reliable)
	void MulticastSpeedBuff(float BaseSpeedBuff, float CrouchSpeedBuff);
	// 스피드업 원래대로
	void SetInitialSpeeds(float BaseSpeed, float CrouchSpeed);
	// 점프 버프
	void BuffJump(float JumpZVelocityBuff, float JumpBuffTime);
	UFUNCTION(NetMulticast, Reliable)
	void MulticastJumpBuff(float JumpZVelocityBuff);
	// 점프 원래대로
	void SetInitialJumpVelocity(float Velocity);

private:
	// 시간당 치유
	void HealRampUp(float DeltaTime);
	// 시간당 실드회복
	void ShieldReplenishRampUp(float DeltaTime);
	// 스피드업 리셋
	void ResetSpeed();
	// 점프업 리셋
	void ResetJump();

private:
	// 힐 관련 변수
	bool bHealing = false;
	float HealingRate = 0.f;
	float AmountToHeal = 0.f;

	// 실드 관련 변수
	bool bShieldReplenish = false;
	float ShieldReplenishRate = 0.f;
	float AmountToShieldReplenish = 0.f;

	// 스피드 관련 변수
	FTimerHandle SpeedBuffTimer;
	float InitialBaseSpeed;
	float InitialCrouchSpeed;

	// 점프 관련 변수
	FTimerHandle JumpBuffTimer;
	float InitialJumpVelocity;

	// 캐릭터 참조
	UPROPERTY()
	TObjectPtr<ABlasterCharacter> Character;
};
