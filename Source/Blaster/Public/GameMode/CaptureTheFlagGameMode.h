// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameMode/TeamGameMode.h"
#include "CaptureTheFlagGameMode.generated.h"

class AFlag;
class AFlagZone;

/**
 * 깃발 뺏기 게임모드 클래스
 */
UCLASS()
class BLASTER_API ACaptureTheFlagGameMode : public ATeamGameMode
{
	GENERATED_BODY()

private:
	virtual void PlayerEliminated(ABlasterCharacter* ElimmedCharacter, ABlasterPlayerController* VictimController, ABlasterPlayerController* AttackerController) final;

public:
	// 깃발 뺏기에 따른 점수 계산
	void FlagCapture(AFlag* Flag, AFlagZone* Zone);
};
