// Fill out your copyright notice in the Description page of Project Settings.

#include "HUD/BlasterHUD.h"
#include "GameFramework/PlayerController.h"
#include "HUD/CharacterOverlay.h"
#include "HUD/Announcement.h"
#include "HUD/ElimAnnouncement.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/HorizontalBox.h"
#include "Components/CanvasPanelSlot.h"

void ABlasterHUD::DrawHUD()
{
	Super::DrawHUD();
	
	if (GEngine && GEngine->GameViewport)
	{
		FVector2D ViewportSize;
		GEngine->GameViewport->GetViewportSize(ViewportSize);

		const FVector2D ViewportCenter(ViewportSize.X / 2.0, ViewportSize.Y / 2.0);
		const double SpreadScaled = CrosshairSpreadMax * HUDPackage.CrosshairSpread;

		DrawCrosshair(HUDPackage.CrosshairsCenter, ViewportCenter, {0.f, 0.f}, HUDPackage.CrosshairsColor);
		DrawCrosshair(HUDPackage.CrosshairsLeft, ViewportCenter, { -SpreadScaled, 0.f }, HUDPackage.CrosshairsColor);
		DrawCrosshair(HUDPackage.CrosshairsRight, ViewportCenter, { SpreadScaled, 0.f }, HUDPackage.CrosshairsColor);
		DrawCrosshair(HUDPackage.CrosshairsTop, ViewportCenter, { 0.f, -SpreadScaled }, HUDPackage.CrosshairsColor);
		DrawCrosshair(HUDPackage.CrosshairsBottom, ViewportCenter, { 0.f, SpreadScaled }, HUDPackage.CrosshairsColor);
	}
}

void ABlasterHUD::AddCharacterOverlay()
{
	if (!CharacterOverlayClass) return;

	PollInit();
	if (!OwningPlayer) return;

	if (CharacterOverlay = CreateWidget<UCharacterOverlay>(OwningPlayer, CharacterOverlayClass))
	{
		CharacterOverlay->AddToViewport();
	}
}

void ABlasterHUD::AddAnnouncement()
{
	if (!AnnouncementClass) return;

	PollInit();
	if (!OwningPlayer) return;

	if (Announcement = CreateWidget<UAnnouncement>(OwningPlayer, AnnouncementClass))
	{
		Announcement->AddToViewport();
	}
}

void ABlasterHUD::AddElimAnnouncement(const FString& AttackerName, const FString& VictimName)
{
	if (!ElimAnnouncementClass) return;

	PollInit();
	if (!OwningPlayer) return;

	if (UElimAnnouncement* ElimAnnouncement = CreateWidget<UElimAnnouncement>(OwningPlayer, ElimAnnouncementClass))
	{
		ElimAnnouncement->SetElimAnnouncementText(AttackerName, VictimName);
		ElimAnnouncement->AddToViewport();

		// 새 메시지를 위해 이전 메시지들을 위로 올린다.
		for (UElimAnnouncement* Msg : ElimMessages)
		{
			if (Msg && Msg->GetAnnouncementBox())
			{
				if (UCanvasPanelSlot* CanvasSlot = UWidgetLayoutLibrary::SlotAsCanvasSlot(Msg->GetAnnouncementBox()))
				{
					const FVector2D Position = CanvasSlot->GetPosition();
					const FVector2D NewPosition(Position.X, Position.Y - CanvasSlot->GetSize().Y);
					CanvasSlot->SetPosition(NewPosition);
				}
			}
		}

		ElimMessages.Add(ElimAnnouncement);

		// 처치 메시지가 일정 시간 뒤에 삭제되도록 타이머 설정
		FTimerDelegate ElimMsgDelegate;
		ElimMsgDelegate.BindUFunction(this, TEXT("ElimAnnouncementTimerFinished"), ElimAnnouncement);

		FTimerHandle ElimMsgTimer;
		GetWorldTimerManager().SetTimer(ElimMsgTimer, ElimMsgDelegate, ElimAnnouncementTime, false);
	}
}

void ABlasterHUD::PollInit()
{
	if (!OwningPlayer)
	{
		OwningPlayer = GetOwningPlayerController();
	}
}

void ABlasterHUD::DrawCrosshair(UTexture2D* Texture, const FVector2D& ViewportCenter, const FVector2D& Spread, const FLinearColor& CrosshairColor)
{
	if (!Texture) return;

	const float TextureWidth = static_cast<float>(Texture->GetSizeX());
	const float TextrueHeight = static_cast<float>(Texture->GetSizeY());
	const FVector2D TextureDrawPoint(ViewportCenter.X - (TextureWidth / 2.0) + Spread.X, ViewportCenter.Y - (TextrueHeight / 2.0) + Spread.Y);

	DrawTexture(Texture, TextureDrawPoint.X, TextureDrawPoint.Y, TextureWidth, TextrueHeight, 0.f, 0.f, 1.f, 1.f, CrosshairColor);
}

void ABlasterHUD::ElimAnnouncementTimerFinished(UElimAnnouncement* MsgToRemove)
{
	if (!MsgToRemove) return;

	MsgToRemove->RemoveFromParent();
}
