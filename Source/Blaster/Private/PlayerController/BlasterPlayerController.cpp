// Fill out your copyright notice in the Description page of Project Settings.

#include "PlayerController/BlasterPlayerController.h"
#include "HUD/BlasterHUD.h"
#include "HUD/CharacterOverlay.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Character/BlasterCharacter.h"
#include "Net/UnrealNetwork.h"
#include "GameMode/BlasterGameMode.h"
#include "HUD/Announcement.h"
#include "Kismet/GameplayStatics.h"
#include "BlasterComponents/CombatComponent.h"
#include "GameState/BlasterGameState.h"
#include "PlayerState/BlasterPlayerState.h"
#include "Components/Image.h"

ABlasterPlayerController::ABlasterPlayerController()
	: TimeSyncFrequency(5.f)
	, HighPingDuration(5.f)
	, CheckPingFrequency(20.f)
	, HighPingThreshold(50.f)
	, CounddownInt(0)
	, ClientServerDelta(0.f)
	, SingleTripTime(0.f)
	, TimeSyncRunningTime(0.f)
	, bInitializeCharacterOverlay(false)
	, HighPingRunningTime(0.f)
	, PingAnimationRunningTime(0.f)
{
}

void ABlasterPlayerController::BeginPlay()
{
	Super::BeginPlay();

	HUDInit();

	if (HasAuthority())
	{
		BlasterGameMode = Cast<ABlasterGameMode>(UGameplayStatics::GetGameMode(this));
	}

	// 클라이언트가 게임에 들어왔으면 서버에게 매치상태를 요청. 서버와 매치상태를 맞춘다.
	ServerCheckMatchState();
}

void ABlasterPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// 플레이어가 제거될 때 컨트롤러가 연결되지 않은 상태에서 BeginPlay()에 들어가면 체력이 갱신되지 않는다.
	// 그래서 OnPossess()에서 갱신
	// 다만 최초 게임 실행에서는 UI보다 컨트롤러가 먼저 생성되서 작동안할 수 있기에 캐릭터에서도 호출한다.
	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(InPawn);
	if (BlasterCharacter)
	{
		SetHUDHealth(BlasterCharacter->GetHealth(), BlasterCharacter->GetMaxHealth());
		SetHUDShield(BlasterCharacter->GetShield(), BlasterCharacter->GetMaxShield());
	}
}

void ABlasterPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	SetHUDTime();

	// 일정 주기마다 시간 보정
	CheckTimeSync(DeltaTime);

	// 컨트롤러가 먼저 생성되서 오버레이가 초기화안됬을 때 초기화
	PollInit();

	// 주기적으로 높은 핑 체크
	CheckPing(DeltaTime);
}

void ABlasterPlayerController::ReceivedPlayer()
{
	Super::ReceivedPlayer();

	// 플레이어가 연결됬을 때 시간을 보정하기 위해 서버에게 시간을 확인
	if (IsLocalController() && GetWorld())
	{
		ServerRequestServerTime(GetWorld()->GetTimeSeconds());
	}
}

void ABlasterPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABlasterPlayerController, MatchState);
}

void ABlasterPlayerController::ServerRequestServerTime_Implementation(float TimeOfClientRequest)
{
	if (!GetWorld()) return;
	// 해당 함수는 Server RPC이므로 클라에서 호출해도 서버에서 사용됨

	// 현재 서버 시간을 알아내서 클라이언트에게 보낸다.
	const float ServerTimeOfReceipt = GetWorld()->GetTimeSeconds();
	ClientReportServerTime(TimeOfClientRequest, ServerTimeOfReceipt);
}

