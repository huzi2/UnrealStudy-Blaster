// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "BlasterPlayerController.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FHighPingDelegate, bool, bPingTooHigh);

class ABlasterGameMode;
class ABlasterHUD;
class UInputMappingContext;
class UInputAction;
class UReturnToMainMenu;
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
	virtual void SetupInputComponent() final;
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

	// Ŭ���̾�Ʈ�� �� ���¸� �������� �˷���
	UFUNCTION(Server, Reliable)
	void ServerReportPingStatus(bool bHighPing);

public:
	FORCEINLINE float GetSingleTripTime() const { return SingleTripTime; }

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
	void HighPingWarning();
	void StopHighPingWarning();
	void CheckPing(float DeltaTime);
	void ShowReturnToMainMenu();

private:
	UPROPERTY(EditAnywhere, Category = "Time")
	float TimeSyncFrequency;

	UPROPERTY(EditAnywhere, Category = "Ping")
	float HighPingDuration;

	UPROPERTY(EditAnywhere, Category = "Ping")
	float CheckPingFrequency;

	UPROPERTY(EditAnywhere, Category = "Ping")
	float HighPingThreshold;
	
	// ���� ����� ���� ��ġ�� �˱� ���� ����. Ŭ��� ���� ��带 Ȯ���� �� ��� ���ø����̼��ؼ� ���
	UPROPERTY(ReplicatedUsing = OnRep_MatchState)
	FName MatchState;

	UFUNCTION()
	void OnRep_MatchState();

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultInputMappingContext;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> QuitInputAction;

	UPROPERTY(EditAnywhere, Category = "HUD")
	TSubclassOf<UUserWidget> ReturnToMainMenuWidget;

	UPROPERTY()
	TObjectPtr<ABlasterGameMode> BlasterGameMode;

	UPROPERTY()
	TObjectPtr<ABlasterHUD> BlasterHUD;

	UPROPERTY()
	TObjectPtr<UReturnToMainMenu> ReturnToMainMenu;

public:
	FHighPingDelegate HighPingDelegate;

private:
	float WarmupTime;
	float MatchTime;
	float CooldownTime;
	float LevelStartingTime;
	uint32 CounddownInt;

	// Ŭ���̾�Ʈ�� ������ �ð� ����
	float ClientServerDelta;
	// Ŭ�� <-> ���� �ѹ� �����ϴ� �ð�
	float SingleTripTime;

	float TimeSyncRunningTime;

	bool bInitializeCharacterOverlay;
	float HUDHealth;
	float HUDMaxHealth;
	float HUDShield;
	float HUDMaxShield;
	float HUDScore;
	int32 HUDDefeats;
	int32 HUDGrenades;
	int32 HUDWeaponAmmo;
	int32 HUDCarriedAmmo;

	float HighPingRunningTime;
	float PingAnimationRunningTime;

	bool bReturnToMainMenuOpen;
};
