// Fill out your copyright notice in the Description page of Project Settings.

#include "GameMode/TeamGameMode.h"
#include "GameState/BlasterGameState.h"
#include "PlayerState/BlasterPlayerState.h"
#include "Kismet/GameplayStatics.h"

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
