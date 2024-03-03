// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HUD/BlasterHUD.h"
#include "Weapon/WeaponTypes.h"
#include "BlasterTypes/CombatState.h"
#include "CombatComponent.generated.h"

class AWeapon;
class ABlasterPlayerController;
class ABlasterHUD;

// ĳ������ ���� ���� ������Ʈ
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

	UFUNCTION(BlueprintCallable)
	void ShotgunShellReload();

public:
	void EquipWeapon(AWeapon* WeaponToEquip);
	void SetAiming(bool bIsAiming);
	void Reload();
	void FireButtonPressed(bool bPressed);
	void JumpToShotgunEnd();

private:
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
	void UpdateShotgunAmmoValues();

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

	UPROPERTY(EditAnywhere, Category = "Combat")
	int32 StartingRocketAmmo;

	UPROPERTY(EditAnywhere, Category = "Combat")
	int32 StartingPistolAmmo;

	UPROPERTY(EditAnywhere, Category = "Combat")
	int32 StartingSMGAmmo;

	UPROPERTY(EditAnywhere, Category = "Combat")
	int32 StartingShotgunAmmo;

	UPROPERTY(EditAnywhere, Category = "Combat")
	int32 StartingSniperAmmo;

	UPROPERTY(EditAnywhere, Category = "Combat")
	int32 StartingGrenaderLauncherAmmo;

	// TMap�� ���ø����̼��� �ȵǼ� ���ø����̼��ϱ� ���� ������
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

	// �÷��̾ ������ �ִ� ���� ������ ź��
	// �ش� ������ ���ø����̼����� �ʰ� CarriedAmmo�� ���� ������ �ִ� ������ TMap�� �ؽ������� ���ø����̼��� �ȵȴ�.
	TMap<EWeaponType, int32> CarriedAmmoMap;
};
