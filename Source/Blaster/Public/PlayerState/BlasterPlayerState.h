// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "BlasterTypes/Team.h"
#include "BlasterPlayerState.generated.h"

class ABlasterCharacter;
class ABlasterPlayerController;

/**
 * 팀과 점수를 관리할 플레이어 스테이트 클래스
 */
UCLASS()
class BLASTER_API ABlasterPlayerState : public APlayerState
{
	GENERATED_BODY()
	
private:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const final;
	virtual void OnRep_Score() final;

public:
	// 팀 관리
	FORCEINLINE ETeam GetTeam() const { return Team; }
	void SetTeam(ETeam TeamToSet);

	// 점수 관리
	void AddToScore(float ScoreAmount);
	void AddToDefeats(int32 DefeatsAmount);

private:
	// 참조 변수 초기화
	void CheckInit();

private:
	UPROPERTY(ReplicatedUsing = OnRep_Team)
	ETeam Team = ETeam::ET_NoTeam;
	UFUNCTION()
	void OnRep_Team();

	UPROPERTY(ReplicatedUsing = OnRep_Defeats)
	int32 Defeats;
	UFUNCTION()
	void OnRep_Defeats();

	// 참조 변수
	UPROPERTY()
	TObjectPtr<ABlasterCharacter> Character;
	UPROPERTY()
	TObjectPtr<ABlasterPlayerController> Controller;
};
