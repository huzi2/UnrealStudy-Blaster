// Fill out your copyright notice in the Description page of Project Settings.

#include "GameMode/BlasterGameMode.h"
#include "Character/BlasterCharacter.h"
#include "PlayerController/BlasterPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"
#include "PlayerState/BlasterPlayerState.h"

ABlasterGameMode::ABlasterGameMode()
	: WarmupTime(10.f)
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
		if (MatchState == MatchState::WaitingToStart)
		{
			CountdownTime = WarmupTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;
			if (CountdownTime <= 0.f)
			{
				StartMatch();
			}
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
