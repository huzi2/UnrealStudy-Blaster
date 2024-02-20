// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "BlasterPlayerState.generated.h"

class ABlasterCharacter;
class ABlasterPlayerController;
/**
 * 
 */
UCLASS()
class BLASTER_API ABlasterPlayerState : public APlayerState
{
	GENERATED_BODY()
	
private:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const final;
	virtual void OnRep_Score() final;

public:
	void AddToScore(float ScoreAmount);
	void AddToDefeats(int32 DefeatsAmount);

private:
	void CheckInit();

private:
	UPROPERTY(ReplicatedUsing = OnRep_Defeats)
	int32 Defeats;

	UFUNCTION()
	void OnRep_Defeats();

	UPROPERTY()
	TObjectPtr<ABlasterCharacter> Character;

	UPROPERTY()
	TObjectPtr<ABlasterPlayerController> Controller;
};
