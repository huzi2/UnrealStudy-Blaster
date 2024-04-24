// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "BlasterGameMode.generated.h"

class ABlasterCharacter;
class ABlasterPlayerController;
class ABlasterPlayerState;

/**
 * Ŀ���� ��ġ����
 */
namespace MatchState
{
	// ��ġ ���̿� ���ڸ� �����ְ� ��ٿ� Ÿ�̸Ӹ� �۵�
	extern BLASTER_API const FName Cooldown;
}

/**
 * ���� ��ġ���¿� ������, ������ �����ϴ� ���Ӹ�� Ŭ����
 */
UCLASS()
class BLASTER_API ABlasterGameMode : public AGameMode
{
	GENERATED_BODY()
	
protected:
	ABlasterGameMode();

public:
	// ������ ���
	virtual float CalculateDamage(AController* Attacker, AController* Victim, float BaseDamage) const;
	// �÷��̾� ���� �� ���� ���
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

	// �÷��̾� �����
	void RequestRespawn(ACharacter* ElimmedCharacter, AController* ElimmedController);
	// �÷��̾� ���� ����
	void PlayerLeftGame(ABlasterPlayerState* PlayerLeaving);

protected:
	bool bTeamsMatch = false;

private:
	// ���� �غ� �ð�
	UPROPERTY(EditDefaultsOnly)
	float WarmupTime = 10.f;
	// ���� ���� �ð�
	UPROPERTY(EditDefaultsOnly)
	float MatchTime = 120.f;
	// ��ġ�� ��ġ ���� �ð�
	UPROPERTY(EditDefaultsOnly)
	float CooldownTime = 10.f;

	float CountdownTime = 0.f;
	float LevelStartingTime = 0.f;
};
