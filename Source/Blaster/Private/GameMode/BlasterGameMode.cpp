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
		// ��ġ ��� �ð��� ������ ��ġ ����
		if (MatchState == MatchState::WaitingToStart)
		{
			CountdownTime = WarmupTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;
			if (CountdownTime <= 0.f)
			{
				StartMatch();
			}
		}
		// ��ġ�� �־��� �ð��� ������ ��ٿ� ���·� ����
		else if (MatchState == MatchState::InProgress)
		{
			CountdownTime = WarmupTime + MatchTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;
			if (CountdownTime <= 0.f)
			{
				SetMatchState(MatchState::Cooldown);
			}
		}
		// ��ġ ���̿� �־��� ����� ������ �ٽ� �ʱ� ���·�
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

	// ��� ��Ʈ�ѷ����� ��ġ ���¸� �˷��ش�. ���Ӹ��� ������ ������ �����Ƿ� Ŭ���� ��Ʈ�ѷ����Դ� ���� ��Ʈ�ѷ��� ���ø����̼����� �˷��ش�.
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
	// �÷��̾� ������Ʈ�� ���� �߰�
	if (AttackerController && VictimController)
	{
		ABlasterPlayerState* AttackerPlayerState = Cast<ABlasterPlayerState>(AttackerController->PlayerState);
		ABlasterPlayerState* VictimPlayerState = Cast<ABlasterPlayerState>(VictimController->PlayerState);
		if (AttackerPlayerState && AttackerPlayerState != VictimPlayerState)
		{
			AttackerPlayerState->AddToScore(1.f);
			
			// �ְ� ���� ����
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
		// ĳ���Ϳ��� ��Ʈ�ѷ� �и�
		ElimmedCharacter->Reset();
		// ĳ���� ����
		ElimmedCharacter->Destroy();
	}

	if (ElimmedController)
	{
		// ��� �÷��̾ŸƮ ��ü �� �ϳ��� �������� �����ؼ� ������
		TArray<AActor*> PlayerStarts;
		UGameplayStatics::GetAllActorsOfClass(this, APlayerStart::StaticClass(), PlayerStarts);
		const int32 Selection = FMath::RandRange(0, PlayerStarts.Num() - 1);

		RestartPlayerAtPlayerStart(ElimmedController, PlayerStarts[Selection]);
	}
}
