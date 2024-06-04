// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "BlasterPlayerController.generated.h"

// 핑이 너무 높을 때 사용할 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FHighPingDelegate, bool, bPingTooHigh);

class ABlasterGameMode;
class ABlasterHUD;
class UInputMappingContext;
class UInputAction;
class UReturnToMainMenu;
class ABlasterPlayerState;
class ABlasterGameState;

/**
 * 플레이어 컨트롤러 클래스
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
	// 클라이언트에서 서버의 시간을 확인할 때 사용
	float GetServerTime() const;
	// 패킷이 한번 전달될 때 걸리는 시간
	FORCEINLINE float GetSingleTripTime() const { return SingleTripTime; }

	// 게임모드에서 매치가 변경될 때 컨트롤러에게 알려줄 때 사용
	void OnMatchStateSet(const FName& State, bool bTeamsMatch = false);

	// 게임모드에서 플레이어가 제거되었음을 컨트롤러에게 알려주는 함수
	void BroadcastElim(APlayerState* Attacker, APlayerState* Victim);

	// HUD에 값 적용 함수
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
	// 서버-클라 시간 동기화
	// 서버에게 현재 서버 시간을 요청
	UFUNCTION(Server, Reliable)
	void ServerRequestServerTime(float TimeOfClientRequest);
	// 클라에게 현재 서버 시간을 알려줌. 클라는 해당 내용으로 서버-클라 시간 차이를 확인
	UFUNCTION(Client, Reliable)
	void ClientReportServerTime(float TimeOfClientRequest, float TimeServerReceivedClientRequest);
	// 주기적으로 시간 동기화
	void CheckTimeSync(float DeltaTime);

	// 매치 관련
	// 서버의 매치 상태를 확인
	UFUNCTION(Server, Reliable)
	void ServerCheckMatchState();
	// 서버에서 확인한 매치 상태를 클라들에게 알림
	UFUNCTION(Client, Reliable)
	void ClientJoinMidGame(const FName& StateOfMatch, float Warmup, float Match, float Cooldown, float StartingTime);
	// 매치 시작
	void HandleMatchHasStarted(bool bTeamsMatch = false);
	// 매치 사이의 쉬는 시간
	void HandleCooldown();
	// 쿨다운 때 표시할 텍스트
	FString GetInfoText(const TArray<ABlasterPlayerState*>& Players) const;
	FString GetTeamsInfoText(ABlasterGameState* BlasterGameState) const;

	// 핑 상태 확인
	// 클라이언트의 핑 상태를 서버에게 알려줌
	UFUNCTION(Server, Reliable)
	void ServerReportPingStatus(bool bHighPing);
	// 주기적으로 핑 확인
	void CheckPing(float DeltaTime);
	// 높은 핑 알림
	void HighPingWarning();
	// 높은 핑 알림 끔
	void StopHighPingWarning();

	// 플레이어 제거
	UFUNCTION(Client, Reliable)
	void ClientElimAnnouncement(APlayerState* Attacker, APlayerState* Victim);

	// HUD 관련
	// HUD 초기화 관련
	void PollInit();
	void HUDInit();
	// HUD에 시간 표시
	void SetHUDTime();
	// HUD에 팀 점수 표시
	void InitTeamScore();
	void HideTeamScore();

	// 종료키 통해서 일시정지 메뉴로
	void ShowReturnToMainMenu();

public:
	// 핑이 너무 높을 때 사용할 델리게이트
	FHighPingDelegate HighPingDelegate;

private:
	// 입력 관련 변수
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultInputMappingContext;
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> QuitInputAction;

	// 서버-클라 시간 동기화
	// 시간 동기화 확인 간격
	UPROPERTY(EditAnywhere, Category = "Time")
	float TimeSyncFrequency = 5.f;
	// 클라이언트와 서버의 시간 차이
	float ClientServerDelta = 0.f;
	// 클라 <-> 서버 한번 전달하는 시간
	float SingleTripTime = 0.f;
	// 시간 동기화 체크할 때 흘러간 시간
	float TimeSyncRunningTime = 0.f;

	// 매치 관련
	// 게임 모드의 현재 매치를 알기 위한 변수. 클라는 게임 모드를 확인할 수 없어서 레플리케이션해서 사용
	UPROPERTY(ReplicatedUsing = OnRep_MatchState)
	FName MatchState;
	UFUNCTION()
	void OnRep_MatchState();
	// 각 매치 상태 사이의 시간
	float WarmupTime;
	float MatchTime;
	float CooldownTime;
	float LevelStartingTime;

	// 핑 상태 확인
	// 핑 확인 간격
	UPROPERTY(EditAnywhere, Category = "Ping")
	float CheckPingFrequency = 20.f;
	// 높은 핑 기준
	UPROPERTY(EditAnywhere, Category = "Ping")
	float HighPingThreshold = 50.f;
	// 높은 핑 애니메이션 간격
	UPROPERTY(EditAnywhere, Category = "Ping")
	float HighPingDuration = 5.f;
	// 높은 핑 체크할 때 흘러간 시간
	float HighPingRunningTime = 0.f;
	// 높은 핑 경고 애니메이션 흘러간 시간
	float PingAnimationRunningTime = 0.f;

	// HUD 관련
	// HUD가 초기화되었는지?
	bool bInitializeCharacterOverlay = false;
	// HUD 초기화에서 사용할 변수들
	float HUDHealth;
	float HUDMaxHealth;
	float HUDShield;
	float HUDMaxShield;
	float HUDScore;
	int32 HUDDefeats;
	int32 HUDGrenades;
	int32 HUDWeaponAmmo;
	int32 HUDCarriedAmmo;
	// 팀 점수 표시 여부. 클라에도 적용시켜야해서 레플리케이션
	UPROPERTY(ReplicatedUsing = OnRep_ShowTeamScores)
	bool bShowTeamScores = false;
	UFUNCTION()
	void OnRep_ShowTeamScores();
	// UI에 시간 업데이트할 때 틱마다 하지않고 1초단위로 하기위한 변수
	uint32 CounddownInt = 0;

	// 일시 정지 메뉴 UI 클래스
	UPROPERTY(EditAnywhere, Category = "HUD")
	TSubclassOf<UUserWidget> ReturnToMainMenuWidget;
	// 일시정지 메뉴를 온오프하기 위한 변수
	bool bReturnToMainMenuOpen = false;

	// 참조 변수
	UPROPERTY()
	TObjectPtr<ABlasterGameMode> BlasterGameMode;
	UPROPERTY()
	TObjectPtr<ABlasterHUD> BlasterHUD;
	UPROPERTY()
	TObjectPtr<UReturnToMainMenu> ReturnToMainMenu;
};
