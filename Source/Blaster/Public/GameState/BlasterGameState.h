// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "BlasterGameState.generated.h"

class ABlasterPlayerState;

/**
 * 게임 전체의 탑스코어와 팀별 점수를 관리하는 게임 스테이트 클래스
 */
UCLASS()
class BLASTER_API ABlasterGameState : public AGameState
{
	GENERATED_BODY()

private:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const final;

public:
	FORCEINLINE TArray<ABlasterPlayerState*>& GetTopScoringPlayers() { return TopScoringPlayers; }
	FORCEINLINE TArray<ABlasterPlayerState*>& GetBlueTeam() { return BlueTeam; }
	FORCEINLINE TArray<ABlasterPlayerState*>& GetRedTeam() { return RedTeam; }
	FORCEINLINE float GetBlueTeamScore() const { return BlueTeamScore; }
	FORCEINLINE float GetRedTeamScore() const { return RedTeamScore; }

	// 최고 점수 득점자 확인
	void UpdateTopScore(ABlasterPlayerState* ScoringPlayer);

	// 각 팀 점수 획득
	void BlueTeamScores();
	void RedTeamScores();

private:
	// 가장 높은 점수(중복 시 여러명)
	UPROPERTY(Replicated)
	TArray<ABlasterPlayerState*> TopScoringPlayers;
	float TopScore = 0.f;

	// 각 팀별 점수
	UPROPERTY(ReplicatedUsing = OnRep_BlueTeamScore)
	float BlueTeamScore = 0.f;
	UFUNCTION()
	void OnRep_BlueTeamScore();
	UPROPERTY(ReplicatedUsing = OnRep_RedTeamScore)
	float RedTeamScore = 0.f;
	UFUNCTION()
	void OnRep_RedTeamScore();

	// 각 팀 인원들
	TArray<ABlasterPlayerState*> BlueTeam;
	TArray<ABlasterPlayerState*> RedTeam;
};
