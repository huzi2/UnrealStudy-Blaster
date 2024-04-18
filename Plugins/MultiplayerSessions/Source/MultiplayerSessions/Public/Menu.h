// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Menu.generated.h"

class UButton;
class UMultiplayerSessionsSubsystem;

/**
 * 멀티플레이어 세션의 생성, 찾기, 가입 및 관리를 담당하는 사용자 인터페이스 클래스
 */
UCLASS()
class MULTIPLAYERSESSIONS_API UMenu : public UUserWidget
{
	GENERATED_BODY()

private:
	virtual bool Initialize() final;
	virtual void NativeDestruct() final;

private:
	// UI 버튼 연결 함수
	UFUNCTION()
	void HostButtonClicked();
	UFUNCTION()
	void JoinButtonClicked();

	// UI 위젯 이벤트 바인딩
	void BindWidgetEvents();

	// 세션 생성 및 탐색(버튼에서 수행)
	void CreateSession();
	void FindSessions();

	// UI 설정 및 초기화
	UFUNCTION(BlueprintCallable)
	void MenuSetup(int32 NumberOfPublicConnections = 4, const FString& TypeOfMatch = TEXT("FreeForAll"), const FString& LobbyPath = TEXT("/Game/ThirdPerson/Maps/Lobby"));
	void UpdateSessionConfig(int32 NumberOfPublicConnections, const FString& TypeOfMatch, const FString& LobbyPath);
	void PrepareUI();
	void SetupInputMode();
	void BindSessionEvents();

	// UI 해제
	void MenuTearDown();
	void ResetInputMode();

	// 세션 이벤트 핸들러
	UFUNCTION()
	void OnCreateSession(bool bWasSuccessful);
	void OnFindSessions(const TArray<FOnlineSessionSearchResult>& SessionResults, bool bWasSuccessful);
	void OnJoinSession(EOnJoinSessionCompleteResult::Type Result);
	UFUNCTION()
	void OnDestroySession(bool bWasSuccessful);
	UFUNCTION()
	void OnStartSession(bool bWasSuccessful);

protected:
	// 세션 설정 변수
	UPROPERTY(BlueprintReadWrite)
	FString MatchType = TEXT("FreeForAll");
	UPROPERTY(BlueprintReadWrite)
	int32 NumPublicConnections = 4;

private:
	// UI 요소
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> HostButton;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> JoinButton;

	// 멀티플레이어 세션 서브시스템 참조
	UPROPERTY()
	TObjectPtr<UMultiplayerSessionsSubsystem> MultiplayerSessionsSubsystem;
	
	// 로비 경로 문자열
	FString PathToLobby = FString();
};
