// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameMode/BlasterGameMode.h"
#include "TeamGameMode.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API ATeamGameMode : public ABlasterGameMode
{
	GENERATED_BODY()

private:
	virtual void HandleMatchHasStarted() final;
	virtual void PostLogin(APlayerController* NewPlayer) final;
	virtual void Logout(AController* Exiting) final;
};
