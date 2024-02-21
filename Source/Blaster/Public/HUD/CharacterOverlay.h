// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CharacterOverlay.generated.h"

class UProgressBar;
class UTextBlock;
/**
 * 
 */
UCLASS()
class BLASTER_API UCharacterOverlay : public UUserWidget
{
	GENERATED_BODY()

public:
	FORCEINLINE UProgressBar* GetHealthBar() const { return HealthBar; }
	FORCEINLINE UTextBlock* GetHealthText() const { return HealthText; }
	FORCEINLINE UTextBlock* GetScoreAmount() const { return ScoreAmount; }
	FORCEINLINE UTextBlock* GetDefeatsAmount() const { return DefeatsAmount; }
	FORCEINLINE UTextBlock* GetWeaponAmmoAmount() const { return WeaponAmmoAmount; }
	FORCEINLINE UTextBlock* GetCarriedAmmoAmount() const { return CarriedAmmoAmount; }

private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> HealthBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> HealthText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ScoreAmount;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> DefeatsAmount;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> WeaponAmmoAmount;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> CarriedAmmoAmount;
};
