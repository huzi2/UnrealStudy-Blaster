// Fill out your copyright notice in the Description page of Project Settings.

#include "GameMode/LobbyGameMode.h"
#include "GameFramework/GameStateBase.h"
#include "MultiplayerSessionsSubsystem.h"

void ALobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	if (!GameState.Get()) return;
	
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		// 서브시스템에 저장해놓은 게임 인원 수와 게임 방법을 얻어온다.
		UMultiplayerSessionsSubsystem* Subsystem = GameInstance->GetSubsystem<UMultiplayerSessionsSubsystem>();
		check(Subsystem);

		// 접속한 사람이 특정 수가 되면 레벨 이동
		const int32 NumberOfPlayers = GameState.Get()->PlayerArray.Num();
		if (NumberOfPlayers == Subsystem->GetDesiredNumPublicConnections())
		{
			if (UWorld* World = GetWorld())
			{
				bUseSeamlessTravel = true;
				const FString MatchType = Subsystem->GetDesiredMatchType();
				if (MatchType == TEXT("FreeForAll"))
				{
					World->ServerTravel(TEXT("/Game/Maps/BlasterMap?listen"));
				}
				else if (MatchType == TEXT("Teams"))
				{
					World->ServerTravel(TEXT("/Game/Maps/Teams?listen"));
				}
				else if (MatchType == TEXT("CaptureTheFlag"))
				{
					World->ServerTravel(TEXT("/Game/Maps/CaptureTheFlag?listen"));
				}
			}
		}
	}
}
