// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "LobbyGameMode.generated.h"

/**
 * 게임 모드에 따라 맵을 연결하는 로비 게임 모드 클래스
 */
UCLASS()
class BLASTER_API ALobbyGameMode : public AGameMode
{
	GENERATED_BODY()
	
private:
	virtual void PostLogin(APlayerController* NewPlayer) final;
};
