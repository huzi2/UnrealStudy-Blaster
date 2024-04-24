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
{
	bDelayedStart = true;
}

float ABlasterGameMode::CalculateDamage(AController* Attacker, AController* Victim, float BaseDamage) const
{
	// �� ���Ӹ�忡���� �Ʊ��� ������ �ʵ��� ����. �⺻ ���Ӹ�忡���� ��ο��� ���� ������ ���
	return BaseDamage;
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
			if (ABlasterGameState* BlasterGameState = GetGameState<ABlasterGameState>())
			{
				// ��� ���� ������ �����ϰ� �ִ� �÷��̾���� �����´�.
				TArray<ABlasterPlayerState*> PlayerCurrentlyInTheLead;
				for (ABlasterPlayerState* LeadPlayer : BlasterGameState->GetTopScoringPlayers())
				{
					PlayerCurrentlyInTheLead.Add(LeadPlayer);
				}

				AttackerPlayerState->AddToScore(1.f);
				BlasterGameState->UpdateTopScore(AttackerPlayerState);

				// ���� ��� �Ŀ� �����ڰ� ������ �����ϰ� �ִٸ� �հ��� �ش�.
				if (BlasterGameState->GetTopScoringPlayers().Contains(AttackerPlayerState))
				{
					if (ABlasterCharacter* Leader = Cast<ABlasterCharacter>(AttackerPlayerState->GetPawn()))
					{
						Leader->MulticastGainedTheLead();
					}
				}

				// ���� ��� �Ŀ� ���带 �����ϰ� �ִ� �� Ȯ���ؼ� �ƴ϶�� �հ��� ������
				for (ABlasterPlayerState* LeadPlayer : PlayerCurrentlyInTheLead)
				{
					if (!BlasterGameState->GetTopScoringPlayers().Contains(LeadPlayer))
					{
						if (ABlasterCharacter* Loser = Cast<ABlasterCharacter>(LeadPlayer->GetPawn()))
						{
							Loser->MulticastLostTheLead();
						}
					}
				}
			}
		}
		if (VictimPlayerState)
		{
			VictimPlayerState->AddToDefeats(1);
		}

		// ��� �����鿡�� ĳ���� óġ �޽����� ���
		if (UWorld* World = GetWorld())
		{
			if (AttackerPlayerState && VictimPlayerState)
			{
				for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
				{
					if (ABlasterPlayerController* BlasterPlayerController = Cast<ABlasterPlayerController>(*It))
					{
						BlasterPlayerController->BroadcastElim(AttackerPlayerState, VictimPlayerState);
					}
				}
			}
		}
	}

	if (ElimmedCharacter)
	{
		ElimmedCharacter->Elim(false);
	}
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
			BlasterPlayer->OnMatchStateSet(MatchState, bTeamsMatch);
		}
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

void ABlasterGameMode::PlayerLeftGame(ABlasterPlayerState* PlayerLeaving)
{
	if (!PlayerLeaving) return;

	// ���� ����� �ְ� ������ �� �ϳ���� ����
	if (ABlasterGameState* BlasterGameState = GetGameState<ABlasterGameState>())
	{
		if (BlasterGameState->GetTopScoringPlayers().Contains(PlayerLeaving))
		{
			BlasterGameState->GetTopScoringPlayers().Remove(PlayerLeaving);
		}
	}

	// ĳ���͸� ���� ó��
	if (ABlasterCharacter* CharacterLeaving = Cast<ABlasterCharacter>(PlayerLeaving->GetPawn()))
	{
		CharacterLeaving->Elim(true);
	}
}
