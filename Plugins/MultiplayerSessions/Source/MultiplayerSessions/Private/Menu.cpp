// Fill out your copyright notice in the Description page of Project Settings.

#include "Menu.h"
#include "Components/Button.h"
#include "MultiplayerSessionsSubsystem.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystem.h"

// ���� �ִ� �÷��̾� ��
constexpr int32 DEFAULT_MAX_PLAYERS = 10000;

bool UMenu::Initialize()
{
	if (!Super::Initialize()) return false;
	BindWidgetEvents();
	return true;
}

void UMenu::NativeDestruct()
{
	MenuTearDown();
	Super::NativeDestruct();
}

void UMenu::HostButtonClicked()
{
	// ���� ���� �� ��ư ��Ȱ��ȭ
	if (HostButton)
	{
		HostButton->SetIsEnabled(false);
	}

	CreateSession();
}

void UMenu::JoinButtonClicked()
{
	// ���� �˻� �� ��ư ��Ȱ��ȭ
	if (JoinButton)
	{
		JoinButton->SetIsEnabled(false);
	}

	FindSessions();
}

void UMenu::BindWidgetEvents()
{
	if (HostButton)
	{
		HostButton->OnClicked.AddDynamic(this, &ThisClass::HostButtonClicked);
	}

	if (JoinButton)
	{
		JoinButton->OnClicked.AddDynamic(this, &ThisClass::JoinButtonClicked);
	}
}

void UMenu::CreateSession()
{
	// ��Ƽ�÷��̾� ���� ����ý��ۿ��� ���� ���� ����
	if (MultiplayerSessionsSubsystem)
	{
		// ���� �ο��� ���� ����� ����
		MultiplayerSessionsSubsystem->CreateSession(NumPublicConnections, MatchType);
	}
}

void UMenu::FindSessions()
{
	// ��Ƽ�÷��̾� ���� ����ý��ۿ��� ���� ã�� ����
	if (MultiplayerSessionsSubsystem)
	{
		MultiplayerSessionsSubsystem->FindSessions(DEFAULT_MAX_PLAYERS);
	}
}

void UMenu::MenuSetup(int32 NumberOfPublicConnections, const FString& TypeOfMatch, const FString& LobbyPath)
{
	UpdateSessionConfig(NumberOfPublicConnections, TypeOfMatch, LobbyPath);
	PrepareUI();
	BindSessionEvents();
}

void UMenu::UpdateSessionConfig(int32 NumberOfPublicConnections, const FString& TypeOfMatch, const FString& LobbyPath)
{
	NumPublicConnections = NumberOfPublicConnections;
	MatchType = TypeOfMatch;
	PathToLobby = FString::Printf(TEXT("%s?listen"), *LobbyPath);
}

void UMenu::PrepareUI()
{
	AddToViewport();
	SetVisibility(ESlateVisibility::Visible);
	SetIsFocusable(true);
	SetupInputMode();
}

void UMenu::SetupInputMode()
{
	if (UWorld* World = GetWorld())
	{
		if (APlayerController* PlayerController = World->GetFirstPlayerController())
		{
			FInputModeUIOnly InputModeData;
			InputModeData.SetWidgetToFocus(TakeWidget());
			InputModeData.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			PlayerController->SetInputMode(InputModeData);
			PlayerController->SetShowMouseCursor(true);
		}
	}
}

void UMenu::BindSessionEvents()
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		// ��Ƽ�÷��̾� ���� ����ý����� Ŀ���� ��������Ʈ�鿡 �Լ� ����
		if (MultiplayerSessionsSubsystem = GameInstance->GetSubsystem<UMultiplayerSessionsSubsystem>())
		{
			MultiplayerSessionsSubsystem->MultiplayerOnCreateSessionComplete.AddDynamic(this, &ThisClass::OnCreateSession);
			MultiplayerSessionsSubsystem->MultiplayerOnFindSessionsComplete.AddUObject(this, &ThisClass::OnFindSessions);
			MultiplayerSessionsSubsystem->MultiplayerOnJoinSessionComplete.AddUObject(this, &ThisClass::OnJoinSession);
			MultiplayerSessionsSubsystem->MultiplayerOnDestroySessionComplete.AddDynamic(this, &ThisClass::OnDestroySession);
			MultiplayerSessionsSubsystem->MultiplayerOnStartSessionComplete.AddDynamic(this, &ThisClass::OnStartSession);
		}
	}
}

