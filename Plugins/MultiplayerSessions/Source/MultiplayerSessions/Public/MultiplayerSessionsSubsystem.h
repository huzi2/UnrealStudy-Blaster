// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "MultiplayerSessionsSubsystem.generated.h"

// ���� ���� ��������Ʈ ����
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMultiplayerOnCreateSessionComplete, bool, bWasSuccessful);
DECLARE_MULTICAST_DELEGATE_TwoParams(FMultiplayerOnFindSessionsComplete, const TArray<FOnlineSessionSearchResult>& SessionResults, bool bWasSuccessful);
DECLARE_MULTICAST_DELEGATE_OneParam(FMultiplayerOnJoinSessionComplete, EOnJoinSessionCompleteResult::Type Result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMultiplayerOnDestroySessionComplete, bool, bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMultiplayerOnStartSessionComplete, bool, bWasSuccessful);

/**
 * ��Ƽ�÷��̾� ������ ����, ���� �� �ı��� ����
 */
UCLASS()
class MULTIPLAYERSESSIONS_API UMultiplayerSessionsSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
private:
	UMultiplayerSessionsSubsystem();

public:
	FORCEINLINE int32 GetDesiredNumPublicConnections() const { return DesiredNumPublicConnections; }
	FORCEINLINE const FString& GetDesiredMatchType() const { return DesiredMatchType; }

	// ���� ���� �Լ�
	void CreateSession(int32 NumPublicConnections, const FString& MatchType);
	void FindSessions(int32 MaxSearchResults);
	void JoinSession(const FOnlineSessionSearchResult& SessionResult);
	void DestroySession();
	void StratSession();

private:
	// CreateSession���� ���
	void ConfigureSessionSettings(int32 NumPublicConnections, const FString& MatchType);
	void StartSessionCreation();

	// ���� �̺�Ʈ �ڵ鷯
	void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	void OnFindSessionsComplete(bool bWasSuccessful);
	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void OnDestroySessionComplete(FName SessionName, bool bWasSuccessful);
	void OnStartSessionComplete(FName SessionName, bool bWasSuccessful);

public:
	// �ܺ� Ŭ�������� ������ �������� �� ����� ��������Ʈ��
	FMultiplayerOnCreateSessionComplete MultiplayerOnCreateSessionComplete;
	FMultiplayerOnFindSessionsComplete MultiplayerOnFindSessionsComplete;
	FMultiplayerOnJoinSessionComplete MultiplayerOnJoinSessionComplete;
	FMultiplayerOnDestroySessionComplete MultiplayerOnDestroySessionComplete;
	FMultiplayerOnStartSessionComplete MultiplayerOnStartSessionComplete;

private:
	IOnlineSessionPtr SessionInterface;
	TSharedPtr<FOnlineSessionSettings> LastSessionSettings;
	TSharedPtr<FOnlineSessionSearch> LastSessionSearch;

	// ���� �̺�Ʈ ��������Ʈ
	FOnCreateSessionCompleteDelegate CreateSessionCompleteDelegate;
	FOnFindSessionsCompleteDelegate FindSessionsCompleteDelegate;
	FOnJoinSessionCompleteDelegate JoinSessionCompleteDelegate;
	FOnDestroySessionCompleteDelegate DestroySessionCompleteDelegate;
	FOnStartSessionCompleteDelegate StartSessionCompleteDelegate;

	// ���� �̺�Ʈ ��������Ʈ �ڵ�
	FDelegateHandle CreateSessionCompleteDelegateHandle;
	FDelegateHandle FindSessionsCompleteDelegateHandle;
	FDelegateHandle JoinSessionCompleteDelegateHandle;
	FDelegateHandle DestroySessionCompleteDelegateHandle;
	FDelegateHandle StartSessionCompleteDelegateHandle;

	// ���� �ı� �� ���� �ڵ� ���� ����
	bool bCreateSessionOnDestroy = false;
	// ������ ������ ���� ���� ��
	int32 LastNumPublicConnections;
	// ������ ������ ��ġ Ÿ��
	FString LastMatchType;
	// ���ϴ� ���� ���� ��
	int32 DesiredNumPublicConnections = 0;
	// ���ϴ� ��ġ Ÿ��
	FString DesiredMatchType = FString();
};
