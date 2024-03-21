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

	void UpdateTopScore(ABlasterPlayerState* ScoringPlayer);

private:
	UPROPERTY(Replicated)
	TArray<ABlasterPlayerState*> TopScoringPlayers;

private:
	float TopScore;
};
