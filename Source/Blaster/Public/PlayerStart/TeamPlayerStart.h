// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerStart.h"
#include "BlasterTypes/Team.h"
#include "TeamPlayerStart.generated.h"

/**
 * 팀별로 시작 위치를 변경할 때 사용할 플레이어 스타트 클래스
 */
UCLASS()
class BLASTER_API ATeamPlayerStart : public APlayerStart
{
	GENERATED_BODY()

public:
	FORCEINLINE ETeam GetTeam() const { return Team; }

private:
	UPROPERTY(EditAnywhere)
	ETeam Team;
};
