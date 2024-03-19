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

	// Ŭ���̾�Ʈ�� ���ӿ� �������� �������� ��ġ���¸� ��û. ������ ��ġ���¸� �����.
	ServerCheckMatchState();
}

void ABlasterPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// �÷��̾ ���ŵ� �� ��Ʈ�ѷ��� ������� ���� ���¿��� BeginPlay()�� ���� ü���� ���ŵ��� �ʴ´�.
	// �׷��� OnPossess()���� ����
	// �ٸ� ���� ���� ���࿡���� UI���� ��Ʈ�ѷ��� ���� �����Ǽ� �۵����� �� �ֱ⿡ ĳ���Ϳ����� ȣ���Ѵ�.
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

	// ���� �ֱ⸶�� �ð� ����
	CheckTimeSync(DeltaTime);

	// ��Ʈ�ѷ��� ���� �����Ǽ� �������̰� �ʱ�ȭ�ȉ��� �� �ʱ�ȭ
	PollInit();

	// �ֱ������� ���� �� üũ
	CheckPing(DeltaTime);
}

void ABlasterPlayerController::ReceivedPlayer()
{
	Super::ReceivedPlayer();

	// �÷��̾ ������� �� �ð��� �����ϱ� ���� �������� �ð��� Ȯ��
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
	// �ش� �Լ��� Server RPC�̹Ƿ� Ŭ�󿡼� ȣ���ص� �������� ����

	// ���� ���� �ð��� �˾Ƴ��� Ŭ���̾�Ʈ���� ������.
	const float ServerTimeOfReceipt = GetWorld()->GetTimeSeconds();
	ClientReportServerTime(TimeOfClientRequest, ServerTimeOfReceipt);
}

void ABlasterPlayerController::ClientReportServerTime_Implementation(float TimeOfClientRequest, float TimeServerReceivedClientRequest)
{
	if (!GetWorld()) return;
	// �ش� �Լ��� Client RPC�̹Ƿ� �������� ȣ���ص� �ش� Ŭ�󿡼� ����

	// ���� Ŭ��ð����� Ŭ�� ��û�� �ð��� ���� ���� �ð��� Ȯ��
	const float RoundTripTime = GetWorld()->GetTimeSeconds() - TimeOfClientRequest;

	// ������ ������ �ð��� ���� �ð��� ���ļ� ���� �ùٸ� ���� �ð��� Ȯ��
	// 0.5f�� �����ð��� �պ� �����̶� ������ ������
	SingleTripTime = RoundTripTime * 0.5f;
	const float CurrentServerTime = TimeServerReceivedClientRequest + SingleTripTime;

	// ������ Ŭ���̾�Ʈ�� �ð� ���� Ȯ��
	ClientServerDelta = CurrentServerTime - GetWorld()->GetTimeSeconds();
}

void ABlasterPlayerController::ServerCheckMatchState_Implementation()
{
	// ������ ���� ��忡�� ��ġ ���� �� ���ð��� ��ġ ���ӽð��� Ȯ��
	if (BlasterGameMode)
	{
		MatchState = BlasterGameMode->GetMatchState();
		WarmupTime = BlasterGameMode->GetWarmupTime();
		MatchTime = BlasterGameMode->GetMatchTime();
		CooldownTime = BlasterGameMode->GetCooldownTime();
		LevelStartingTime = BlasterGameMode->GetLevelStartingTime();

		// �������� �˾Ƴ� ��ġ ���� ������ Ŭ���̾�Ʈ�鿡�Ե� �˷���
		ClientJoinMidGame(MatchState, WarmupTime, MatchTime, CooldownTime, LevelStartingTime);
	}
}

void ABlasterPlayerController::ClientJoinMidGame_Implementation(const FName& StateOfMatch, float Warmup, float Match, float Cooldown, float StartingTime)
{
	// �������� �˾Ƴ� ��ġ ���� ������ �߰��� ���� Ŭ���̾�Ʈ�鿡�Ե� �˷���
	WarmupTime = Warmup;
	MatchTime = Match;
	CooldownTime = Cooldown;
	LevelStartingTime = StartingTime;
	MatchState = StateOfMatch;

	// �߰��� �������� �� ��ġ���·� �ٷ� ����
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

	if (HasAuthority()) return GetWorld()->GetTimeSeconds(); // ������ �״�� ����
	// ���� ���� �ð��� Ŭ��ð��� ���̸� ���ļ� �ùٸ� ���� �ð��� �˷���
	else return GetWorld()->GetTimeSeconds() + ClientServerDelta;
}

void ABlasterPlayerController::OnMatchStateSet(const FName& State)
{
	// �� �Լ��� ���Ӹ�忡�� ȣ��ǹǷ� ������ �����̵ȴ�. Ŭ��� ���ø����̼����� �˷���
	MatchState = State;

	// ��ġ�� ���۵Ǹ�
	if (MatchState == MatchState::InProgress)
	{
		HandleMatchHasStarted();
	}
	// Ŀ���� ��ġ ����. ��ġ ������ �ð�
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

	// ���� �ð��� ������ 1�ʶ� �����Ǿ��� �� UI ������Ʈ(ƽ���� ������Ʈ ����)
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
	// ��Ʈ�ѷ��� �������̺��� ���� �������� �� ���߿� �ʱ�ȭ�ϱ�����
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
		// ��ġ ���� �� �˸� �������� ������ �����(��ٿ� �� ��Ȱ��)
		if (BlasterHUD->GetAnnouncement())
		{
			BlasterHUD->GetAnnouncement()->SetVisibility(ESlateVisibility::Hidden);
		}

		// �������� ���� ȭ�鿡 ǥ��
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
		// �������� ������ ��ġ �����ϸ鼭 ���� ����⿡ ���⼭�� �ƿ� ����
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
				// �ְ��� �÷��̾���� ǥ��
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

	// ��ٿ� ������ ���� Ư�� �ൿ�� ����
	if (ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(GetPawn()))
	{
		BlasterCharacter->SetDisableGameplay(true);

		if (BlasterCharacter->GetCombat())
		{
			// ���Ⱑ ������ �� �ڵ��߻縦 ����
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
	// ���� �ð� �ֱ�� ���� ������ üũ
	HighPingRunningTime += DeltaTime;
	if (HighPingRunningTime > CheckPingFrequency)
	{
		if (!PlayerState) PlayerState = GetPlayerState<APlayerState>();
		if (PlayerState)
		{
			// ���� ���ٸ� UI �ִϸ��̼��� ���� ���
			if (PlayerState->GetPingInMilliseconds() > HighPingThreshold)
			{
				HighPingWarning();
				PingAnimationRunningTime = 0.f;
				// Ŭ���� ���� ���ٰ� �������� �˷��ش�.
				ServerReportPingStatus(true);
			}
			else
			{
				// Ŭ���� ���� ���ٰ� �������� �˷��ش�.
				ServerReportPingStatus(false);
			}
		}
		HighPingRunningTime = 0.f;
	}

	// ���� �� ��� �ִϸ��̼��� ���� �ð��� ����
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
	// �������� �����Ѱ� Ŭ�󿡼��� �Ȱ��� ����
	if (MatchState == MatchState::InProgress)
	{
		HandleMatchHasStarted();
	}
	else if (MatchState == MatchState::Cooldown)
	{
		HandleCooldown();
	}
}
