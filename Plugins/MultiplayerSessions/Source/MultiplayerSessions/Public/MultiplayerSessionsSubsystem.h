// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "MultiplayerSessionsSubsystem.generated.h"

// 세션 관련 델리게이트 선언
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMultiplayerOnCreateSessionComplete, bool, bWasSuccessful);
DECLARE_MULTICAST_DELEGATE_TwoParams(FMultiplayerOnFindSessionsComplete, const TArray<FOnlineSessionSearchResult>& SessionResults, bool bWasSuccessful);
DECLARE_MULTICAST_DELEGATE_OneParam(FMultiplayerOnJoinSessionComplete, EOnJoinSessionCompleteResult::Type Result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMultiplayerOnDestroySessionComplete, bool, bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMultiplayerOnStartSessionComplete, bool, bWasSuccessful);

/**
 * 멀티플레이어 세션의 생성, 관리 및 파괴를 수행
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

	// 세션 관리 함수
	void CreateSession(int32 NumPublicConnections, const FString& MatchType);
	void FindSessions(int32 MaxSearchResults);
	void JoinSession(const FOnlineSessionSearchResult& SessionResult);
	void DestroySession();
	void StratSession();

private:
	// CreateSession에서 사용
	void ConfigureSessionSettings(int32 NumPublicConnections, const FString& MatchType);
	void StartSessionCreation();

	// 세션 이벤트 핸들러
	void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	void OnFindSessionsComplete(bool bWasSuccessful);
	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void OnDestroySessionComplete(FName SessionName, bool bWasSuccessful);
	void OnStartSessionComplete(FName SessionName, bool bWasSuccessful);

public:
	// 외부 클래스에서 세션을 수행했을 때 사용할 델리게이트들
	FMultiplayerOnCreateSessionComplete MultiplayerOnCreateSessionComplete;
	FMultiplayerOnFindSessionsComplete MultiplayerOnFindSessionsComplete;
	FMultiplayerOnJoinSessionComplete MultiplayerOnJoinSessionComplete;
	FMultiplayerOnDestroySessionComplete MultiplayerOnDestroySessionComplete;
	FMultiplayerOnStartSessionComplete MultiplayerOnStartSessionComplete;

private:
	IOnlineSessionPtr SessionInterface;
	TSharedPtr<FOnlineSessionSettings> LastSessionSettings;
	TSharedPtr<FOnlineSessionSearch> LastSessionSearch;

	// 세션 이벤트 델리게이트
	FOnCreateSessionCompleteDelegate CreateSessionCompleteDelegate;
	FOnFindSessionsCompleteDelegate FindSessionsCompleteDelegate;
	FOnJoinSessionCompleteDelegate JoinSessionCompleteDelegate;
	FOnDestroySessionCompleteDelegate DestroySessionCompleteDelegate;
	FOnStartSessionCompleteDelegate StartSessionCompleteDelegate;

	// 세션 이벤트 델리게이트 핸들
	FDelegateHandle CreateSessionCompleteDelegateHandle;
	FDelegateHandle FindSessionsCompleteDelegateHandle;
	FDelegateHandle JoinSessionCompleteDelegateHandle;
	FDelegateHandle DestroySessionCompleteDelegateHandle;
	FDelegateHandle StartSessionCompleteDelegateHandle;

	// 세션 파괴 후 세션 자동 생성 여부
	bool bCreateSessionOnDestroy = false;
	// 마지막 세션의 공개 연결 수
	int32 LastNumPublicConnections;
	// 마지막 세션의 매치 타입
	FString LastMatchType;
	// 원하는 공개 연결 수
	int32 DesiredNumPublicConnections = 0;
	// 원하는 매치 타입
	FString DesiredMatchType = FString();
};
