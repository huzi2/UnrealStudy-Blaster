// Fill out your copyright notice in the Description page of Project Settings.

#include "GameMode/TeamGameMode.h"
#include "GameState/BlasterGameState.h"
#include "PlayerState/BlasterPlayerState.h"
#include "Kismet/GameplayStatics.h"

void ATeamGameMode::HandleMatchHasStarted()
{
	Super::HandleMatchHasStarted();

	// ��ġ�� ���۵� �� ��� �÷��̾�� �յ��ϰ� ���� �й�
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

	// ���� �α����� �÷��̾��� ���� ����
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

	// �÷��̾ �α׾ƿ��ϸ� �� �����̳ʿ��� ã�Ƽ� ����
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
