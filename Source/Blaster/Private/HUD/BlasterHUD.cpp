// Fill out your copyright notice in the Description page of Project Settings.

#include "HUD/BlasterHUD.h"
#include "GameFramework/PlayerController.h"
#include "HUD/CharacterOverlay.h"

ABlasterHUD::ABlasterHUD()
	: CrosshairSpreadMax(16.0)
{
}

void ABlasterHUD::BeginPlay()
{
	Super::BeginPlay();

	AddCharacterOverlay();
}

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

	APlayerController * PlayerController = GetOwningPlayerController();
	if (!PlayerController) return;

	CharacterOverlay = CreateWidget<UCharacterOverlay>(PlayerController, CharacterOverlayClass);
	CharacterOverlay->AddToViewport();
}

void ABlasterHUD::DrawCrosshair(UTexture2D* Texture, const FVector2D& ViewportCenter, const FVector2D& Spread, const FLinearColor& CrosshairColor)
{
	if (!Texture) return;

	const float TextureWidth = static_cast<float>(Texture->GetSizeX());
	const float TextrueHeight = static_cast<float>(Texture->GetSizeY());
	const FVector2D TextureDrawPoint(ViewportCenter.X - (TextureWidth / 2.0) + Spread.X, ViewportCenter.Y - (TextrueHeight / 2.0) + Spread.Y);

	DrawTexture(Texture, TextureDrawPoint.X, TextureDrawPoint.Y, TextureWidth, TextrueHeight, 0.f, 0.f, 1.f, 1.f, CrosshairColor);
}
