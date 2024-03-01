// Fill out your copyright notice in the Description page of Project Settings.

#include "Menu.h"
#include "Components/Button.h"
#include "MultiplayerSessionsSubsystem.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystem.h"

UMenu::UMenu(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, NumPublicConnections(4)
	, MatchType(TEXT("FreeForAll"))
	, PathToLobby(FString())
{
}

bool UMenu::Initialize()
{
	if (!Super::Initialize()) return false;

	if (HostButton)
	{
		HostButton->OnClicked.AddDynamic(this, &ThisClass::HostButtonClicked);
	}

	if (JoinButton)
	{
		JoinButton->OnClicked.AddDynamic(this, &ThisClass::JoinButtonClicked);
	}

	return true;
}

void UMenu::NativeDestruct()
{
	MenuTearDown();

	Super::NativeDestruct();
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
		UWorld* World = GetWorld();
		if (World)
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
	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	if (OnlineSubsystem)
	{
		IOnlineSessionPtr SessionInterface = OnlineSubsystem->GetSessionInterface();
		if (SessionInterface.IsValid())
		{
			FString Address = FString();
			SessionInterface->GetResolvedConnectString(NAME_GameSession, Address);

			APlayerController* PlayerController = GetGameInstance()->GetFirstLocalPlayerController();
			if (PlayerController)
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

void UMenu::MenuSetup(int32 NumberOfPublicConnections, const FString& TypeOfMatch, const FString& LobbyPath)
{
	NumPublicConnections = NumberOfPublicConnections;
	MatchType = TypeOfMatch;
	PathToLobby = FString::Printf(TEXT("%s?listen"), *LobbyPath);

	AddToViewport();
	SetVisibility(ESlateVisibility::Visible);
	SetIsFocusable(true);

	UWorld* World = GetWorld();
	if (World)
	{
		APlayerController* PlayerController = World->GetFirstPlayerController();
		if (PlayerController)
		{
			FInputModeUIOnly InputModeData;
			InputModeData.SetWidgetToFocus(TakeWidget());
			InputModeData.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			PlayerController->SetInputMode(InputModeData);
			PlayerController->SetShowMouseCursor(true);
		}
	}

	UGameInstance* GameInstance = GetGameInstance();
	if (GameInstance)
	{
		MultiplayerSessionsSubsystem = GameInstance->GetSubsystem<UMultiplayerSessionsSubsystem>();
	}

	// ��Ƽ�÷��̾� ���� ����ý����� Ŀ���� ��������Ʈ�鿡 �Լ� ����
	if (MultiplayerSessionsSubsystem)
	{
		MultiplayerSessionsSubsystem->MultiplayerOnCreateSessionComplete.AddDynamic(this, &ThisClass::OnCreateSession);
		MultiplayerSessionsSubsystem->MultiplayerOnFindSessionsComplete.AddUObject(this, &ThisClass::OnFindSessions);
		MultiplayerSessionsSubsystem->MultiplayerOnJoinSessionComplete.AddUObject(this, &ThisClass::OnJoinSession);
		MultiplayerSessionsSubsystem->MultiplayerOnDestroySessionComplete.AddDynamic(this, &ThisClass::OnDestroySession);
		MultiplayerSessionsSubsystem->MultiplayerOnStartSessionComplete.AddDynamic(this, &ThisClass::OnStartSession);
	}
}

void UMenu::HostButtonClicked()
{
	if (HostButton)
	{
		HostButton->SetIsEnabled(false);
	}

	// Host ��ư�� ������ ��Ƽ�÷��̾� ���� ����ý��ۿ��� ���� ���� ����
	if (MultiplayerSessionsSubsystem)
	{
		MultiplayerSessionsSubsystem->CreateSession(NumPublicConnections, MatchType);
	}
}

void UMenu::JoinButtonClicked()
{
	if (JoinButton)
	{
		JoinButton->SetIsEnabled(false);
	}

	// Host ��ư�� ������ ��Ƽ�÷��̾� ���� ����ý��ۿ��� ���� ã�� ����
	if (MultiplayerSessionsSubsystem)
	{
		MultiplayerSessionsSubsystem->FindSessions(10000);
	}
}

void UMenu::MenuTearDown()
{
	RemoveFromParent();

	UWorld* World = GetWorld();
	if (World)
	{
		APlayerController* PlayerController = World->GetFirstPlayerController();
		if (PlayerController)
		{
			FInputModeGameOnly InputModeData;
			PlayerController->SetInputMode(InputModeData);
			PlayerController->SetShowMouseCursor(false);
		}
	}
}
