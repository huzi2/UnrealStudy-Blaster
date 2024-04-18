// Fill out your copyright notice in the Description page of Project Settings.

#include "MultiplayerSessionsSubsystem.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include "Online/OnlineSessionNames.h"

UMultiplayerSessionsSubsystem::UMultiplayerSessionsSubsystem()
	: CreateSessionCompleteDelegate(FOnCreateSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnCreateSessionComplete))
	, FindSessionsCompleteDelegate(FOnFindSessionsCompleteDelegate::CreateUObject(this, &ThisClass::OnFindSessionsComplete))
	, JoinSessionCompleteDelegate(FOnJoinSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnJoinSessionComplete))
	, DestroySessionCompleteDelegate(FOnDestroySessionCompleteDelegate::CreateUObject(this, &ThisClass::OnDestroySessionComplete))
	, StartSessionCompleteDelegate(FOnStartSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnStartSessionComplete))
{
	if (IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get())
	{
		SessionInterface = OnlineSubsystem->GetSessionInterface();
	}
}

void UMultiplayerSessionsSubsystem::CreateSession(int32 NumPublicConnections, const FString& MatchType)
{
	// 실제 게임에서 사용할 인원 수와 게임 방식 저장
	DesiredNumPublicConnections = NumPublicConnections;
	DesiredMatchType = MatchType;

	if (!SessionInterface.IsValid() || !GetWorld())
	{
		MultiplayerOnCreateSessionComplete.Broadcast(false);
		return;
	}

	// 기존에 세션이 있으면 세션 제거
	if (FNamedOnlineSession* ExistingSession = SessionInterface->GetNamedSession(NAME_GameSession))
	{
		bCreateSessionOnDestroy = true;
		LastNumPublicConnections = NumPublicConnections;
		LastMatchType = MatchType;

		DestroySession();
	}

	// 세션 생성에 대한 델리게이트 연결
	CreateSessionCompleteDelegateHandle = SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegate);

	// 세션 설정
	LastSessionSettings = MakeShareable(new FOnlineSessionSettings);
	if (!LastSessionSettings.IsValid())
	{
		MultiplayerOnCreateSessionComplete.Broadcast(false);
		return;
	}

	ConfigureSessionSettings(NumPublicConnections, MatchType);
	StartSessionCreation();
}

void UMultiplayerSessionsSubsystem::ConfigureSessionSettings(int32 NumPublicConnections, const FString& MatchType)
{
	// 랜매칭 허용 여부
	LastSessionSettings->bIsLANMatch = IOnlineSubsystem::Get()->GetSubsystemName() == TEXT("NULL") ? true : false;
	// 세션에 참가할 수 있는 인원수
	LastSessionSettings->NumPublicConnections = NumPublicConnections;
	// 진행 중인 세션이 참가할 수 있는가?
	LastSessionSettings->bAllowJoinInProgress = true;
	// 존재감(친구목록 등)을 통해 세션에 참가할 수 있는가?
	LastSessionSettings->bAllowJoinViaPresence = true;
	// 세션이 광고되는지?
	LastSessionSettings->bShouldAdvertise = true;
	// 존재감 정보를 사용할 지?
	LastSessionSettings->bUsesPresence = true;
	// 로비 시스템을 사용할 것인가?
	LastSessionSettings->bUseLobbiesIfAvailable = true;
	// 세션의 게임 버전과의 호환성을 검사할 것인가?
	LastSessionSettings->BuildUniqueId = 1;

	LastSessionSettings->Set(TEXT("MatchType"), MatchType, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
}

void UMultiplayerSessionsSubsystem::StartSessionCreation()
{
	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	if (!LocalPlayer)
	{
		MultiplayerOnCreateSessionComplete.Broadcast(false);
		return;
	}

	// 세션 생성
	if (!SessionInterface->CreateSession(*LocalPlayer->GetPreferredUniqueNetId(), NAME_GameSession, *LastSessionSettings))
	{
		// 세션 생성이 끝났으므로 델리게이트 제거
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);

		// 세션 생성에 실패했으므로 외부 델리게이트에 false 전달
		MultiplayerOnCreateSessionComplete.Broadcast(false);
	}
}

void UMultiplayerSessionsSubsystem::FindSessions(int32 MaxSearchResults)
{
	if (!SessionInterface.IsValid() || !GetWorld())
	{
		MultiplayerOnFindSessionsComplete.Broadcast(TArray<FOnlineSessionSearchResult>(), false);
		return;
	}

	// 세션 찾기에 대한 델리게이트 연결
	FindSessionsCompleteDelegateHandle = SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegate);

	// 찾을 세션의 정보
	LastSessionSearch = MakeShareable(new FOnlineSessionSearch());
	if (!LastSessionSearch.IsValid())
	{
		MultiplayerOnFindSessionsComplete.Broadcast(TArray<FOnlineSessionSearchResult>(), false);
		return;
	}

	LastSessionSearch->MaxSearchResults = MaxSearchResults;
	LastSessionSearch->bIsLanQuery = IOnlineSubsystem::Get()->GetSubsystemName() == TEXT("NULL") ? true : false;
	LastSessionSearch->QuerySettings.Set(SEARCH_PRESENCE, true, EOnlineComparisonOp::Equals);

	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	if (!LocalPlayer)
	{
		MultiplayerOnFindSessionsComplete.Broadcast(TArray<FOnlineSessionSearchResult>(), false);
		return;
	}

	// 세션 찾기
	if (!SessionInterface->FindSessions(*LocalPlayer->GetPreferredUniqueNetId(), LastSessionSearch.ToSharedRef()))
	{
		// 세션 찾기에 실패하면 델리게이트 제거
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);

		// 세션 찾기에 실패했으므로 외부 델리게이트에 false 전달
		MultiplayerOnFindSessionsComplete.Broadcast(TArray<FOnlineSessionSearchResult>(), false);
	}
}

