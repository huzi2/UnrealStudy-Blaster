// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Announcement.generated.h"

class UTextBlock;
/**
 * 
 */
UCLASS()
class BLASTER_API UAnnouncement : public UUserWidget
{
	GENERATED_BODY()

public:
	FORCEINLINE UTextBlock* GetWarmupTime() const { return WarmupTime; }
	FORCEINLINE UTextBlock* GetAnnouncementText() const { return AnnouncementText; }
	FORCEINLINE UTextBlock* GetInfoText() const { return InfoText; }

private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> WarmupTime;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> AnnouncementText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> InfoText;
};
