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
	// �Ʒ� �Լ����� ����-Ŭ�� �ð��� ����ȭ�ϱ� ����
	// �������� ���� ���� �ð��� ��û
	UFUNCTION(Server, Reliable)
	void ServerRequestServerTime(float TimeOfClientRequest);

	// Ŭ�󿡰� ���� ���� �ð��� �˷���. Ŭ��� �ش� �������� ����-Ŭ�� �ð� ���̸� Ȯ��
	UFUNCTION(Client, Reliable)
	void ClientReportServerTime(float TimeOfClientRequest, float TimeServerReceivedClientRequest);

	// ������ ��ġ ���¸� Ȯ��
	UFUNCTION(Server, Reliable)
	void ServerCheckMatchState();

	// �������� Ȯ���� ��ġ ���¸� Ŭ��鿡�� �˸�
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
	
	// ���� ����� ���� ��ġ�� �˱� ���� ����. Ŭ��� ���� ��带 Ȯ���� �� ��� ���ø����̼��ؼ� ���
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

	// Ŭ���̾�Ʈ�� ������ �ð� ����
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
