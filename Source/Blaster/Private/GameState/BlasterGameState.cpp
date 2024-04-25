// Fill out your copyright notice in the Description page of Project Settings.

#include "GameState/BlasterGameState.h"
#include "Net/UnrealNetwork.h"
#include "PlayerState/BlasterPlayerState.h"
#include "PlayerController/BlasterPlayerController.h"

void ABlasterGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABlasterGameState, TopScoringPlayers);
	DOREPLIFETIME(ABlasterGameState, BlueTeamScore);
	DOREPLIFETIME(ABlasterGameState, RedTeamScore);
}

void ABlasterGameState::UpdateTopScore(ABlasterPlayerState* ScoringPlayer)
{
	if (!ScoringPlayer) return;

	if (TopScoringPlayers.Num() == 0)
	{
		TopScoringPlayers.Add(ScoringPlayer);
		TopScore = ScoringPlayer->GetScore();
	}
	else if(ScoringPlayer->GetScore() == TopScore)
	{
		TopScoringPlayers.AddUnique(ScoringPlayer);
	}
	else if (ScoringPlayer->GetScore() > TopScore)
	{
		TopScoringPlayers.Empty();
		TopScoringPlayers.Add(ScoringPlayer);
		TopScore = ScoringPlayer->GetScore();
	}
}

void ABlasterGameState::BlueTeamScores()
{
	if (!GetWorld()) return;

	++BlueTeamScore;

	if (ABlasterPlayerController* BlasterPlayerController = Cast<ABlasterPlayerController>(GetWorld()->GetFirstPlayerController()))
	{
		BlasterPlayerController->SetHUDBlueTeamScore(BlueTeamScore);
	}
}

void ABlasterGameState::RedTeamScores()
{
	if (!GetWorld()) return;

	++RedTeamScore;

	if (ABlasterPlayerController* BlasterPlayerController = Cast<ABlasterPlayerController>(GetWorld()->GetFirstPlayerController()))
	{
		BlasterPlayerController->SetHUDRedTeamScore(RedTeamScore);
	}
}

void ABlasterGameState::OnRep_BlueTeamScore()
{
	if (!GetWorld()) return;

	if (ABlasterPlayerController* BlasterPlayerController = Cast<ABlasterPlayerController>(GetWorld()->GetFirstPlayerController()))
	{
		BlasterPlayerController->SetHUDBlueTeamScore(BlueTeamScore);
	}
}

void ABlasterGameState::OnRep_RedTeamScore()
{
	if (!GetWorld()) return;

	if (ABlasterPlayerController* BlasterPlayerController = Cast<ABlasterPlayerController>(GetWorld()->GetFirstPlayerController()))
	{
		BlasterPlayerController->SetHUDRedTeamScore(RedTeamScore);
	}
}
