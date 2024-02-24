// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HUD/BlasterHUD.h"
#include "Weapon/WeaponTypes.h"
#include "BlasterTypes/CombatState.h"
#include "CombatComponent.generated.h"

constexpr double TRACE_LENGTH = 80000.0;

class AWeapon;
class ABlasterPlayerController;
class ABlasterHUD;

// 캐릭터의 무기 관리 컴포넌트
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class BLASTER_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()

private:
	UCombatComponent();
	friend class ABlasterCharacter;

private:
	virtual void BeginPlay() final;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) final;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const final;

private:
	UFUNCTION(Server, Reliable)
	void ServerSetAiming(bool bIsAiming);

	UFUNCTION(Server, Reliable)
	void ServerFire(const FVector_NetQuantize& TraceHitTarget);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastFire(const FVector_NetQuantize& TraceHitTarget);

	UFUNCTION(Server, Reliable)
	void ServerReload();

	UFUNCTION(BlueprintCallable)
	void FinishReloading();

public:
	void EquipWeapon(AWeapon* WeaponToEquip);
	void SetAiming(bool bIsAiming);
	void Reload();

private:
	void FireButtonPressed(bool bPressed);
	void TraceUnderCrosshairs(FHitResult& TraceHitResult);
	void SetHUDCrosshairs(float DeltaTime);
	void InterpFOV(float DeltaTime);
	void Fire();
	void StartFireTimer();
	void FireTimerFinished();
	bool CanFire() const;
	void InitializeCarriedAmmo();
	void CheckInit();
	void HandleReload();
	int32 AmountToReload() const;
	void UpdateAmmoValues();

private:
	UPROPERTY(ReplicatedUsing = OnRep_EquippedWeapon)
	TObjectPtr<AWeapon> EquippedWeapon;

	UFUNCTION()
	void OnRep_EquippedWeapon();

	UPROPERTY(Replicated)
	bool bAiming;

	UPROPERTY(EditAnywhere, Category = "Movement")
	float BaseWalkSpeed;

	UPROPERTY(EditAnywhere, Category = "Movement")
	float AimWalkSpeed;

	UPROPERTY(EditAnywhere, Category = "Combat")
	float ZoomedFOV;

	UPROPERTY(EditAnywhere, Category = "Combat")
	float ZoomInterpSpeed;

	UPROPERTY(EditAnywhere, Category = "Combat")
	int32 StartingARAmmo;

	// TMap은 레플리케이션이 안되서 레플리케이션하기 위한 변수임
	UPROPERTY(ReplicatedUsing = OnRep_CarriedAmmo)
	int32 CarriedAmmo;

	UFUNCTION()
	void OnRep_CarriedAmmo();

	UPROPERTY(ReplicatedUsing = OnRep_CombatState)
	ECombatState CombatState;

	UFUNCTION()
	void OnRep_CombatState();

	UPROPERTY()
	TObjectPtr<ABlasterCharacter> Character;

	UPROPERTY()
	TObjectPtr<ABlasterPlayerController> Controller;

	UPROPERTY()
	TObjectPtr<ABlasterHUD> HUD;

private:
	bool bFireButtonPressed;

	double CrosshairVelocityFactor;
	double CrosshairInAirFactor;
	double CrosshairAimFactor;
	double CrosshairShootingFactor;

	FVector HitTarget;

	float DefaultFOV;
	float CurrentFOV;

	FHUDPackage HUDPackage;

	bool bCanFire;
	FTimerHandle FireTimer;

	// 플레이어가 가지고 있는 무기 종류별 탄약
	// 해당 변수를 레플리케이션하지 않고 CarriedAmmo로 따로 가지고 있는 이유는 TMap은 해쉬맵으로 레플리케이션이 안된다.
	TMap<EWeaponType, int32> CarriedAmmoMap;
};
