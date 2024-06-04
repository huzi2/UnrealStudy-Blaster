// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "BlasterPlayerController.generated.h"

// ���� �ʹ� ���� �� ����� ��������Ʈ
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FHighPingDelegate, bool, bPingTooHigh);

class ABlasterGameMode;
class ABlasterHUD;
class UInputMappingContext;
class UInputAction;
class UReturnToMainMenu;
class ABlasterPlayerState;
class ABlasterGameState;

/**
 * �÷��̾� ��Ʈ�ѷ� Ŭ����
 */
UCLASS()
class BLASTER_API ABlasterPlayerController : public APlayerController
{
	GENERATED_BODY()

private:
	virtual void BeginPlay() final;
	virtual void OnPossess(APawn* InPawn) final;
	virtual void SetupInputComponent() final;
	virtual void Tick(float DeltaTime) final;
	virtual void ReceivedPlayer() final;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const final;

public:
	// Ŭ���̾�Ʈ���� ������ �ð��� Ȯ���� �� ���
	float GetServerTime() const;
	// ��Ŷ�� �ѹ� ���޵� �� �ɸ��� �ð�
	FORCEINLINE float GetSingleTripTime() const { return SingleTripTime; }

	// ���Ӹ�忡�� ��ġ�� ����� �� ��Ʈ�ѷ����� �˷��� �� ���
	void OnMatchStateSet(const FName& State, bool bTeamsMatch = false);

	// ���Ӹ�忡�� �÷��̾ ���ŵǾ����� ��Ʈ�ѷ����� �˷��ִ� �Լ�
	void BroadcastElim(APlayerState* Attacker, APlayerState* Victim);

	// HUD�� �� ���� �Լ�
	void SetHUDHealth(float Health, float MaxHealth);
	void SetHUDShield(float Shield, float MaxShield);
	void SetHUDScore(float Score);
	void SetHUDDefeats(int32 Defeats);
	void SetHUDWeaponAmmo(int32 Ammo);
	void SetHUDCarriedAmmo(int32 Ammo);
	void SetHUDMatchCountdown(float CounddownTime);
	void SetHUDAnnouncementCountdown(float CounddownTime);
	void SetHUDGrenades(int32 Grenades);
	void SetHUDBlueTeamScore(int32 BlueScore);
	void SetHUDRedTeamScore(int32 RedScore);

private:
	// ����-Ŭ�� �ð� ����ȭ
	// �������� ���� ���� �ð��� ��û
	UFUNCTION(Server, Reliable)
	void ServerRequestServerTime(float TimeOfClientRequest);
	// Ŭ�󿡰� ���� ���� �ð��� �˷���. Ŭ��� �ش� �������� ����-Ŭ�� �ð� ���̸� Ȯ��
	UFUNCTION(Client, Reliable)
	void ClientReportServerTime(float TimeOfClientRequest, float TimeServerReceivedClientRequest);
	// �ֱ������� �ð� ����ȭ
	void CheckTimeSync(float DeltaTime);

	// ��ġ ����
	// ������ ��ġ ���¸� Ȯ��
	UFUNCTION(Server, Reliable)
	void ServerCheckMatchState();
	// �������� Ȯ���� ��ġ ���¸� Ŭ��鿡�� �˸�
	UFUNCTION(Client, Reliable)
	void ClientJoinMidGame(const FName& StateOfMatch, float Warmup, float Match, float Cooldown, float StartingTime);
	// ��ġ ����
	void HandleMatchHasStarted(bool bTeamsMatch = false);
	// ��ġ ������ ���� �ð�
	void HandleCooldown();
	// ��ٿ� �� ǥ���� �ؽ�Ʈ
	FString GetInfoText(const TArray<ABlasterPlayerState*>& Players) const;
	FString GetTeamsInfoText(ABlasterGameState* BlasterGameState) const;

	// �� ���� Ȯ��
	// Ŭ���̾�Ʈ�� �� ���¸� �������� �˷���
	UFUNCTION(Server, Reliable)
	void ServerReportPingStatus(bool bHighPing);
	// �ֱ������� �� Ȯ��
	void CheckPing(float DeltaTime);
	// ���� �� �˸�
	void HighPingWarning();
	// ���� �� �˸� ��
	void StopHighPingWarning();

