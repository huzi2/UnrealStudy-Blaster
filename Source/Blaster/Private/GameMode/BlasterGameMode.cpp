// Fill out your copyright notice in the Description page of Project Settings.

#include "GameMode/BlasterGameMode.h"
#include "Character/BlasterCharacter.h"
#include "PlayerController/BlasterPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"
#include "PlayerState/BlasterPlayerState.h"
#include "GameState/BlasterGameState.h"

namespace MatchState
{
	const FName Cooldown = TEXT("Cooldown");
}

ABlasterGameMode::ABlasterGameMode()
	: WarmupTime(10.f)
	, MatchTime(120.f)
	, CooldownTime(10.f)
	, CountdownTime(0.f)
	, LevelStartingTime(0.f)
{
	bDelayedStart = true;
}

void ABlasterGameMode::BeginPlay()
{
	Super::BeginPlay();

	if (GetWorld())
	{
		LevelStartingTime = GetWorld()->GetTimeSeconds();
	}
}

void ABlasterGameMode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (GetWorld())
	{
		// 매치 대기 시간이 지나면 매치 시작
		if (MatchState == MatchState::WaitingToStart)
		{
			CountdownTime = WarmupTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;
			if (CountdownTime <= 0.f)
			{
				StartMatch();
			}
		}
		// 매치에 주어진 시간이 지나면 쿨다운 상태로 변경
		else if (MatchState == MatchState::InProgress)
		{
			CountdownTime = WarmupTime + MatchTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;
			if (CountdownTime <= 0.f)
			{
				SetMatchState(MatchState::Cooldown);
			}
		}
		// 매치 사이에 주어진 사긴이 끝나면 다시 초기 상태로
		else if (MatchState == MatchState::Cooldown)
		{
			CountdownTime = WarmupTime + MatchTime + CooldownTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;
			if (CountdownTime <= 0.f)
			{
				RestartGame();
			}
		}
	}
}

void ABlasterGameMode::OnMatchStateSet()
{
	Super::OnMatchStateSet();

	if (!GetWorld()) return;

	// 모든 컨트롤러에게 매치 상태를 알려준다. 게임모드는 서버만 가지고 있으므로 클라의 컨트롤러에게는 서버 컨트롤러가 레플리케이션으로 알려준다.
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		ABlasterPlayerController* BlasterPlayer = Cast<ABlasterPlayerController>(*It);
		if (BlasterPlayer)
		{
			BlasterPlayer->OnMatchStateSet(MatchState);
		}
	}
}

void ABlasterGameMode::PlayerEliminated(ABlasterCharacter* ElimmedCharacter, ABlasterPlayerController* VictimController, ABlasterPlayerController* AttackerController)
{
	// 플레이어 스테이트에 점수 추가
	if (AttackerController && VictimController)
	{
		ABlasterPlayerState* AttackerPlayerState = Cast<ABlasterPlayerState>(AttackerController->PlayerState);
		ABlasterPlayerState* VictimPlayerState = Cast<ABlasterPlayerState>(VictimController->PlayerState);
		if (AttackerPlayerState && AttackerPlayerState != VictimPlayerState)
		{
			AttackerPlayerState->AddToScore(1.f);
			
			// 최고 점수 갱신
			ABlasterGameState* BlasterGameState = GetGameState<ABlasterGameState>();
			if (BlasterGameState)
			{
				BlasterGameState->UpdateTopScore(AttackerPlayerState);
			}
		}
		if (VictimPlayerState)
		{
			VictimPlayerState->AddToDefeats(1);
		}
	}

	if (ElimmedCharacter)
	{
		ElimmedCharacter->Elim();
	}
}

void ABlasterGameMode::RequestRespawn(ACharacter* ElimmedCharacter, AController* ElimmedController)
{
	if (ElimmedCharacter)
	{
		// 캐릭터에서 컨트롤러 분리
		ElimmedCharacter->Reset();
		// 캐릭터 제거
		ElimmedCharacter->Destroy();
	}

	if (ElimmedController)
	{
		// 모든 플레이어스타트 객체 중 하나를 랜덤으로 선택해서 리스폰
		TArray<AActor*> PlayerStarts;
		UGameplayStatics::GetAllActorsOfClass(this, APlayerStart::StaticClass(), PlayerStarts);
		const int32 Selection = FMath::RandRange(0, PlayerStarts.Num() - 1);

		RestartPlayerAtPlayerStart(ElimmedController, PlayerStarts[Selection]);
	}
}
