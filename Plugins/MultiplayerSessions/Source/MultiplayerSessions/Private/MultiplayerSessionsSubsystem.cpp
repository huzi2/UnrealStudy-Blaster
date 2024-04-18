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
	// ���� ���ӿ��� ����� �ο� ���� ���� ��� ����
	DesiredNumPublicConnections = NumPublicConnections;
	DesiredMatchType = MatchType;

	if (!SessionInterface.IsValid() || !GetWorld())
	{
		MultiplayerOnCreateSessionComplete.Broadcast(false);
		return;
	}

	// ������ ������ ������ ���� ����
	if (FNamedOnlineSession* ExistingSession = SessionInterface->GetNamedSession(NAME_GameSession))
	{
		bCreateSessionOnDestroy = true;
		LastNumPublicConnections = NumPublicConnections;
		LastMatchType = MatchType;

		DestroySession();
	}

	// ���� ������ ���� ��������Ʈ ����
	CreateSessionCompleteDelegateHandle = SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegate);

	// ���� ����
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
	// ����Ī ��� ����
	LastSessionSettings->bIsLANMatch = IOnlineSubsystem::Get()->GetSubsystemName() == TEXT("NULL") ? true : false;
	// ���ǿ� ������ �� �ִ� �ο���
	LastSessionSettings->NumPublicConnections = NumPublicConnections;
	// ���� ���� ������ ������ �� �ִ°�?
	LastSessionSettings->bAllowJoinInProgress = true;
	// ���簨(ģ����� ��)�� ���� ���ǿ� ������ �� �ִ°�?
	LastSessionSettings->bAllowJoinViaPresence = true;
	// ������ ����Ǵ���?
	LastSessionSettings->bShouldAdvertise = true;
	// ���簨 ������ ����� ��?
	LastSessionSettings->bUsesPresence = true;
	// �κ� �ý����� ����� ���ΰ�?
	LastSessionSettings->bUseLobbiesIfAvailable = true;
	// ������ ���� �������� ȣȯ���� �˻��� ���ΰ�?
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

	// ���� ����
	if (!SessionInterface->CreateSession(*LocalPlayer->GetPreferredUniqueNetId(), NAME_GameSession, *LastSessionSettings))
	{
		// ���� ������ �������Ƿ� ��������Ʈ ����
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);

		// ���� ������ ���������Ƿ� �ܺ� ��������Ʈ�� false ����
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

	// ���� ã�⿡ ���� ��������Ʈ ����
	FindSessionsCompleteDelegateHandle = SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegate);

	// ã�� ������ ����
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

	// ���� ã��
	if (!SessionInterface->FindSessions(*LocalPlayer->GetPreferredUniqueNetId(), LastSessionSearch.ToSharedRef()))
	{
		// ���� ã�⿡ �����ϸ� ��������Ʈ ����
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);

		// ���� ã�⿡ ���������Ƿ� �ܺ� ��������Ʈ�� false ����
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

	// ���� ���Կ� ���� ��������Ʈ ����
	JoinSessionCompleteDelegateHandle = SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegate);

	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	if (!LocalPlayer)
	{
		MultiplayerOnJoinSessionComplete.Broadcast(EOnJoinSessionCompleteResult::UnknownError);
		return;
	}

	// ���� ����
	if (!SessionInterface->JoinSession(*LocalPlayer->GetPreferredUniqueNetId(), NAME_GameSession, SessionResult))
	{
		// ���� ���Կ� �����ϸ� ��������Ʈ ����
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);

		// ���� ���Կ� ���������Ƿ� �ܺ� ��������Ʈ�� ���� ����
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

	// ���� �ı��� ���� ��������Ʈ ����
	DestroySessionCompleteDelegateHandle = SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegate);

	// ���� �ı�
	if (!SessionInterface->DestroySession(NAME_GameSession))
	{
		// ���� �ı��� �����ϸ� ��������Ʈ ����
		SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);

		// ���� �ı��� ���������Ƿ� �ܺ� ��������Ʈ�� ���� ����
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

	// ���� ���ۿ� ���� ��������Ʈ ����
	StartSessionCompleteDelegateHandle = SessionInterface->AddOnStartSessionCompleteDelegate_Handle(StartSessionCompleteDelegate);

	// ���� ����
	if (!SessionInterface->StartSession(NAME_GameSession))
	{
		// ���� ���ۿ� �����ϸ� ��������Ʈ ����
		SessionInterface->ClearOnStartSessionCompleteDelegate_Handle(StartSessionCompleteDelegateHandle);

		// ���� ���ۿ� ���������Ƿ� �ܺ� ��������Ʈ�� ���� ����
		MultiplayerOnStartSessionComplete.Broadcast(false);
	}
}

void UMultiplayerSessionsSubsystem::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (SessionInterface.IsValid())
	{
		// ���� ������ �������Ƿ� ��������Ʈ ����
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);
	}

	// ���� ������ ����� �ܺ� ��������Ʈ�� ����
	MultiplayerOnCreateSessionComplete.Broadcast(bWasSuccessful);
}

void UMultiplayerSessionsSubsystem::OnFindSessionsComplete(bool bWasSuccessful)
{
	if (SessionInterface.IsValid())
	{
		// ���� ã�⿡ ���������Ƿ� ��������Ʈ ����
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);
	}

	if (LastSessionSearch.IsValid())
	{
		// ���� ã�⿡ �����߾ ������ 0���� ����
		if (LastSessionSearch->SearchResults.Num() == 0)
		{
			MultiplayerOnFindSessionsComplete.Broadcast(TArray<FOnlineSessionSearchResult>(), false);
		}
		else
		{
			// ���� ã���� ����� �ܺ� ��������Ʈ�� ����
			MultiplayerOnFindSessionsComplete.Broadcast(LastSessionSearch->SearchResults, bWasSuccessful);
		}
	}
}

void UMultiplayerSessionsSubsystem::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	if (SessionInterface.IsValid())
	{
		// ���� ���Կ� �����ϸ� ��������Ʈ ����
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);
	}

	// ���� ������ ����� �ܺ� ��������Ʈ�� ����
	MultiplayerOnJoinSessionComplete.Broadcast(Result);
}

void UMultiplayerSessionsSubsystem::OnDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (SessionInterface.IsValid())
	{
		// ���� �ı��� �������Ƿ� ��������Ʈ ����
		SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);
	}

	// ���� ������ �� �ߺ� ������ �����ߴٸ� �ٽ� ���� ���� ����
	if (bWasSuccessful && bCreateSessionOnDestroy)
	{
		bCreateSessionOnDestroy = false;
		CreateSession(LastNumPublicConnections, LastMatchType);
	}

	// ���� �ı��� ����� �ܺ� ��������Ʈ�� ����
	MultiplayerOnDestroySessionComplete.Broadcast(bWasSuccessful);
}

void UMultiplayerSessionsSubsystem::OnStartSessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (SessionInterface.IsValid())
	{
		// ���� ������ �������Ƿ� ��������Ʈ ����
		SessionInterface->ClearOnStartSessionCompleteDelegate_Handle(StartSessionCompleteDelegateHandle);
	}

	// ���� ������ ����� �ܺ� ��������Ʈ�� ����
	MultiplayerOnStartSessionComplete.Broadcast(bWasSuccessful);
}
