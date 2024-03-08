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
	FORCEINLINE UProgressBar* GetShieldBar() const { return ShieldBar; }
	FORCEINLINE UTextBlock* GetShieldText() const { return ShieldText; }
	FORCEINLINE UTextBlock* GetScoreAmount() const { return ScoreAmount; }
	FORCEINLINE UTextBlock* GetDefeatsAmount() const { return DefeatsAmount; }
	FORCEINLINE UTextBlock* GetWeaponAmmoAmount() const { return WeaponAmmoAmount; }
	FORCEINLINE UTextBlock* GetCarriedAmmoAmount() const { return CarriedAmmoAmount; }
	FORCEINLINE UTextBlock* GetMatchCountdownText() const { return MatchCountdownText; }
	FORCEINLINE UTextBlock* GetGrenadeText() const { return GrenadeText; }

private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> HealthBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> HealthText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> ShieldBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ShieldText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ScoreAmount;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> DefeatsAmount;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> WeaponAmmoAmount;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> CarriedAmmoAmount;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> MatchCountdownText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> GrenadeText;
};
