// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BuffComponent.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class BLASTER_API UBuffComponent : public UActorComponent
{
	GENERATED_BODY()

private:
	UBuffComponent();
	friend class ABlasterCharacter;

private:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	UFUNCTION(NetMulticast, Reliable)
	void MulticastSpeedBuff(float BaseSpeedBuff, float CrouchSpeedBuff);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastJumpBuff(float JumpZVelocityBuff);

public:
	void Heal(float HealAmount, float HealingTime);
	void BuffSpped(float BaseSpeedBuff, float CrouchSpeedBuff, float SpeedBuffTime);
	void SetInitialSpeeds(float BaseSpeed, float CrouchSpeed);
	void BuffJump(float JumpZVelocityBuff, float JumpBuffTime);
	void SetInitialJumpVelocity(float Velocity);

private:
	void HealRampUp(float DeltaTime);
	void ResetSpeed();
	void ResetJump();

private:
	UPROPERTY()
	TObjectPtr<ABlasterCharacter> Character;

private:
	bool bHealing;
	float HealingRate;
	float AmountToHeal;

	FTimerHandle SpeedBuffTimer;
	float InitialBaseSpeed;
	float InitialCrouchSpeed;

	FTimerHandle JumpBuffTimer;
	float InitialJumpVelocity;
};
