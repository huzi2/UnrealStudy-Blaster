// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "BlasterGameState.generated.h"

class ABlasterPlayerState;
/**
 * 
 */
UCLASS()
class BLASTER_API ABlasterGameState : public AGameState
{
	GENERATED_BODY()
	
private:
	ABlasterGameState();

private:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const final;

public:
	FORCEINLINE TArray<ABlasterPlayerState*>& GetTopScoringPlayers() { return TopScoringPlayers; }
	FORCEINLINE TArray<ABlasterPlayerState*>& GetBlueTeam() { return BlueTeam; }
	FORCEINLINE TArray<ABlasterPlayerState*>& GetRedTeam() { return RedTeam; }
	FORCEINLINE float GetBlueTeamScore() const { return BlueTeamScore; }
	FORCEINLINE float GetRedTeamScore() const { return RedTeamScore; }

	void UpdateTopScore(ABlasterPlayerState* ScoringPlayer);
	void BlueTeamScores();
	void RedTeamScores();

private:
	UPROPERTY(Replicated)
	TArray<ABlasterPlayerState*> TopScoringPlayers;

	UPROPERTY(ReplicatedUsing = OnRep_BlueTeamScore)
	float BlueTeamScore;

	UFUNCTION()
	void OnRep_BlueTeamScore();

	UPROPERTY(ReplicatedUsing = OnRep_RedTeamScore)
	float RedTeamScore;

	UFUNCTION()
	void OnRep_RedTeamScore();

private:
	float TopScore;

	TArray<ABlasterPlayerState*> BlueTeam;
	TArray<ABlasterPlayerState*> RedTeam;
};