void ABlasterPlayerController::ClientReportServerTime_Implementation(float TimeOfClientRequest, float TimeServerReceivedClientRequest)
{
	if (!GetWorld()) return;
	// 해당 함수는 Client RPC이므로 서버에서 호출해도 해당 클라에서 사용됨

	// 현재 클라시간에서 클라가 요청한 시간을 빼서 지연 시간을 확인
	const float RoundTripTime = GetWorld()->GetTimeSeconds() - TimeOfClientRequest;

	// 서버가 보내준 시간에 지연 시간을 합쳐서 현재 올바른 서버 시간을 확인
	// 0.5f는 지연시간이 왕복 기준이라 반으로 나눈것
	SingleTripTime = RoundTripTime * 0.5f;
	const float CurrentServerTime = TimeServerReceivedClientRequest + SingleTripTime;

	// 서버와 클라이언트의 시간 차이 확인
	ClientServerDelta = CurrentServerTime - GetWorld()->GetTimeSeconds();
}

void ABlasterPlayerController::ServerCheckMatchState_Implementation()
{
	// 서버의 게임 모드에서 매치 시작 전 대기시간과 매치 지속시간을 확인
	if (BlasterGameMode)
	{
		MatchState = BlasterGameMode->GetMatchState();
		WarmupTime = BlasterGameMode->GetWarmupTime();
		MatchTime = BlasterGameMode->GetMatchTime();
		CooldownTime = BlasterGameMode->GetCooldownTime();
		LevelStartingTime = BlasterGameMode->GetLevelStartingTime();

		// 서버에서 알아낸 매치 관련 정보를 클라이언트들에게도 알려줌
		ClientJoinMidGame(MatchState, WarmupTime, MatchTime, CooldownTime, LevelStartingTime);
	}
}

void ABlasterPlayerController::ClientJoinMidGame_Implementation(const FName& StateOfMatch, float Warmup, float Match, float Cooldown, float StartingTime)
{
	// 서버에서 알아낸 매치 관련 정보를 중간에 들어온 클라이언트들에게도 알려줌
	WarmupTime = Warmup;
	MatchTime = Match;
	CooldownTime = Cooldown;
	LevelStartingTime = StartingTime;
	MatchState = StateOfMatch;

	// 중간에 들어왔으면 새 매치상태로 바로 변경
	OnMatchStateSet(MatchState);

	if (BlasterHUD && MatchState == MatchState::WaitingToStart)
	{
		BlasterHUD->AddAnnouncement();
	}
}

void ABlasterPlayerController::ServerReportPingStatus_Implementation(bool bHighPing)
{
	HighPingDelegate.Broadcast(bHighPing);
}

void ABlasterPlayerController::SetHUDHealth(float Health, float MaxHealth)
{
	HUDInit();

	if (!BlasterHUD || !BlasterHUD->GetCharacterOverlay())
	{
		if (!GetHUD())
		{
			UE_LOG(LogTemp, Warning, TEXT("HUD not"));
		}

		UE_LOG(LogTemp, Warning, TEXT("BlasterHUD not"));
		bInitializeCharacterOverlay = true;
		HUDHealth = Health;
		HUDMaxHealth = MaxHealth;
		return;
	}
	if (!BlasterHUD->GetCharacterOverlay()->GetHealthBar()) return;
	if (!BlasterHUD->GetCharacterOverlay()->GetHealthText()) return;

	const float HealthPercent = MaxHealth == 0.f ? 0.f : Health / MaxHealth;
	BlasterHUD->GetCharacterOverlay()->GetHealthBar()->SetPercent(HealthPercent);

	const FString HealthText = FString::Printf(TEXT("%d/%d"), FMath::CeilToInt(Health), FMath::CeilToInt(MaxHealth));
	BlasterHUD->GetCharacterOverlay()->GetHealthText()->SetText(FText::FromString(HealthText));
}

