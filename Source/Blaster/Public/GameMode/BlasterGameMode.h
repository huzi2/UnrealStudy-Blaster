// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "BlasterGameMode.generated.h"

class ABlasterCharacter;
class ABlasterPlayerController;
class ABlasterPlayerState;

// 커스텀 매치상태 추가
namespace MatchState
{
	// 매치 사이에 승자를 보여주고 쿨다운 타이머를 작동
	extern BLASTER_API const FName Cooldown;

}

/**
 * 
 */
UCLASS()
class BLASTER_API ABlasterGameMode : public AGameMode
{
	GENERATED_BODY()
	
protected:
	ABlasterGameMode();

private:
	virtual void BeginPlay() final;
	virtual void Tick(float DeltaTime) final;
	virtual void OnMatchStateSet() final;

public:
	FORCEINLINE float GetWarmupTime() const { return WarmupTime; }
	FORCEINLINE float GetMatchTime() const { return MatchTime; }
	FORCEINLINE float GetCooldownTime() const { return CooldownTime; }
	FORCEINLINE float GetCountdownTime() const { return CountdownTime; }
	FORCEINLINE float GetLevelStartingTime() const { return LevelStartingTime; }

	void PlayerEliminated(ABlasterCharacter* ElimmedCharacter, ABlasterPlayerController* VictimController, ABlasterPlayerController* AttackerController);
	void RequestRespawn(ACharacter* ElimmedCharacter, AController* ElimmedController);
	void PlayerLeftGame(ABlasterPlayerState* PlayerLeaving);

private:
	UPROPERTY(EditDefaultsOnly)
	float WarmupTime;

	UPROPERTY(EditDefaultsOnly)
	float MatchTime;

	UPROPERTY(EditDefaultsOnly)
	float CooldownTime;

private:
	float CountdownTime;
	float LevelStartingTime;
};
