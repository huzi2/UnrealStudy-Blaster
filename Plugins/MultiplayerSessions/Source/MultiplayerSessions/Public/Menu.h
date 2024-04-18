// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Menu.generated.h"

class UButton;
class UMultiplayerSessionsSubsystem;

/**
 * ��Ƽ�÷��̾� ������ ����, ã��, ���� �� ������ ����ϴ� ����� �������̽� Ŭ����
 */
UCLASS()
class MULTIPLAYERSESSIONS_API UMenu : public UUserWidget
{
	GENERATED_BODY()

private:
	virtual bool Initialize() final;
	virtual void NativeDestruct() final;

private:
	// UI ��ư ���� �Լ�
	UFUNCTION()
	void HostButtonClicked();
	UFUNCTION()
	void JoinButtonClicked();

	// UI ���� �̺�Ʈ ���ε�
	void BindWidgetEvents();

	// ���� ���� �� Ž��(��ư���� ����)
	void CreateSession();
	void FindSessions();

	// UI ���� �� �ʱ�ȭ
	UFUNCTION(BlueprintCallable)
	void MenuSetup(int32 NumberOfPublicConnections = 4, const FString& TypeOfMatch = TEXT("FreeForAll"), const FString& LobbyPath = TEXT("/Game/ThirdPerson/Maps/Lobby"));
	void UpdateSessionConfig(int32 NumberOfPublicConnections, const FString& TypeOfMatch, const FString& LobbyPath);
	void PrepareUI();
	void SetupInputMode();
	void BindSessionEvents();

	// UI ����
	void MenuTearDown();
	void ResetInputMode();

	// ���� �̺�Ʈ �ڵ鷯
	UFUNCTION()
	void OnCreateSession(bool bWasSuccessful);
	void OnFindSessions(const TArray<FOnlineSessionSearchResult>& SessionResults, bool bWasSuccessful);
	void OnJoinSession(EOnJoinSessionCompleteResult::Type Result);
	UFUNCTION()
	void OnDestroySession(bool bWasSuccessful);
	UFUNCTION()
	void OnStartSession(bool bWasSuccessful);

protected:
	// ���� ���� ����
	UPROPERTY(BlueprintReadWrite)
	FString MatchType = TEXT("FreeForAll");
	UPROPERTY(BlueprintReadWrite)
	int32 NumPublicConnections = 4;

private:
	// UI ���
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> HostButton;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> JoinButton;

	// ��Ƽ�÷��̾� ���� ����ý��� ����
	UPROPERTY()
	TObjectPtr<UMultiplayerSessionsSubsystem> MultiplayerSessionsSubsystem;
	
	// �κ� ��� ���ڿ�
	FString PathToLobby = FString();
};
