// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ReturnToMainMenu.generated.h"

class UButton;
class UMultiplayerSessionsSubsystem;

/**
 * 메인 메뉴로 돌아가는 UI 클래스
 */
UCLASS()
class BLASTER_API UReturnToMainMenu : public UUserWidget
{
	GENERATED_BODY()

public:
	// UI를 클릭할 수 있도록 수정하고 메뉴를 띄움
	void MenuSetup();
	// 위 상태에서 원래 게임 상태로 돌아옴
	void MenuTearDown();

private:
	UFUNCTION()
	void ReturnButtonClicked();
	UFUNCTION()
	void OnDestroySession(bool bWasSuccessful);
	UFUNCTION()
	void OnPlayerLeftGame();

private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ReturnButton;

	// 참조 변수
	UPROPERTY()
	TObjectPtr<UMultiplayerSessionsSubsystem> MultiplayerSessionsSubsystem;
	UPROPERTY()
	TObjectPtr<APlayerController> PlayerController;
};
