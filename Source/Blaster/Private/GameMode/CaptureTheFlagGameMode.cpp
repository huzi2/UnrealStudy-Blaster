// Fill out your copyright notice in the Description page of Project Settings.

#include "GameMode/CaptureTheFlagGameMode.h"
#include "Weapon/Flag.h"
#include "CaptureTheFlag/FlagZone.h"
#include "GameState/BlasterGameState.h"

void ACaptureTheFlagGameMode::PlayerEliminated(ABlasterCharacter* ElimmedCharacter, ABlasterPlayerController* VictimController, ABlasterPlayerController* AttackerController)
{
	// 개인 점수는 체크한다.
	ABlasterGameMode::PlayerEliminated(ElimmedCharacter, VictimController, AttackerController);

	// 깃발뺏기에서는 팀점수는 깃발로만 올릴 수 있음. 제거로는 올라가지 않는다. 그래서 아래 코드에서는 아무것도 안함
}

void ACaptureTheFlagGameMode::FlagCapture(AFlag* Flag, AFlagZone* Zone)
{
	if (!Flag) return;
	if (!Zone) return;

	if (ABlasterGameState* BlasterGameState = Cast<ABlasterGameState>(GameState))
	{
		if (Zone->GetTeam() == ETeam::ET_BlueTeam)
		{
			BlasterGameState->BlueTeamScores();
		}
		else if (Zone->GetTeam() == ETeam::ET_RedTeam)
		{
			BlasterGameState->RedTeamScores();
		}
	}
}
