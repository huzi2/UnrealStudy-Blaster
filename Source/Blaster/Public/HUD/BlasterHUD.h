// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "BlasterHUD.generated.h"

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
	virtual void DrawHUD() final;

public:
	FORCEINLINE void SetHUDPackage(const FHUDPackage& Package) { HUDPackage = Package; }

private:
	void DrawCrosshair(UTexture2D* Texture, const FVector2D& ViewportCenter, const FVector2D& Spread);

private:
	UPROPERTY(EditAnywhere)
	double CrosshairSpreadMax;

private:
	FHUDPackage HUDPackage;
};