void ABlasterPlayerController::SetHUDShield(float Shield, float MaxShield)
{
	HUDInit();

	if (!BlasterHUD || !BlasterHUD->GetCharacterOverlay())
	{
		bInitializeCharacterOverlay = true;
		HUDShield = Shield;
		HUDMaxShield = MaxShield;
		return;
	}
	if (!BlasterHUD->GetCharacterOverlay()->GetShieldBar()) return;
	if (!BlasterHUD->GetCharacterOverlay()->GetShieldText()) return;

	const float ShieldPercent = MaxShield == 0.f ? 0.f : Shield / MaxShield;
	BlasterHUD->GetCharacterOverlay()->GetShieldBar()->SetPercent(ShieldPercent);

	const FString ShieldText = FString::Printf(TEXT("%d/%d"), FMath::CeilToInt(Shield), FMath::CeilToInt(MaxShield));
	BlasterHUD->GetCharacterOverlay()->GetShieldText()->SetText(FText::FromString(ShieldText));
}

void ABlasterPlayerController::SetHUDScore(float Score)
{
	HUDInit();

	if (!BlasterHUD || !BlasterHUD->GetCharacterOverlay())
	{
		bInitializeCharacterOverlay = true;
		HUDScore = Score;
		return;
	}
	if (!BlasterHUD->GetCharacterOverlay()->GetScoreAmount()) return;

	const FString ScoreText = FString::Printf(TEXT("%d"), FMath::FloorToInt(Score));
	BlasterHUD->GetCharacterOverlay()->GetScoreAmount()->SetText(FText::FromString(ScoreText));
}

void ABlasterPlayerController::SetHUDDefeats(int32 Defeats)
{
	HUDInit();

	if (!BlasterHUD || !BlasterHUD->GetCharacterOverlay())
	{
		bInitializeCharacterOverlay = true;
		HUDDefeats = Defeats;
		return;
	}
	if (!BlasterHUD->GetCharacterOverlay()->GetDefeatsAmount()) return;

	const FString DefeatsText = FString::Printf(TEXT("%d"), Defeats);
	BlasterHUD->GetCharacterOverlay()->GetDefeatsAmount()->SetText(FText::FromString(DefeatsText));
}

void ABlasterPlayerController::SetHUDWeaponAmmo(int32 Ammo)
{
	HUDInit();

	if (!BlasterHUD || !BlasterHUD->GetCharacterOverlay())
	{
		bInitializeCharacterOverlay = true;
		HUDWeaponAmmo = Ammo;
		return;
	}
	if (!BlasterHUD->GetCharacterOverlay()->GetWeaponAmmoAmount()) return;

	const FString AmmoText = FString::Printf(TEXT("%d"), Ammo);
	BlasterHUD->GetCharacterOverlay()->GetWeaponAmmoAmount()->SetText(FText::FromString(AmmoText));
}

void ABlasterPlayerController::SetHUDCarriedAmmo(int32 Ammo)
{
	HUDInit();

	if (!BlasterHUD || !BlasterHUD->GetCharacterOverlay())
	{
		bInitializeCharacterOverlay = true;
		HUDCarriedAmmo = Ammo;
		return;
	}
	if (!BlasterHUD->GetCharacterOverlay()->GetCarriedAmmoAmount()) return;

	const FString AmmoText = FString::Printf(TEXT("%d"), Ammo);
	BlasterHUD->GetCharacterOverlay()->GetCarriedAmmoAmount()->SetText(FText::FromString(AmmoText));
}

void ABlasterPlayerController::SetHUDMatchCountdown(float CounddownTime)
{
	HUDInit();

	if (!BlasterHUD) return;
	if (!BlasterHUD->GetCharacterOverlay()) return;
	if (!BlasterHUD->GetCharacterOverlay()->GetMatchCountdownText()) return;

	if (CounddownTime < 0.f)
	{
		BlasterHUD->GetCharacterOverlay()->GetMatchCountdownText()->SetText(FText());
		return;
	}

	const int32 Minutes = FMath::FloorToInt(CounddownTime / 60.f);
	const int32 Seconds = CounddownTime - (Minutes * 60);

	const FString CountdownText = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
	BlasterHUD->GetCharacterOverlay()->GetMatchCountdownText()->SetText(FText::FromString(CountdownText));
}

