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
	// �Ʒ� �Լ����� ����-Ŭ�� �ð��� ����ȭ�ϱ� ����
	// �������� ���� ���� �ð��� ��û
	UFUNCTION(Server, Reliable)
	void ServerRequestServerTime(float TimeOfClientRequest);

	// Ŭ�󿡰� ���� ���� �ð��� �˷���. Ŭ��� �ش� �������� ����-Ŭ�� �ð� ���̸� Ȯ��
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

	// Ŭ���̾�Ʈ�� ������ �ð� ����
	float ClientServerDelta;

	float TimeSyncRunningTime;
};
