// Fill out your copyright notice in the Description page of Project Settings.

#include "HUD/ElimAnnouncement.h"
#include "Components/TextBlock.h"

void UElimAnnouncement::SetElimAnnouncementText(const FString& AttackerName, const FString& VictimName)
{
	if (!AnnouncementText) return;

	const FString ElimAnnouncementText = FString::Printf(TEXT("%s elimmed %s!"), *AttackerName, *VictimName);
	AnnouncementText->SetText(FText::FromString(ElimAnnouncementText));
}