void ABlasterPlayerController::SetHUDAnnouncementCountdown(float CounddownTime)
{
	HUDInit();

	if (!BlasterHUD) return;
	if (!BlasterHUD->GetAnnouncement()) return;
	if (!BlasterHUD->GetAnnouncement()->GetWarmupTime()) return;

	if (CounddownTime < 0.f)
	{
		BlasterHUD->GetAnnouncement()->GetWarmupTime()->SetText(FText());
		return;
	}

	const int32 Minutes = FMath::FloorToInt(CounddownTime / 60.f);
	const int32 Seconds = CounddownTime - (Minutes * 60);

	const FString CountdownText = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
	BlasterHUD->GetAnnouncement()->GetWarmupTime()->SetText(FText::FromString(CountdownText));
}

void ABlasterPlayerController::SetHUDGrenades(int32 Grenades)
{
	HUDInit();

	if (!BlasterHUD || !BlasterHUD->GetCharacterOverlay())
	{
		bInitializeCharacterOverlay = true;
		HUDGrenades = Grenades;
		return;
	}
	if (!BlasterHUD->GetCharacterOverlay()->GetGrenadeText()) return;

	const FString GrenadesText = FString::Printf(TEXT("%d"), Grenades);
	BlasterHUD->GetCharacterOverlay()->GetGrenadeText()->SetText(FText::FromString(GrenadesText));
}

float ABlasterPlayerController::GetServerTime() const
{
	if (!GetWorld()) return 0.f;

	if (HasAuthority()) return GetWorld()->GetTimeSeconds(); // 서버는 그대로 리턴
	// 현재 서버 시간과 클라시간의 차이를 합쳐서 올바른 서버 시간을 알려줌
	else return GetWorld()->GetTimeSeconds() + ClientServerDelta;
}

void ABlasterPlayerController::OnMatchStateSet(const FName& State)
{
	// 이 함수는 게임모드에서 호출되므로 서버만 세팅이된다. 클라는 레플리케이션으로 알려줌
	MatchState = State;

	// 매치가 시작되면
	if (MatchState == MatchState::InProgress)
	{
		HandleMatchHasStarted();
	}
	// 커스텀 매치 상태. 매치 사이의 시간
	else if (MatchState == MatchState::Cooldown)
	{
		HandleCooldown();
	}
}

void ABlasterPlayerController::HUDInit()
{
	if (!BlasterHUD)
	{
		BlasterHUD = Cast<ABlasterHUD>(GetHUD());
	}
}

void ABlasterPlayerController::SetHUDTime()
{
	if (!GetWorld()) return;

	float TimeLeft = 0.f;
	if (MatchState == MatchState::WaitingToStart)
	{
		TimeLeft = WarmupTime - GetServerTime() + LevelStartingTime;
	}
	else if (MatchState == MatchState::InProgress)
	{
		TimeLeft = WarmupTime + MatchTime - GetServerTime() + LevelStartingTime;
	}
	else if (MatchState == MatchState::Cooldown)
	{
		TimeLeft = WarmupTime + MatchTime + CooldownTime - GetServerTime() + LevelStartingTime;
	}

	uint32 SecondsLeft = 0;
	if (BlasterGameMode)
	{
		SecondsLeft = BlasterGameMode->GetCountdownTime() + LevelStartingTime;
	}
	else
	{
		SecondsLeft = FMath::CeilToInt(TimeLeft);
	}

	// 남은 시간이 실제로 1초라도 수정되었을 때 UI 업데이트(틱마다 업데이트 방지)
	if (CounddownInt != SecondsLeft)
	{
		if (MatchState == MatchState::WaitingToStart || MatchState == MatchState::Cooldown)
		{
			SetHUDAnnouncementCountdown(TimeLeft);
		}
		else if (MatchState == MatchState::InProgress)
		{
			SetHUDMatchCountdown(TimeLeft);
		}
	}

	CounddownInt = SecondsLeft;
}

