// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "BlasterGameMode.generated.h"

class ABlasterCharacter;
class ABlasterPlayerController;
/**
 * 
 */
UCLASS()
class BLASTER_API ABlasterGameMode : public AGameMode
{
	GENERATED_BODY()
	
private:
	ABlasterGameMode();

private:
	virtual void BeginPlay() final;
	virtual void Tick(float DeltaTime) final;

public:
	void PlayerEliminated(ABlasterCharacter* ElimmedCharacter, ABlasterPlayerController* VictimController, ABlasterPlayerController* AttackerController);
	void RequestRespawn(ACharacter* ElimmedCharacter, AController* ElimmedController);

private:
	UPROPERTY(EditDefaultsOnly)
	float WarmupTime;

private:
	float CountdownTime;
	float LevelStartingTime;
};
