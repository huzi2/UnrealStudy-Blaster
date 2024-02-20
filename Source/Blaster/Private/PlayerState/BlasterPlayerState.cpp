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
	// 서버에 의해서 점수가 변동되었을 때 클라이언트들에게 호출
	Super::OnRep_Score();

	CheckInit();

	if (Controller)
	{
		Controller->SetHUDScore(GetScore());
	}
}

void ABlasterPlayerState::AddToScore(float ScoreAmount)
{
	// OnRep_Score()는 클라이언트에게만 호출되기에 서버에서 호출할 함수
	SetScore(GetScore() + ScoreAmount);

	CheckInit();

	if (Controller)
	{
		Controller->SetHUDScore(GetScore());
	}
}

void ABlasterPlayerState::AddToDefeats(int32 DefeatsAmount)
{
	// 서버의 패배수 적용
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
	// 클라이언트의 패배수 적용
	CheckInit();

	if (Controller)
	{
		Controller->SetHUDDefeats(Defeats);
	}
}