void UMultiplayerSessionsSubsystem::JoinSession(const FOnlineSessionSearchResult& SessionResult)
{
	if (!SessionInterface.IsValid() || !GetWorld())
	{
		MultiplayerOnJoinSessionComplete.Broadcast(EOnJoinSessionCompleteResult::UnknownError);
		return;
	}

	// 세션 가입에 대한 델리게이트 연결
	JoinSessionCompleteDelegateHandle = SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegate);

	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	if (!LocalPlayer)
	{
		MultiplayerOnJoinSessionComplete.Broadcast(EOnJoinSessionCompleteResult::UnknownError);
		return;
	}

	// 세션 가입
	if (!SessionInterface->JoinSession(*LocalPlayer->GetPreferredUniqueNetId(), NAME_GameSession, SessionResult))
	{
		// 세션 가입에 실패하면 델리게이트 제거
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);

		// 세션 가입에 실패했으므로 외부 델리게이트에 에러 전달
		MultiplayerOnJoinSessionComplete.Broadcast(EOnJoinSessionCompleteResult::UnknownError);
	}
}

void UMultiplayerSessionsSubsystem::DestroySession()
{
	if (!SessionInterface.IsValid())
	{
		MultiplayerOnDestroySessionComplete.Broadcast(false);
		return;
	}

	// 세션 파괴에 대한 델리게이트 연결
	DestroySessionCompleteDelegateHandle = SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegate);

	// 세션 파괴
	if (!SessionInterface->DestroySession(NAME_GameSession))
	{
		// 세션 파괴에 실패하면 델리게이트 제거
		SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);

		// 세션 파괴에 실패했으므로 외부 델리게이트에 에러 전달
		MultiplayerOnDestroySessionComplete.Broadcast(false);
	}
}

void UMultiplayerSessionsSubsystem::StratSession()
{
	if (!SessionInterface.IsValid())
	{
		MultiplayerOnStartSessionComplete.Broadcast(false);
		return;
	}

	// 세션 시작에 대한 델리게이트 연결
	StartSessionCompleteDelegateHandle = SessionInterface->AddOnStartSessionCompleteDelegate_Handle(StartSessionCompleteDelegate);

	// 세션 시작
	if (!SessionInterface->StartSession(NAME_GameSession))
	{
		// 세션 시작에 실패하면 델리게이트 제거
		SessionInterface->ClearOnStartSessionCompleteDelegate_Handle(StartSessionCompleteDelegateHandle);

		// 세션 시작에 실패했으므로 외부 델리게이트에 에러 전달
		MultiplayerOnStartSessionComplete.Broadcast(false);
	}
}

void UMultiplayerSessionsSubsystem::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (SessionInterface.IsValid())
	{
		// 세션 생성이 끝났으므로 델리게이트 제거
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);
	}

	// 세션 생성의 결과를 외부 델리게이트에 전달
	MultiplayerOnCreateSessionComplete.Broadcast(bWasSuccessful);
}

void UMultiplayerSessionsSubsystem::OnFindSessionsComplete(bool bWasSuccessful)
{
	if (SessionInterface.IsValid())
	{
		// 세션 찾기에 성공했으므로 델리게이트 제거
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);
	}

	if (LastSessionSearch.IsValid())
	{
		// 세션 찾기에 성공했어도 개수가 0개면 실패
		if (LastSessionSearch->SearchResults.Num() == 0)
		{
			MultiplayerOnFindSessionsComplete.Broadcast(TArray<FOnlineSessionSearchResult>(), false);
		}
		else
		{
			// 세션 찾기의 결과를 외부 델리게이트에 전달
			MultiplayerOnFindSessionsComplete.Broadcast(LastSessionSearch->SearchResults, bWasSuccessful);
		}
	}
}

void UMultiplayerSessionsSubsystem::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	if (SessionInterface.IsValid())
	{
		// 세션 가입에 성공하면 델리게이트 제거
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);
	}

	// 세션 가입의 결과를 외부 델리게이트에 전달
	MultiplayerOnJoinSessionComplete.Broadcast(Result);
}

void UMultiplayerSessionsSubsystem::OnDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (SessionInterface.IsValid())
	{
		// 세션 파괴가 끝났으므로 델리게이트 제거
		SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);
	}

	// 세션 생성할 때 중복 세션을 제거했다면 다시 세션 생성 수행
	if (bWasSuccessful && bCreateSessionOnDestroy)
	{
		bCreateSessionOnDestroy = false;
		CreateSession(LastNumPublicConnections, LastMatchType);
	}

	// 세션 파괴의 결과를 외부 델리게이트에 전달
	MultiplayerOnDestroySessionComplete.Broadcast(bWasSuccessful);
}

void UMultiplayerSessionsSubsystem::OnStartSessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (SessionInterface.IsValid())
	{
		// 세션 시작이 끝났으므로 델리게이트 제거
		SessionInterface->ClearOnStartSessionCompleteDelegate_Handle(StartSessionCompleteDelegateHandle);
	}

	// 세션 시작의 결과를 외부 델리게이트에 전달
	MultiplayerOnStartSessionComplete.Broadcast(bWasSuccessful);
}
