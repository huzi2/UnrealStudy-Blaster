// Fill out your copyright notice in the Description page of Project Settings.

#include "PlayerState/BlasterPlayerState.h"
#include "Character/BlasterCharacter.h"
#include "PlayerController/BlasterPlayerController.h"
#include "Net/UnrealNetwork.h"

void ABlasterPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABlasterPlayerState, Defeats);
}

void ABlasterPlayerState::OnRep_Score()
{
	// ������ ���ؼ� ������ �����Ǿ��� �� Ŭ���̾�Ʈ�鿡�� ȣ��
	Super::OnRep_Score();

	CheckInit();

	if (Controller)
	{
		Controller->SetHUDScore(GetScore());
	}
}

void ABlasterPlayerState::AddToScore(float ScoreAmount)
{
	// OnRep_Score()�� Ŭ���̾�Ʈ���Ը� ȣ��Ǳ⿡ �������� ȣ���� �Լ�
	SetScore(GetScore() + ScoreAmount);

	CheckInit();

	if (Controller)
	{
		Controller->SetHUDScore(GetScore());
	}
}

void ABlasterPlayerState::AddToDefeats(int32 DefeatsAmount)
{
	// ������ �й�� ����
	Defeats += DefeatsAmount;

	CheckInit();

	if (Controller)
	{
		Controller->SetHUDDefeats(Defeats);
	}
}

void ABlasterPlayerState::CheckInit()
{
	if (!Character)
	{
		Character = Cast<ABlasterCharacter>(GetPawn());
	}

	if (Character)
	{
		if (!Controller)
		{
			Controller = Cast<ABlasterPlayerController>(Character->Controller);
		}
	}
}

void ABlasterPlayerState::OnRep_Defeats()
{
	// Ŭ���̾�Ʈ�� �й�� ����
	CheckInit();

	if (Controller)
	{
		Controller->SetHUDDefeats(Defeats);
	}
}
