// Fill out your copyright notice in the Description page of Project Settings.

#include "GameMode/CaptureTheFlagGameMode.h"
#include "Weapon/Flag.h"
#include "CaptureTheFlag/FlagZone.h"
#include "GameState/BlasterGameState.h"

void ACaptureTheFlagGameMode::PlayerEliminated(ABlasterCharacter* ElimmedCharacter, ABlasterPlayerController* VictimController, ABlasterPlayerController* AttackerController)
{
	// ���� ������ üũ�Ѵ�.
	ABlasterGameMode::PlayerEliminated(ElimmedCharacter, VictimController, AttackerController);

	// ��߻��⿡���� �������� ��߷θ� �ø� �� ����. ���ŷδ� �ö��� �ʴ´�. �׷��� �Ʒ� �ڵ忡���� �ƹ��͵� ����
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
