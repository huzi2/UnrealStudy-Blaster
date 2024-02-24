// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "BlasterPlayerController.generated.h"

class ABlasterHUD;
/**
 * 
 */
UCLASS()
class BLASTER_API ABlasterPlayerController : public APlayerController
{
	GENERATED_BODY()

private:
	ABlasterPlayerController();

private:
	virtual void BeginPlay() final;
	virtual void OnPossess(APawn* InPawn) final;
	virtual void Tick(float DeltaTime) final;
	virtual void ReceivedPlayer() final;

private:
	// 아래 함수들은 서버-클라 시간을 동기화하기 위함
	// 서버에게 현재 서버 시간을 요청
	UFUNCTION(Server, Reliable)
	void ServerRequestServerTime(float TimeOfClientRequest);

	// 클라에게 현재 서버 시간을 알려줌. 클라는 해당 내용으로 서버-클라 시간 차이를 확인
	UFUNCTION(Client, Reliable)
	void ClientReportServerTime(float TimeOfClientRequest, float TimeServerReceivedClientRequest);

public:
	void SetHUDHealth(float Health, float MaxHealth);
	void SetHUDScore(float Score);
	void SetHUDDefeats(int32 Defeats);
	void SetHUDWeaponAmmo(int32 Ammo);
	void SetHUDCarriedAmmo(int32 Ammo);
	void SetHUDMatchCountdown(float CounddownTime);
	float GetServerTime() const;

private:
	void SetHUDTime();
	void CheckTimeSync(float DeltaTime);

private:
	UPROPERTY(EditAnywhere, Category = "Time")
	float TimeSyncFrequency;

private:
	TObjectPtr<ABlasterHUD> BlasterHUD;
	float MatchTime;
	uint32 CounddownInt;

	// 클라이언트와 서버의 시간 차이
	float ClientServerDelta;

	float TimeSyncRunningTime;
};
