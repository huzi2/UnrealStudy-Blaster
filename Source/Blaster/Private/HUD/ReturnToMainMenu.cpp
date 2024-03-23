// Fill out your copyright notice in the Description page of Project Settings.

#include "HUD/ReturnToMainMenu.h"
#include "GameFramework/PlayerController.h"
#include "Components/Button.h"
#include "MultiplayerSessionsSubsystem.h"
#include "GameFramework/GameModeBase.h"
#include "Character/BlasterCharacter.h"

void UReturnToMainMenu::MenuSetup()
{
	if (!PlayerController && GetWorld())
	{
		PlayerController = GetWorld()->GetFirstPlayerController();
	}

	if (!MultiplayerSessionsSubsystem)
	{
		if (UGameInstance* GameInstance = GetGameInstance())
		{
			MultiplayerSessionsSubsystem = GameInstance->GetSubsystem<UMultiplayerSessionsSubsystem>();
		}
	}

	if (MultiplayerSessionsSubsystem && !MultiplayerSessionsSubsystem->MultiplayerOnDestroySessionComplete.IsBound())
	{
		MultiplayerSessionsSubsystem->MultiplayerOnDestroySessionComplete.AddDynamic(this, &ThisClass::OnDestroySession);
	}

	if (ReturnButton && !ReturnButton->OnClicked.IsBound())
	{
		ReturnButton->OnClicked.AddDynamic(this, &ThisClass::ReturnButtonClicked);
	}

	AddToViewport();
	SetVisibility(ESlateVisibility::Visible);
	SetIsFocusable(true);

	if (PlayerController)
	{
		FInputModeGameAndUI InputModeData;
		InputModeData.SetWidgetToFocus(TakeWidget());
		PlayerController->SetInputMode(InputModeData);
		PlayerController->SetShowMouseCursor(true);
	}
}

void UReturnToMainMenu::MenuTearDown()
{
	RemoveFromParent();

	if (PlayerController)
	{
		FInputModeGameOnly InputModeData;
		PlayerController->SetInputMode(InputModeData);
		PlayerController->SetShowMouseCursor(false);
	}

	if (MultiplayerSessionsSubsystem && MultiplayerSessionsSubsystem->MultiplayerOnDestroySessionComplete.IsBound())
	{
		MultiplayerSessionsSubsystem->MultiplayerOnDestroySessionComplete.RemoveDynamic(this, &ThisClass::OnDestroySession);
	}

	if (ReturnButton && ReturnButton->OnClicked.IsBound())
	{
		ReturnButton->OnClicked.RemoveDynamic(this, &ThisClass::ReturnButtonClicked);
	}
}

void UReturnToMainMenu::ReturnButtonClicked()
{
	if (ReturnButton)
	{
		ReturnButton->SetIsEnabled(false);
	}

	if (PlayerController)
	{
		// 서버에게 게임 나가는 것을 알려줌
		if (ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(PlayerController->GetPawn()))
		{
			BlasterCharacter->ServerLeaveGame();
			// 캐릭터가 할 거 다하고 마지막에 완전히 게임을 나갈 때를 알기위해 바인드
			BlasterCharacter->OnLeftGame.AddDynamic(this, &ThisClass::OnPlayerLeftGame);
		}
		else
		{
			if (ReturnButton)
			{
				ReturnButton->SetIsEnabled(true);
			}
		}
	}
}

void UReturnToMainMenu::OnDestroySession(bool bWasSuccessful)
{
	// 세션 파괴에 실패하면 버튼 다시 활성화해서 다시 시도할 수 있게함
	if (!bWasSuccessful)
	{
		if (ReturnButton)
		{
			ReturnButton->SetIsEnabled(true);
		}
		return;
	}

	if (UWorld* World = GetWorld())
	{
		// 게임모드가 유효하다는건 서버라는 뜻
		if (AGameModeBase* GameMode = World->GetAuthGameMode<AGameModeBase>())
		{
			GameMode->ReturnToMainMenuHost();
		}
		// 게임모드가 없으니 클라이언트
		else
		{
			if (PlayerController)
			{
				PlayerController->ClientReturnToMainMenuWithTextReason(FText());
			}
		}
	}
}

void UReturnToMainMenu::OnPlayerLeftGame()
{
	// 이 함수는 캐릭터가 죽음 관련 행동을 한 뒤 브로드캐스트하면서 수행된다.
	// 현재 접속 중인 세션 연결 해제
	if (MultiplayerSessionsSubsystem)
	{
		MultiplayerSessionsSubsystem->DestroySession();
	}
}
