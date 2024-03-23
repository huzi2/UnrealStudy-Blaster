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
		// �������� ���� ������ ���� �˷���
		if (ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(PlayerController->GetPawn()))
		{
			BlasterCharacter->ServerLeaveGame();
			// ĳ���Ͱ� �� �� ���ϰ� �������� ������ ������ ���� ���� �˱����� ���ε�
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
	// ���� �ı��� �����ϸ� ��ư �ٽ� Ȱ��ȭ�ؼ� �ٽ� �õ��� �� �ְ���
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
		// ���Ӹ�尡 ��ȿ�ϴٴ°� ������� ��
		if (AGameModeBase* GameMode = World->GetAuthGameMode<AGameModeBase>())
		{
			GameMode->ReturnToMainMenuHost();
		}
		// ���Ӹ�尡 ������ Ŭ���̾�Ʈ
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
	// �� �Լ��� ĳ���Ͱ� ���� ���� �ൿ�� �� �� ��ε�ĳ��Ʈ�ϸ鼭 ����ȴ�.
	// ���� ���� ���� ���� ���� ����
	if (MultiplayerSessionsSubsystem)
	{
		MultiplayerSessionsSubsystem->DestroySession();
	}
}
