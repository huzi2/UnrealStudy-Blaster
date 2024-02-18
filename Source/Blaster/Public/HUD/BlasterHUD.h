// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "BlasterHUD.generated.h"

class UCharacterOverlay;

USTRUCT(BlueprintType)
struct FHUDPackage
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TObjectPtr<UTexture2D> CrosshairsCenter;

	UPROPERTY()
	TObjectPtr<UTexture2D> CrosshairsLeft;

	UPROPERTY()
	TObjectPtr<UTexture2D> CrosshairsRight;

	UPROPERTY()
	TObjectPtr<UTexture2D> CrosshairsTop;

	UPROPERTY()
	TObjectPtr<UTexture2D> CrosshairsBottom;

	double CrosshairSpread;
	FLinearColor CrosshairsColor;
};

/**
 * 
 */
UCLASS()
class BLASTER_API ABlasterHUD : public AHUD
{
	GENERATED_BODY()
	
private:
	ABlasterHUD();

private:
	virtual void BeginPlay() final;
	virtual void DrawHUD() final;

public:
	FORCEINLINE UCharacterOverlay* GetCharacterOverlay() const { return CharacterOverlay; }
	FORCEINLINE void SetHUDPackage(const FHUDPackage& Package) { HUDPackage = Package; }

private:
	void AddCharacterOverlay();
	void DrawCrosshair(UTexture2D* Texture, const FVector2D& ViewportCenter, const FVector2D& Spread, const FLinearColor& CrosshairColor);

private:
	UPROPERTY(EditAnywhere)
	double CrosshairSpreadMax;

	UPROPERTY(EditAnywhere, Category = "Player Stats")
	TSubclassOf<UUserWidget> CharacterOverlayClass;

	UPROPERTY()
	TObjectPtr<UCharacterOverlay> CharacterOverlay;

private:
	FHUDPackage HUDPackage;
};