void ABlasterPlayerController::CheckTimeSync(float DeltaTime)
{
	TimeSyncRunningTime += DeltaTime;
	if (IsLocalController() && GetWorld() && TimeSyncRunningTime > TimeSyncFrequency)
	{
		ServerRequestServerTime(GetWorld()->GetTimeSeconds());
		TimeSyncRunningTime = 0.f;
	}
}

void ABlasterPlayerController::PollInit()
{
	// 컨트롤러가 오버레이보다 먼저 생성됬을 때 나중에 초기화하기위함
	if (BlasterHUD)
	{
		if (bInitializeCharacterOverlay)
		{
			SetHUDHealth(HUDHealth, HUDMaxHealth);
			SetHUDShield(HUDShield, HUDMaxShield);
			SetHUDScore(HUDScore);
			SetHUDDefeats(HUDDefeats);
			SetHUDWeaponAmmo(HUDWeaponAmmo);
			SetHUDCarriedAmmo(HUDCarriedAmmo);

			if (ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(GetPawn()))
			{
				if (BlasterCharacter->GetCombat())
				{
					SetHUDGrenades(BlasterCharacter->GetCombat()->GetGrenades());
				}
			}
			
			bInitializeCharacterOverlay = false;
		}
	}
}

void ABlasterPlayerController::HandleMatchHasStarted()
{
	HUDInit();

	if (BlasterHUD)
	{
		// 매치 시작 전 알림 오버레이 위젯은 숨기고(쿨다운 때 재활용)
		if (BlasterHUD->GetAnnouncement())
		{
			BlasterHUD->GetAnnouncement()->SetVisibility(ESlateVisibility::Hidden);
		}

		// 오버레이 위젯 화면에 표시
		if (!BlasterHUD->GetCharacterOverlay())
		{
			BlasterHUD->AddCharacterOverlay();
		}
	}
}

void ABlasterPlayerController::HandleCooldown()
{
	HUDInit();

	if (BlasterHUD)
	{
		// 오버레이 위젯은 매치 시작하면서 새로 만들기에 여기서는 아예 제거
		if (BlasterHUD->GetCharacterOverlay())
		{
			BlasterHUD->GetCharacterOverlay()->RemoveFromParent();
		}

		if (BlasterHUD->GetAnnouncement())
		{
			BlasterHUD->GetAnnouncement()->SetVisibility(ESlateVisibility::Visible);
			
			if (BlasterHUD->GetAnnouncement()->GetAnnouncementText())
			{
				const FString AnnouncementText(TEXT("New Match Starts In:"));
				BlasterHUD->GetAnnouncement()->GetAnnouncementText()->SetText(FText::FromString(AnnouncementText));
			}
			if (BlasterHUD->GetAnnouncement()->GetInfoText())
			{
				// 최고점 플레이어들을 표시
				ABlasterGameState* BlasterGameState = Cast<ABlasterGameState>(UGameplayStatics::GetGameState(this));
				ABlasterPlayerState* BlasterPlayerState = GetPlayerState<ABlasterPlayerState>();

				if (BlasterGameState)
				{
					FString InfoTextString = FString("");

					const TArray<ABlasterPlayerState*>& TopPlayers = BlasterGameState->GetTopScoringPlayers();
					if (TopPlayers.Num() == 0)
					{
						InfoTextString = TEXT("There is no winner");
					}
					else if(TopPlayers.Num() == 1 && TopPlayers[0] == BlasterPlayerState)
					{
						InfoTextString = TEXT("You are the winner!");
					}
					else if (TopPlayers.Num() == 1)
					{
						InfoTextString = FString::Printf(TEXT("Winner: \n%s"), *TopPlayers[0]->GetPlayerName());
					}
					else if(TopPlayers.Num() > 1)
					{
						InfoTextString = TEXT("Players tied for the win\n");
						for (const ABlasterPlayerState* TiedPlayer : TopPlayers)
						{
							InfoTextString.Append(FString::Printf(TEXT("%s"), *TiedPlayer->GetPlayerName()));
						}
					}

					BlasterHUD->GetAnnouncement()->GetInfoText()->SetText(FText::FromString(InfoTextString));
				}
				else
				{
					BlasterHUD->GetAnnouncement()->GetInfoText()->SetText(FText());
				}
			}
		}
	}

	// 쿨다운 상태일 때는 특정 행동을 막음
	if (ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(GetPawn()))
	{
		BlasterCharacter->SetDisableGameplay(true);

		if (BlasterCharacter->GetCombat())
		{
			// 무기가 오토일 때 자동발사를 막음
			BlasterCharacter->GetCombat()->FireButtonPressed(false);
		}
	}
}

