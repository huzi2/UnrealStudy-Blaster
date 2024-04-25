// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "BlasterGameState.generated.h"

class ABlasterPlayerState;

/**
 * ���� ��ü�� ž���ھ�� ���� ������ �����ϴ� ���� ������Ʈ Ŭ����
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

	// �ְ� ���� ������ Ȯ��
	void UpdateTopScore(ABlasterPlayerState* ScoringPlayer);

	// �� �� ���� ȹ��
	void BlueTeamScores();
	void RedTeamScores();

private:
	// ���� ���� ����(�ߺ� �� ������)
	UPROPERTY(Replicated)
	TArray<ABlasterPlayerState*> TopScoringPlayers;
	float TopScore = 0.f;

	// �� ���� ����
	UPROPERTY(ReplicatedUsing = OnRep_BlueTeamScore)
	float BlueTeamScore = 0.f;
	UFUNCTION()
	void OnRep_BlueTeamScore();
	UPROPERTY(ReplicatedUsing = OnRep_RedTeamScore)
	float RedTeamScore = 0.f;
	UFUNCTION()
	void OnRep_RedTeamScore();

	// �� �� �ο���
	TArray<ABlasterPlayerState*> BlueTeam;
	TArray<ABlasterPlayerState*> RedTeam;
};
