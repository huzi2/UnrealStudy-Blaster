// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ElimAnnouncement.generated.h"

class UHorizontalBox;
class UTextBlock;

/**
 * 플레이어 제거 메시지 UI 클래스
 */
UCLASS()
class BLASTER_API UElimAnnouncement : public UUserWidget
{
	GENERATED_BODY()
	
public:
	FORCEINLINE UHorizontalBox* GetAnnouncementBox() const { return AnnouncementBox; }

	// 공격자와 피격자에 대한 제거 메시지 띄우기
	void SetElimAnnouncementText(const FString& AttackerName, const FString& VictimName);

private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UHorizontalBox> AnnouncementBox;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> AnnouncementText;
};