void UMenu::MenuTearDown()
{
	RemoveFromParent();
	ResetInputMode();
}

void UMenu::ResetInputMode()
{
	if (UWorld* World = GetWorld())
	{
		if (APlayerController* PlayerController = World->GetFirstPlayerController())
		{
			FInputModeGameOnly InputModeData;
			PlayerController->SetInputMode(InputModeData);
			PlayerController->SetShowMouseCursor(false);
		}
	}
}

void UMenu::OnCreateSession(bool bWasSuccessful)
{
	// ��Ƽ�÷��̾� ���� ����ý����� ���� ������ ������ ����
	if (bWasSuccessful)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Yellow, TEXT("Session created successfully!"));
		}

		// ���� ������ �����ϰ� ���� �̵�. ��ư �����ڸ��� �̵���Ű�� ���� ������ ����� �ȵ� �� ����
		if (UWorld* World = GetWorld())
		{
			World->ServerTravel(PathToLobby);
		}
	}
	else
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Red, TEXT("Failed to create session!"));
		}

		if (HostButton)
		{
			HostButton->SetIsEnabled(true);
		}
	}
}

void UMenu::OnFindSessions(const TArray<FOnlineSessionSearchResult>& SessionResults, bool bWasSuccessful)
{
	// ��Ƽ�÷��̾� ���� ����ý����� ���� ã�Ⱑ ������ ����
	if (!MultiplayerSessionsSubsystem) return;

	if (bWasSuccessful)
	{
		for (const FOnlineSessionSearchResult& Result : SessionResults)
		{
			// ã�� ������ ������ ��ġŸ�԰� ���� �� Ȯ��
			FString SettingValue = FString();
			Result.Session.SessionSettings.Get(TEXT("MatchType"), SettingValue);

			// ���� ��ġŸ���� ���� ���� ���ǿ� ����
			if (SettingValue == MatchType)
			{
				// ��Ƽ�÷��̾� ���� ����ý��ۿ��� ���� ���� ����
				MultiplayerSessionsSubsystem->JoinSession(Result);
				return;
			}
		}
	}

	if (!bWasSuccessful || SessionResults.Num() == 0)
	{
		if (JoinButton)
		{
			JoinButton->SetIsEnabled(true);
		}
	}
}

void UMenu::OnJoinSession(EOnJoinSessionCompleteResult::Type Result)
{
	// ��Ƽ�÷��̾� ���� ����ý����� ���� ������ ������ ����
	// �¶��� ���� �ý����� �����ͼ� Ŭ���̾�Ʈ�� ���ǿ� ����
	if (IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get())
	{
		IOnlineSessionPtr SessionInterface = OnlineSubsystem->GetSessionInterface();
		if (SessionInterface.IsValid())
		{
			FString Address = FString();
			SessionInterface->GetResolvedConnectString(NAME_GameSession, Address);

			if (APlayerController* PlayerController = GetGameInstance()->GetFirstLocalPlayerController())
			{
				PlayerController->ClientTravel(Address, ETravelType::TRAVEL_Absolute);
			}
		}
	}

	if (Result != EOnJoinSessionCompleteResult::Success)
	{
		if (JoinButton)
		{
			JoinButton->SetIsEnabled(true);
		}
	}
}

void UMenu::OnDestroySession(bool bWasSuccessful)
{
	// ��Ƽ�÷��̾� ���� ����ý����� ���� �ı��� ������ ����
	if (bWasSuccessful)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Yellow, TEXT("Session destroy successfully!"));
		}
	}
	else
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Red, TEXT("Failed to destroy session!"));
		}
	}
}

void UMenu::OnStartSession(bool bWasSuccessful)
{
	// ��Ƽ�÷��̾� ���� ����ý����� ���� ������ ������ ����
	if (bWasSuccessful)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Yellow, TEXT("Session start successfully!"));
		}
	}
	else
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Red, TEXT("Failed to start session!"));
		}
	}
}
