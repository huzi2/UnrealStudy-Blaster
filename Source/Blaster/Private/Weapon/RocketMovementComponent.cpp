// Fill out your copyright notice in the Description page of Project Settings.

#include "Weapon/RocketMovementComponent.h"

URocketMovementComponent::EHandleBlockingHitResult URocketMovementComponent::HandleBlockingHit(const FHitResult& Hit, float TimeTick, const FVector& MoveDelta, float& SubTickTimeRemaining)
{
	Super::HandleBlockingHit(Hit, TimeTick, MoveDelta, SubTickTimeRemaining);
	// 이동 중 다른 오브젝트와 충돌해도 중지하지않고 다음 단계로 넘어가도록함
	return EHandleBlockingHitResult::AdvanceNextSubstep;
}

void URocketMovementComponent::HandleImpact(const FHitResult& Hit, float TimeSlice, const FVector& MoveDelta)
{
	// 로켓은 오로지 충돌했을 때만 멈춰야함. 그래서 여기서 아무것도 안함
}
