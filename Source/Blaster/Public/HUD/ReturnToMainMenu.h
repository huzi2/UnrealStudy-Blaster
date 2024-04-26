// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ReturnToMainMenu.generated.h"

class UButton;
class UMultiplayerSessionsSubsystem;

/**
 * ���� �޴��� ���ư��� UI Ŭ����
 */
UCLASS()
class BLASTER_API UReturnToMainMenu : public UUserWidget
{
	GENERATED_BODY()

public:
	// UI�� Ŭ���� �� �ֵ��� �����ϰ� �޴��� ���
	void MenuSetup();
	// �� ���¿��� ���� ���� ���·� ���ƿ�
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

	// ���� ����
	UPROPERTY()
	TObjectPtr<UMultiplayerSessionsSubsystem> MultiplayerSessionsSubsystem;
	UPROPERTY()
	TObjectPtr<APlayerController> PlayerController;
};
