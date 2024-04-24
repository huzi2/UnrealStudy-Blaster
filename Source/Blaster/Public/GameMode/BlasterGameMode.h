// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "BlasterGameMode.generated.h"

class ABlasterCharacter;
class ABlasterPlayerController;
class ABlasterPlayerState;

/**
 * 커스텀 매치상태
 */
namespace MatchState
{
	// 매치 사이에 승자를 보여주고 쿨다운 타이머를 작동
	extern BLASTER_API const FName Cooldown;
}

/**
 * 게임 매치상태와 리스폰, 점수를 관리하는 게임모드 클래스
 */
UCLASS()
class BLASTER_API ABlasterGameMode : public AGameMode
{
	GENERATED_BODY()
	
protected:
	ABlasterGameMode();

public:
	// 데미지 계산
	virtual float CalculateDamage(AController* Attacker, AController* Victim, float BaseDamage) const;
	// 플레이어 제거 후 점수 계산
	virtual void PlayerEliminated(ABlasterCharacter* ElimmedCharacter, ABlasterPlayerController* VictimController, ABlasterPlayerController* AttackerController);

private:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void OnMatchStateSet() override;

public:
	FORCEINLINE float GetWarmupTime() const { return WarmupTime; }
	FORCEINLINE float GetMatchTime() const { return MatchTime; }
	FORCEINLINE float GetCooldownTime() const { return CooldownTime; }
	FORCEINLINE float GetCountdownTime() const { return CountdownTime; }
	FORCEINLINE float GetLevelStartingTime() const { return LevelStartingTime; }

	// 플레이어 재생성
	void RequestRespawn(ACharacter* ElimmedCharacter, AController* ElimmedController);
	// 플레이어 게임 떠남
	void PlayerLeftGame(ABlasterPlayerState* PlayerLeaving);

protected:
	bool bTeamsMatch = false;

private:
	// 게임 준비 시간
	UPROPERTY(EditDefaultsOnly)
	float WarmupTime = 10.f;
	// 게임 진행 시간
	UPROPERTY(EditDefaultsOnly)
	float MatchTime = 120.f;
	// 매치와 매치 사이 시간
	UPROPERTY(EditDefaultsOnly)
	float CooldownTime = 10.f;

	float CountdownTime = 0.f;
	float LevelStartingTime = 0.f;
};
