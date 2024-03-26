// Fill out your copyright notice in the Description page of Project Settings.

#include "GameMode/TeamGameMode.h"
#include "GameState/BlasterGameState.h"
#include "PlayerState/BlasterPlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "PlayerController/BlasterPlayerController.h"

ATeamGameMode::ATeamGameMode()
{
	bTeamsMatch = true;
}

void ATeamGameMode::HandleMatchHasStarted()
{
	Super::HandleMatchHasStarted();

	// 매치가 시작될 때 모든 플레이어들 균등하게 팀을 분배
	if (ABlasterGameState* BlasterGameState = Cast<ABlasterGameState>(UGameplayStatics::GetGameState(this)))
	{
		for (APlayerState* PlayerState : BlasterGameState->PlayerArray)
		{
			if (ABlasterPlayerState* BlasterPlayerState = Cast<ABlasterPlayerState>(PlayerState))
			{
				if (BlasterPlayerState->GetTeam() == ETeam::ET_NoTeam)
				{
					if (BlasterGameState->GetBlueTeam().Num() > BlasterGameState->GetRedTeam().Num())
					{
						BlasterGameState->GetRedTeam().AddUnique(BlasterPlayerState);
						BlasterPlayerState->SetTeam(ETeam::ET_RedTeam);
					}
					else
					{
						BlasterGameState->GetBlueTeam().AddUnique(BlasterPlayerState);
						BlasterPlayerState->SetTeam(ETeam::ET_BlueTeam);
					}
				}
			}
		}
	}
}

void ATeamGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	if (!NewPlayer) return;

	// 새로 로그인한 플레이어의 팀을 정함
	if (ABlasterGameState* BlasterGameState = Cast<ABlasterGameState>(UGameplayStatics::GetGameState(this)))
	{
		if (ABlasterPlayerState* BlasterPlayerState = NewPlayer->GetPlayerState<ABlasterPlayerState>())
		{
			if (BlasterPlayerState->GetTeam() == ETeam::ET_NoTeam)
			{
				if (BlasterGameState->GetBlueTeam().Num() > BlasterGameState->GetRedTeam().Num())
				{
					BlasterGameState->GetRedTeam().AddUnique(BlasterPlayerState);
					BlasterPlayerState->SetTeam(ETeam::ET_RedTeam);
				}
				else
				{
					BlasterGameState->GetBlueTeam().AddUnique(BlasterPlayerState);
					BlasterPlayerState->SetTeam(ETeam::ET_BlueTeam);
				}
			}
		}
	}
}

void ATeamGameMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);

	if (!Exiting) return;

	// 플레이어가 로그아웃하면 팀 컨테이너에서 찾아서 제거
	if (ABlasterGameState* BlasterGameState = Cast<ABlasterGameState>(UGameplayStatics::GetGameState(this)))
	{
		if (ABlasterPlayerState* BlasterPlayerState = Exiting->GetPlayerState<ABlasterPlayerState>())
		{
			if (BlasterGameState->GetRedTeam().Contains(BlasterPlayerState))
			{
				BlasterGameState->GetRedTeam().Remove(BlasterPlayerState);
			}
			if (BlasterGameState->GetBlueTeam().Contains(BlasterPlayerState))
			{
				BlasterGameState->GetBlueTeam().Remove(BlasterPlayerState);
			}
		}
	}
}

float ATeamGameMode::CalculateDamage(AController* Attacker, AController* Victim, float BaseDamage) const
{
	if (!Attacker) return BaseDamage;
	if (!Victim) return BaseDamage;

	ABlasterPlayerState* AttackPlayerState = Attacker->GetPlayerState<ABlasterPlayerState>();
	if (!AttackPlayerState) return BaseDamage;

	ABlasterPlayerState* VictimPlayerState = Victim->GetPlayerState<ABlasterPlayerState>();
	if (!VictimPlayerState) return BaseDamage;

	// 자해
	if (AttackPlayerState == VictimPlayerState) return BaseDamage;

	// 같은 팀이면 데미지 무효
	if (AttackPlayerState->GetTeam() == VictimPlayerState->GetTeam()) return 0.f;

	return BaseDamage;
}

void ATeamGameMode::PlayerEliminated(ABlasterCharacter* ElimmedCharacter, ABlasterPlayerController* VictimController, ABlasterPlayerController* AttackerController)
{
	Super::PlayerEliminated(ElimmedCharacter, VictimController, AttackerController);

	// 공격자의 팀의 점수를 올려준다.
	if (ABlasterGameState* BlasterGameState = Cast<ABlasterGameState>(UGameplayStatics::GetGameState(this)))
	{
		ABlasterPlayerState* AttackPlayerState = AttackerController->GetPlayerState<ABlasterPlayerState>();
		if (!AttackPlayerState) return;

		if (AttackPlayerState->GetTeam() == ETeam::ET_BlueTeam)
		{
			BlasterGameState->BlueTeamScores();
		}
		else if (AttackPlayerState->GetTeam() == ETeam::ET_RedTeam)
		{
			BlasterGameState->RedTeamScores();
		}
	}
}
