// Fill out your copyright notice in the Description page of Project Settings.

#include "Menu.h"
#include "Components/Button.h"
#include "MultiplayerSessionsSubsystem.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystem.h"

// 세션 최대 플레이어 수
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
	// 세션 생성 중 버튼 비활성화
	if (HostButton)
	{
		HostButton->SetIsEnabled(false);
	}

	CreateSession();
}

void UMenu::JoinButtonClicked()
{
	// 세션 검색 중 버튼 비활성화
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
	// 멀티플레이어 세션 서브시스템에서 세션 생성 수행
	if (MultiplayerSessionsSubsystem)
	{
		// 게임 인원과 게임 방식을 저장
		MultiplayerSessionsSubsystem->CreateSession(NumPublicConnections, MatchType);
	}
}

void UMenu::FindSessions()
{
	// 멀티플레이어 세션 서브시스템에서 세션 찾기 수행
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
		// 멀티플레이어 세션 서브시스템의 커스텀 델리게이트들에 함수 연결
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
	// 멀티플레이어 세션 서브시스템이 세션 생성이 끝나면 수행
	if (bWasSuccessful)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Yellow, TEXT("Session created successfully!"));
		}

		// 세션 생성이 성공하고 레벨 이동. 버튼 누르자마자 이동시키면 세션 생성이 제대로 안될 수 있음
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
	// 멀티플레이어 세션 서브시스템이 세션 찾기가 끝나면 수행
	if (!MultiplayerSessionsSubsystem) return;

	if (bWasSuccessful)
	{
		for (const FOnlineSessionSearchResult& Result : SessionResults)
		{
			// 찾은 세션이 설정한 매치타입과 같은 지 확인
			FString SettingValue = FString();
			Result.Session.SessionSettings.Get(TEXT("MatchType"), SettingValue);

			// 같은 매치타입이 먼저 나온 세션에 가입
			if (SettingValue == MatchType)
			{
				// 멀티플레이어 세션 서브시스템에서 세션 가입 수행
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
	// 멀티플레이어 세션 서브시스템이 세션 가입이 끝나면 수행
	// 온라인 서브 시스템을 가져와서 클라이언트를 세션에 연결
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
	// 멀티플레이어 세션 서브시스템이 세션 파괴가 끝나면 수행
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
	// 멀티플레이어 세션 서브시스템이 세션 시작이 끝나면 수행
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
