// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "OverheadWidget.generated.h"

class UTextBlock;

/**
 * �÷��̾��� NetRole�� ǥ�����ִ� UI Ŭ����
 */
UCLASS()
class BLASTER_API UOverheadWidget : public UUserWidget
{
	GENERATED_BODY()
	
private:
	virtual void NativeDestruct() final;

private:
	UFUNCTION(BlueprintCallable)
	void ShowPlayerNetRole(APawn* InPawn);

	void SetDisplayText(const FString& TextToDisplay);

private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> DisplayText;
};
