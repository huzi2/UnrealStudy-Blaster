// Fill out your copyright notice in the Description page of Project Settings.

#include "GameMode/BlasterGameMode.h"
#include "Character/BlasterCharacter.h"
#include "PlayerController/BlasterPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"
#include "PlayerState/BlasterPlayerState.h"

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