	// �÷��̾� ����
	UFUNCTION(Client, Reliable)
	void ClientElimAnnouncement(APlayerState* Attacker, APlayerState* Victim);

	// HUD ����
	// HUD �ʱ�ȭ ����
	void PollInit();
	void HUDInit();
	// HUD�� �ð� ǥ��
	void SetHUDTime();
	// HUD�� �� ���� ǥ��
	void InitTeamScore();
	void HideTeamScore();

	// ����Ű ���ؼ� �Ͻ����� �޴���
	void ShowReturnToMainMenu();

public:
	// ���� �ʹ� ���� �� ����� ��������Ʈ
	FHighPingDelegate HighPingDelegate;

private:
	// �Է� ���� ����
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultInputMappingContext;
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> QuitInputAction;

	// ����-Ŭ�� �ð� ����ȭ
	// �ð� ����ȭ Ȯ�� ����
	UPROPERTY(EditAnywhere, Category = "Time")
	float TimeSyncFrequency = 5.f;
	// Ŭ���̾�Ʈ�� ������ �ð� ����
	float ClientServerDelta = 0.f;
	// Ŭ�� <-> ���� �ѹ� �����ϴ� �ð�
	float SingleTripTime = 0.f;
	// �ð� ����ȭ üũ�� �� �귯�� �ð�
	float TimeSyncRunningTime = 0.f;

	// ��ġ ����
	// ���� ����� ���� ��ġ�� �˱� ���� ����. Ŭ��� ���� ��带 Ȯ���� �� ��� ���ø����̼��ؼ� ���
	UPROPERTY(ReplicatedUsing = OnRep_MatchState)
	FName MatchState;
	UFUNCTION()
	void OnRep_MatchState();
	// �� ��ġ ���� ������ �ð�
	float WarmupTime;
	float MatchTime;
	float CooldownTime;
	float LevelStartingTime;

	// �� ���� Ȯ��
	// �� Ȯ�� ����
	UPROPERTY(EditAnywhere, Category = "Ping")
	float CheckPingFrequency = 20.f;
	// ���� �� ����
	UPROPERTY(EditAnywhere, Category = "Ping")
	float HighPingThreshold = 50.f;
	// ���� �� �ִϸ��̼� ����
	UPROPERTY(EditAnywhere, Category = "Ping")
	float HighPingDuration = 5.f;
	// ���� �� üũ�� �� �귯�� �ð�
	float HighPingRunningTime = 0.f;
	// ���� �� ��� �ִϸ��̼� �귯�� �ð�
	float PingAnimationRunningTime = 0.f;

	// HUD ����
	// HUD�� �ʱ�ȭ�Ǿ�����?
	bool bInitializeCharacterOverlay = false;
	// HUD �ʱ�ȭ���� ����� ������
	float HUDHealth;
	float HUDMaxHealth;
	float HUDShield;
	float HUDMaxShield;
	float HUDScore;
	int32 HUDDefeats;
	int32 HUDGrenades;
	int32 HUDWeaponAmmo;
	int32 HUDCarriedAmmo;
	// �� ���� ǥ�� ����. Ŭ�󿡵� ������Ѿ��ؼ� ���ø����̼�
	UPROPERTY(ReplicatedUsing = OnRep_ShowTeamScores)
	bool bShowTeamScores = false;
	UFUNCTION()
	void OnRep_ShowTeamScores();
	// UI�� �ð� ������Ʈ�� �� ƽ���� �����ʰ� 1�ʴ����� �ϱ����� ����
	uint32 CounddownInt = 0;

	// �Ͻ� ���� �޴� UI Ŭ����
	UPROPERTY(EditAnywhere, Category = "HUD")
	TSubclassOf<UUserWidget> ReturnToMainMenuWidget;
	// �Ͻ����� �޴��� �¿����ϱ� ���� ����
	bool bReturnToMainMenuOpen = false;

	// ���� ����
	UPROPERTY()
	TObjectPtr<ABlasterGameMode> BlasterGameMode;
	UPROPERTY()
	TObjectPtr<ABlasterHUD> BlasterHUD;
	UPROPERTY()
	TObjectPtr<UReturnToMainMenu> ReturnToMainMenu;
};
