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
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "InputActionValue.h"
#include "HUD/ReturnToMainMenu.h"
#include "BlasterTypes/Announcement.h"

ABlasterPlayerController::ABlasterPlayerController()
	: TimeSyncFrequency(5.f)
	, HighPingDuration(5.f)
	, CheckPingFrequency(20.f)
	, HighPingThreshold(50.f)
	, bShowTeamScores(false)
	, CounddownInt(0)
	, ClientServerDelta(0.f)
	, SingleTripTime(0.f)
	, TimeSyncRunningTime(0.f)
	, bInitializeCharacterOverlay(false)
	, HighPingRunningTime(0.f)
	, PingAnimationRunningTime(0.f)
	, bReturnToMainMenuOpen(false)
{
}

void ABlasterPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		Subsystem->AddMappingContext(DefaultInputMappingContext, 0);
	}

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

void ABlasterPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(InputComponent))
	{
		Input->BindAction(QuitInputAction, ETriggerEvent::Started, this, &ThisClass::ShowReturnToMainMenu);
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
	DOREPLIFETIME(ABlasterPlayerController, bShowTeamScores);
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

void ABlasterPlayerController::ClientElimAnnouncement_Implementation(APlayerState* Attacker, APlayerState* Victim)
{
	APlayerState* Self = GetPlayerState<APlayerState>();
	if (Attacker && Victim && Self)
	{
		HUDInit();

		if (BlasterHUD)
		{
			// �ڽ��� �������� ����
			if (Attacker == Self && Victim != Self)
			{
				BlasterHUD->AddElimAnnouncement(TEXT("You"), Victim->GetPlayerName());
			}
			// �������� �ڽ��� ����
			else if (Victim == Self && Attacker != Self)
			{
				BlasterHUD->AddElimAnnouncement(Attacker->GetPlayerName(), TEXT("you"));
			}
			// ������ �ڻ�
			else if(Attacker == Self && Victim == Self)
			{
				BlasterHUD->AddElimAnnouncement(TEXT("You"), TEXT("yourself"));
			}
			// �������� �ڻ�
			else if(Attacker == Victim && Attacker != Self)
			{
				BlasterHUD->AddElimAnnouncement(Attacker->GetPlayerName(), TEXT("themselves"));
			}
			else
			{
				BlasterHUD->AddElimAnnouncement(Attacker->GetPlayerName(), Victim->GetPlayerName());
			}
		}
	}
}

void ABlasterPlayerController::SetHUDHealth(float Health, float MaxHealth)
{
	HUDInit();

	if (!BlasterHUD || !BlasterHUD->GetCharacterOverlay())
	{
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

void ABlasterPlayerController::SetHUDBlueTeamScore(int32 BlueScore)
{
	HUDInit();

	if (!BlasterHUD) return;
	if (!BlasterHUD->GetCharacterOverlay()) return;
	if (!BlasterHUD->GetCharacterOverlay()->GetBlueTeamScore()) return;

	const FString BlueScoreText = FString::Printf(TEXT("%d"), BlueScore);
	BlasterHUD->GetCharacterOverlay()->GetBlueTeamScore()->SetText(FText::FromString(BlueScoreText));
}

void ABlasterPlayerController::SetHUDRedTeamScore(int32 RedScore)
{
	HUDInit();

	if (!BlasterHUD) return;
	if (!BlasterHUD->GetCharacterOverlay()) return;
	if (!BlasterHUD->GetCharacterOverlay()->GetRedTeamScore()) return;

	const FString RedScoreText = FString::Printf(TEXT("%d"), RedScore);
	BlasterHUD->GetCharacterOverlay()->GetRedTeamScore()->SetText(FText::FromString(RedScoreText));
}

float ABlasterPlayerController::GetServerTime() const
{
	if (!GetWorld()) return 0.f;

	if (HasAuthority()) return GetWorld()->GetTimeSeconds(); // ������ �״�� ����
	// ���� ���� �ð��� Ŭ��ð��� ���̸� ���ļ� �ùٸ� ���� �ð��� �˷���
	else return GetWorld()->GetTimeSeconds() + ClientServerDelta;
}

void ABlasterPlayerController::OnMatchStateSet(const FName& State, bool bTeamsMatch)
{
	// �� �Լ��� ���Ӹ�忡�� ȣ��ǹǷ� ������ �����̵ȴ�. Ŭ��� ���ø����̼����� �˷���
	MatchState = State;

	// ��ġ�� ���۵Ǹ�
	if (MatchState == MatchState::InProgress)
	{
		HandleMatchHasStarted(bTeamsMatch);
	}
	// Ŀ���� ��ġ ����. ��ġ ������ �ð�
	else if (MatchState == MatchState::Cooldown)
	{
		HandleCooldown();
	}
}

void ABlasterPlayerController::BroadcastElim(APlayerState* Attacker, APlayerState* Victim)
{
	// �� �Լ��� ���Ӹ�带 ���� ������ ȣ��Ǿ���
	// Ŭ���̾�Ʈ���Ե� �˷��ֱ� ���� Ŭ���̾�Ʈ RPC �Լ��� ȣ��
	ClientElimAnnouncement(Attacker, Victim);
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

void ABlasterPlayerController::HandleMatchHasStarted(bool bTeamsMatch)
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

		// �� ����� ��� �������� ǥ��. bTeamsMatch�� true�� �ϴ°� ������ �ϰ��ִ�.
		if (HasAuthority())
		{
			if (bTeamsMatch)
			{
				InitTeamScore();
			}
			else
			{
				HideTeamScore();
			}

			// Ŭ���̾�Ʈ������ �������� ǥ���ϱ�����. bShowTeamScores�� ���ø�����Ʈ ����
			bShowTeamScores = bTeamsMatch;
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
				const FString AnnouncementText = Announcement::NewMatchStartsIn;
				BlasterHUD->GetAnnouncement()->GetAnnouncementText()->SetText(FText::FromString(AnnouncementText));
			}
			if (BlasterHUD->GetAnnouncement()->GetInfoText())
			{
				// �ְ��� �÷��̾���� ǥ��
				ABlasterGameState* BlasterGameState = Cast<ABlasterGameState>(UGameplayStatics::GetGameState(this));
				ABlasterPlayerState* BlasterPlayerState = GetPlayerState<ABlasterPlayerState>();

				if (BlasterGameState)
				{
					const TArray<ABlasterPlayerState*>& TopPlayers = BlasterGameState->GetTopScoringPlayers();
					const FString InfoTextString = bShowTeamScores ? GetTeamsInfoText(BlasterGameState) : GetInfoText(TopPlayers);

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

void ABlasterPlayerController::ShowReturnToMainMenu()
{
	if (!ReturnToMainMenuWidget) return;

	if (!ReturnToMainMenu)
	{
		ReturnToMainMenu = CreateWidget<UReturnToMainMenu>(this, ReturnToMainMenuWidget);
	}

	if (ReturnToMainMenu)
	{
		bReturnToMainMenuOpen = !bReturnToMainMenuOpen;

		bReturnToMainMenuOpen ? ReturnToMainMenu->MenuSetup() : ReturnToMainMenu->MenuTearDown();
	}
}

void ABlasterPlayerController::HideTeamScore()
{
	HUDInit();

	if (!BlasterHUD) return;
	if (!BlasterHUD->GetCharacterOverlay()) return;
	if (!BlasterHUD->GetCharacterOverlay()->GetBlueTeamScore()) return;
	if (!BlasterHUD->GetCharacterOverlay()->GetRedTeamScore()) return;
	if (!BlasterHUD->GetCharacterOverlay()->GetScoreSpacerText()) return;

	BlasterHUD->GetCharacterOverlay()->GetBlueTeamScore()->SetText(FText());
	BlasterHUD->GetCharacterOverlay()->GetRedTeamScore()->SetText(FText());
	BlasterHUD->GetCharacterOverlay()->GetScoreSpacerText()->SetText(FText());
}

void ABlasterPlayerController::InitTeamScore()
{
	HUDInit();

	if (!BlasterHUD) return;
	if (!BlasterHUD->GetCharacterOverlay()) return;
	if (!BlasterHUD->GetCharacterOverlay()->GetBlueTeamScore()) return;
	if (!BlasterHUD->GetCharacterOverlay()->GetRedTeamScore()) return;
	if (!BlasterHUD->GetCharacterOverlay()->GetScoreSpacerText()) return;

	BlasterHUD->GetCharacterOverlay()->GetBlueTeamScore()->SetText(FText::FromString(TEXT("0")));
	BlasterHUD->GetCharacterOverlay()->GetRedTeamScore()->SetText(FText::FromString(TEXT("0")));
	BlasterHUD->GetCharacterOverlay()->GetScoreSpacerText()->SetText(FText::FromString(TEXT(":")));
}

FString ABlasterPlayerController::GetInfoText(const TArray<ABlasterPlayerState*>& Players) const
{
	ABlasterPlayerState* BlasterPlayerState = GetPlayerState<ABlasterPlayerState>();
	if (!BlasterPlayerState) return FString();

	FString InfoTextString = FString();

	// ���ڰ� ����
	if (Players.Num() == 0)
	{
		InfoTextString = Announcement::ThereIsNoWinner;
	}
	// ���ڰ� ���� �÷��̾�
	else if (Players.Num() == 1 && Players[0] == BlasterPlayerState)
	{
		InfoTextString = Announcement::YouAreTheWinner;
	}
	// ���ڰ� �ٸ� ������ �Ѹ�
	else if (Players.Num() == 1)
	{
		InfoTextString = FString::Printf(TEXT("Winner: \n%s"), *Players[0]->GetPlayerName());
	}
	// ���ڰ� ������. ��� ���� �̸� ǥ��
	else if (Players.Num() > 1)
	{
		InfoTextString = Announcement::PlayerTiedForTheWin;
		InfoTextString.Append(FString("\n"));
		for (const ABlasterPlayerState* TiedPlayer : Players)
		{
			InfoTextString.Append(FString::Printf(TEXT("%s"), *TiedPlayer->GetPlayerName()));
		}
	}
	return InfoTextString;
}

FString ABlasterPlayerController::GetTeamsInfoText(ABlasterGameState* BlasterGameState) const
{
	if (!BlasterGameState) return FString();

	FString InfoTextString = FString();

	const int32 BlueTeamScore = BlasterGameState->GetBlueTeamScore();
	const int32 RedTeamScore = BlasterGameState->GetRedTeamScore();

	// ���ڰ� ����
	if (BlueTeamScore == 0 && RedTeamScore == 0)
	{
		InfoTextString = Announcement::ThereIsNoWinner;
	}
	// �� �� ������ ����
	else if (BlueTeamScore == RedTeamScore)
	{
		InfoTextString = Announcement::TeamsTiedForTheWin;
		InfoTextString.Append(FString("\n"));
		InfoTextString.Append(Announcement::BlueTeam);
		InfoTextString.Append(FString("\n"));
		InfoTextString.Append(Announcement::RedTeam);
	}
	// ����� �¸�. ���� ǥ��
	else if (BlueTeamScore > RedTeamScore)
	{
		InfoTextString = Announcement::BlueTeamWins;
		InfoTextString.Append(FString("\n"));
		InfoTextString.Append(FString::Printf(TEXT("%s: %d\n"), *Announcement::BlueTeam, BlueTeamScore));
		InfoTextString.Append(FString::Printf(TEXT("%s: %d\n"), *Announcement::RedTeam, RedTeamScore));
	}
	// ������ �¸�. ���� ǥ��
	else if(RedTeamScore > BlueTeamScore)
	{
		InfoTextString = Announcement::RedTeamWins;
		InfoTextString.Append(FString("\n"));
		InfoTextString.Append(FString::Printf(TEXT("%s: %d\n"), *Announcement::RedTeam, RedTeamScore));
		InfoTextString.Append(FString::Printf(TEXT("%s: %d\n"), *Announcement::BlueTeam, BlueTeamScore));
	}
	return InfoTextString;
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

void ABlasterPlayerController::OnRep_ShowTeamScores()
{
	if (bShowTeamScores)
	{
		InitTeamScore();
	}
	else
	{
		HideTeamScore();
	}
}
