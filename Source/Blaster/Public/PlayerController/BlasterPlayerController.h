// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "BlasterPlayerController.generated.h"

class ABlasterGameMode;
class ABlasterHUD;
/**
 * 
 */
UCLASS()
class BLASTER_API ABlasterPlayerController : public APlayerController
{
	GENERATED_BODY()

private:
	ABlasterPlayerController();

private:
	virtual void BeginPlay() final;
	virtual void OnPossess(APawn* InPawn) final;
	virtual void Tick(float DeltaTime) final;
	virtual void ReceivedPlayer() final;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const final;

private:
	// 아래 함수들은 서버-클라 시간을 동기화하기 위함
	// 서버에게 현재 서버 시간을 요청
	UFUNCTION(Server, Reliable)
	void ServerRequestServerTime(float TimeOfClientRequest);

	// 클라에게 현재 서버 시간을 알려줌. 클라는 해당 내용으로 서버-클라 시간 차이를 확인
	UFUNCTION(Client, Reliable)
	void ClientReportServerTime(float TimeOfClientRequest, float TimeServerReceivedClientRequest);

	// 서버의 매치 상태를 확인
	UFUNCTION(Server, Reliable)
	void ServerCheckMatchState();

	// 서버에서 확인한 매치 상태를 클라들에게 알림
	UFUNCTION(Client, Reliable)
	void ClientJoinMidGame(const FName& StateOfMatch, float Warmup, float Match, float Cooldown, float StartingTime);

public:
	void SetHUDHealth(float Health, float MaxHealth);
	void SetHUDShield(float Shield, float MaxShield);
	void SetHUDScore(float Score);
	void SetHUDDefeats(int32 Defeats);
	void SetHUDWeaponAmmo(int32 Ammo);
	void SetHUDCarriedAmmo(int32 Ammo);
	void SetHUDMatchCountdown(float CounddownTime);
	void SetHUDAnnouncementCountdown(float CounddownTime);
	void SetHUDGrenades(int32 Grenades);
	float GetServerTime() const;
	void OnMatchStateSet(const FName& State);

private:
	void HUDInit();
	void SetHUDTime();
	void CheckTimeSync(float DeltaTime);
	void PollInit();
	void HandleMatchHasStarted();
	void HandleCooldown();

private:
	UPROPERTY(EditAnywhere, Category = "Time")
	float TimeSyncFrequency;
	
	// 게임 모드의 현재 매치를 알기 위한 변수. 클라는 게임 모드를 확인할 수 없어서 레플리케이션해서 사용
	UPROPERTY(ReplicatedUsing = OnRep_MatchState)
	FName MatchState;

	UFUNCTION()
	void OnRep_MatchState();

	UPROPERTY()
	TObjectPtr<ABlasterGameMode> BlasterGameMode;

	UPROPERTY()
	TObjectPtr<ABlasterHUD> BlasterHUD;

private:
	float WarmupTime;
	float MatchTime;
	float CooldownTime;
	float LevelStartingTime;
	uint32 CounddownInt;

	// 클라이언트와 서버의 시간 차이
	float ClientServerDelta;

	float TimeSyncRunningTime;

	bool bInitializeCharacterOverlay;
	float HUDHealth;
	float HUDMaxHealth;
	float HUDShield;
	float HUDMaxShield;
	float HUDScore;
	int32 HUDDefeats;
	int32 HUDGrenades;
};