void ABlasterPlayerController::HighPingWarning()
{
	if (!BlasterHUD) return;
	if (!BlasterHUD->GetCharacterOverlay()) return;
	if (!BlasterHUD->GetCharacterOverlay()->GetHighPingImage()) return;
	if (!BlasterHUD->GetCharacterOverlay()->GetHighPingAnimation()) return;

	BlasterHUD->GetCharacterOverlay()->GetHighPingImage()->SetOpacity(1.f);
	BlasterHUD->GetCharacterOverlay()->PlayAnimation(BlasterHUD->GetCharacterOverlay()->GetHighPingAnimation(), 0.f, 5);
}

void ABlasterPlayerController::StopHighPingWarning()
{
	if (!BlasterHUD) return;
	if (!BlasterHUD->GetCharacterOverlay()) return;
	if (!BlasterHUD->GetCharacterOverlay()->GetHighPingImage()) return;
	if (!BlasterHUD->GetCharacterOverlay()->GetHighPingAnimation()) return;

	BlasterHUD->GetCharacterOverlay()->GetHighPingImage()->SetOpacity(0.f);
	if (BlasterHUD->GetCharacterOverlay()->IsPlayingAnimation())
	{
		BlasterHUD->GetCharacterOverlay()->StopAnimation(BlasterHUD->GetCharacterOverlay()->GetHighPingAnimation());
	}
}

void ABlasterPlayerController::CheckPing(float DeltaTime)
{
	// 일정 시간 주기로 핑이 높은지 체크
	HighPingRunningTime += DeltaTime;
	if (HighPingRunningTime > CheckPingFrequency)
	{
		if (!PlayerState) PlayerState = GetPlayerState<APlayerState>();
		if (PlayerState)
		{
			// 핑이 높다면 UI 애니메이션을 통해 경고
			if (PlayerState->GetPingInMilliseconds() > HighPingThreshold)
			{
				HighPingWarning();
				PingAnimationRunningTime = 0.f;
				// 클라의 핑이 높다고 서버에게 알려준다.
				ServerReportPingStatus(true);
			}
			else
			{
				// 클라의 핑이 낮다고 서버에게 알려준다.
				ServerReportPingStatus(false);
			}
		}
		HighPingRunningTime = 0.f;
	}

	// 높은 핑 경고 애니메이션은 일정 시간만 수행
	if (BlasterHUD && BlasterHUD->GetCharacterOverlay() && BlasterHUD->GetCharacterOverlay()->GetHighPingAnimation() && BlasterHUD->GetCharacterOverlay()->IsAnimationPlaying(BlasterHUD->GetCharacterOverlay()->GetHighPingAnimation()))
	{
		PingAnimationRunningTime += DeltaTime;
		if (PingAnimationRunningTime > HighPingDuration)
		{
			StopHighPingWarning();
		}
	}
}

void ABlasterPlayerController::OnRep_MatchState()
{
	// 서버에서 세팅한걸 클라에서도 똑같이 세팅
	if (MatchState == MatchState::InProgress)
	{
		HandleMatchHasStarted();
	}
	else if (MatchState == MatchState::Cooldown)
	{
		HandleCooldown();
	}
}
