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
class AProjectile;

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
	void ServerShotgunFire(const TArray<FVector_NetQuantize>& TraceHitTargets);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastShotgunFire(const TArray<FVector_NetQuantize>& TraceHitTargets);

	UFUNCTION(Server, Reliable)
	void ServerReload();

	UFUNCTION(Server, Reliable)
	void ServerThrowGrenade();

	UFUNCTION(Server, Reliable)
	void ServerLaunchGrenade(const FVector_NetQuantize& Target);

	UFUNCTION(BlueprintCallable)
	void FinishReloading();

	UFUNCTION(BlueprintCallable)
	void ShotgunShellReload();

	UFUNCTION(BlueprintCallable)
	void ThrowGrenadeFinished();

	UFUNCTION(BlueprintCallable)
	void LaunchGrenade();

public:
	FORCEINLINE int32 GetGrenades() const { return Grenades; }

	void EquipWeapon(AWeapon* WeaponToEquip);
	void SwapWeapons();
	void SetAiming(bool bIsAiming);
	void Reload();
	void FireButtonPressed(bool bPressed);
	void JumpToShotgunEnd();
	void ThrowGrenade();
	void PickupAmmo(EWeaponType WeaponType, int32 AmmoAmount);
	bool ShouldSwapWeapons() const;

private:
	void TraceUnderCrosshairs(FHitResult& TraceHitResult);
	void SetHUDCrosshairs(float DeltaTime);
	void InterpFOV(float DeltaTime);
	void Fire();
	void FireProjectileWeapon();
	void FireHitScanWeapon();
	void FireShotgun();
	void LocalFire(const FVector_NetQuantize& TraceHitTarget);
	void LocalShotgunFire(const TArray<FVector_NetQuantize>& TraceHitTargets);
	void StartFireTimer();
	void FireTimerFinished();
	bool CanFire() const;
	void InitializeCarriedAmmo();
	void CheckInit();
	void HandleReload();
	int32 AmountToReload() const;
	void UpdateAmmoValues();
	void UpdateShotgunAmmoValues();
	void DropEquippedWedapon();
	void AttachActorToRightHand(AActor* ActorToAttach);
	void AttachActorToLeftHand(AActor* ActorToAttach);
	void AttachActorToBackpack(AActor* ActorToAttach);
	void UpdateCarriedAmmo();
	void PlayEquipWeaponSound(AWeapon* WeaponToEquip);
	void ReloadEmptyWeapon();
	void ShowAttachedGrenade(bool bShowGrenade);
	void UpdateHUDGrenades();
	void EquipPrimaryWeapon(AWeapon* WeaponToEquip);
	void EquipSecondaryWeapon(AWeapon* WeaponToEquip);

private:
	UPROPERTY(ReplicatedUsing = OnRep_EquippedWeapon)
	TObjectPtr<AWeapon> EquippedWeapon;

	UFUNCTION()
	void OnRep_EquippedWeapon();

	UPROPERTY(ReplicatedUsing = OnRep_SecondaryWeapon)
	TObjectPtr<AWeapon> SecondaryWeapon;

	UFUNCTION()
	void OnRep_SecondaryWeapon();

	UPROPERTY(ReplicatedUsing = OnRep_Aiming)
	bool bAiming;

	UFUNCTION()
	void OnRep_Aiming();

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

	UPROPERTY(EditAnywhere, Category = "Combat")
	int32 MaxGrenades;

	UPROPERTY(EditAnywhere, Category = "Combat")
	TSubclassOf<AProjectile> GrenadeClass;

	UPROPERTY(EditAnywhere, Category = "Combat")
	int32 MaxCarriedAmmo;

	// TMap은 레플리케이션이 안되서 레플리케이션하기 위한 변수임
	UPROPERTY(ReplicatedUsing = OnRep_CarriedAmmo)
	int32 CarriedAmmo;

	UFUNCTION()
	void OnRep_CarriedAmmo();

	UPROPERTY(ReplicatedUsing = OnRep_CombatState)
	ECombatState CombatState;

	UFUNCTION()
	void OnRep_CombatState();

	UPROPERTY(ReplicatedUsing = OnRep_Grenades)
	int32 Grenades;

	UFUNCTION()
	void OnRep_Grenades();

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

	bool bAimButtonPressed;
};
