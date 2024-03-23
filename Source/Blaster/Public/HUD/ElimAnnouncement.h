// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ElimAnnouncement.generated.h"

class UHorizontalBox;
class UTextBlock;
/**
 * 
 */
UCLASS()
class BLASTER_API UElimAnnouncement : public UUserWidget
{
	GENERATED_BODY()
	
public:
	FORCEINLINE UHorizontalBox* GetAnnouncementBox() const { return AnnouncementBox; }
	void SetElimAnnouncementText(const FString& AttackerName, const FString& VictimName);

private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UHorizontalBox> AnnouncementBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> AnnouncementText;
};
