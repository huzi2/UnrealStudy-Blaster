// Fill out your copyright notice in the Description page of Project Settings.

#include "PlayerController/BlasterPlayerController.h"
#include "HUD/BlasterHUD.h"
#include "HUD/CharacterOverlay.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Character/BlasterCharacter.h"

ABlasterPlayerController::ABlasterPlayerController()
	: TimeSyncFrequency(5.f)
	, MatchTime(120.f)
	, CounddownInt(0)
	, ClientServerDelta(0.f)
	, TimeSyncRunningTime(0.f)
{
}

void ABlasterPlayerController::BeginPlay()
{
	Super::BeginPlay();

	BlasterHUD = Cast<ABlasterHUD>(GetHUD());
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
	}
}

void ABlasterPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	SetHUDTime();

	// 일정 주기마다 시간 보정
	CheckTimeSync(DeltaTime);
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
	const float CurrentServerTime = TimeServerReceivedClientRequest + (RoundTripTime * 0.5f);

	// 서버와 클라이언트의 시간 차이 확인
	ClientServerDelta = CurrentServerTime - GetWorld()->GetTimeSeconds();
}

void ABlasterPlayerController::SetHUDHealth(float Health, float MaxHealth)
{
	if (!BlasterHUD) return;
	if (!BlasterHUD->GetCharacterOverlay()) return;
	if (!BlasterHUD->GetCharacterOverlay()->GetHealthBar()) return;
	if (!BlasterHUD->GetCharacterOverlay()->GetHealthText()) return;

	const float HealthPercent = MaxHealth == 0.f ? 0.f : Health / MaxHealth;
	BlasterHUD->GetCharacterOverlay()->GetHealthBar()->SetPercent(HealthPercent);

	const FString HealthText = FString::Printf(TEXT("%d/%d"), FMath::CeilToInt(Health), FMath::CeilToInt(MaxHealth));
	BlasterHUD->GetCharacterOverlay()->GetHealthText()->SetText(FText::FromString(HealthText));
}

void ABlasterPlayerController::SetHUDScore(float Score)
{
	if (!BlasterHUD) return;
	if (!BlasterHUD->GetCharacterOverlay()) return;
	if (!BlasterHUD->GetCharacterOverlay()->GetScoreAmount()) return;

	const FString ScoreText = FString::Printf(TEXT("%d"), FMath::FloorToInt(Score));
	BlasterHUD->GetCharacterOverlay()->GetScoreAmount()->SetText(FText::FromString(ScoreText));
}

void ABlasterPlayerController::SetHUDDefeats(int32 Defeats)
{
	if (!BlasterHUD) return;
	if (!BlasterHUD->GetCharacterOverlay()) return;
	if (!BlasterHUD->GetCharacterOverlay()->GetDefeatsAmount()) return;

	const FString DefeatsText = FString::Printf(TEXT("%d"), Defeats);
	BlasterHUD->GetCharacterOverlay()->GetDefeatsAmount()->SetText(FText::FromString(DefeatsText));
}

void ABlasterPlayerController::SetHUDWeaponAmmo(int32 Ammo)
{
	if (!BlasterHUD) return;
	if (!BlasterHUD->GetCharacterOverlay()) return;
	if (!BlasterHUD->GetCharacterOverlay()->GetWeaponAmmoAmount()) return;

	const FString AmmoText = FString::Printf(TEXT("%d"), Ammo);
	BlasterHUD->GetCharacterOverlay()->GetWeaponAmmoAmount()->SetText(FText::FromString(AmmoText));
}

void ABlasterPlayerController::SetHUDCarriedAmmo(int32 Ammo)
{
	if (!BlasterHUD) return;
	if (!BlasterHUD->GetCharacterOverlay()) return;
	if (!BlasterHUD->GetCharacterOverlay()->GetCarriedAmmoAmount()) return;

	const FString AmmoText = FString::Printf(TEXT("%d"), Ammo);
	BlasterHUD->GetCharacterOverlay()->GetCarriedAmmoAmount()->SetText(FText::FromString(AmmoText));
}

void ABlasterPlayerController::SetHUDMatchCountdown(float CounddownTime)
{
	if (!BlasterHUD) return;
	if (!BlasterHUD->GetCharacterOverlay()) return;
	if (!BlasterHUD->GetCharacterOverlay()->GetMatchCountdownText()) return;

	const int32 Minutes = FMath::FloorToInt(CounddownTime / 60.f);
	const int32 Seconds = CounddownTime - (Minutes * 60);

	const FString CountdownText = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
	BlasterHUD->GetCharacterOverlay()->GetMatchCountdownText()->SetText(FText::FromString(CountdownText));
}

float ABlasterPlayerController::GetServerTime() const
{
	if (!GetWorld()) return 0.f;

	if (HasAuthority()) return GetWorld()->GetTimeSeconds(); // 서버는 그대로 리턴
	// 현재 서버 시간과 클라시간의 차이를 합쳐서 올바른 서버 시간을 알려줌
	else return GetWorld()->GetTimeSeconds() + ClientServerDelta;
}

void ABlasterPlayerController::SetHUDTime()
{
	if (!GetWorld()) return;

	// 매치타임에서 현재 흘러간 시간을 빼서 남은 시간을 구한다.
	const uint32 SecondsLeft = FMath::CeilToInt(MatchTime - GetServerTime());

	// 남은 시간이 실제로 1초라도 수정되었을 때 UI 업데이트(틱마다 업데이트 방지)
	if (CounddownInt != SecondsLeft)
	{
		SetHUDMatchCountdown(MatchTime - GetServerTime());
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
