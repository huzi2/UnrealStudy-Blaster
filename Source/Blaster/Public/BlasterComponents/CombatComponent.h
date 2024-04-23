// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HUD/BlasterHUD.h"
#include "Weapon/WeaponTypes.h"
#include "BlasterTypes/CombatState.h"
#include "CombatComponent.generated.h"

class ABlasterCharacter;
class AWeapon;
class ABlasterPlayerController;
class ABlasterHUD;
class AProjectile;

/**
 * ĳ������ ���� ���� ������Ʈ
 */
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class BLASTER_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()

private:
	UCombatComponent();
	friend ABlasterCharacter;

private:
	virtual void BeginPlay() final;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) final;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const final;

public:
	FORCEINLINE int32 GetGrenades() const { return Grenades; }
	FORCEINLINE bool GetLocallyReloading() const { return bLocallyReloading; }

	// ���� ����
	void EquipWeapon(AWeapon* WeaponToEquip);
	// ���� ����
	void SwapWeapons();
	bool ShouldSwapWeapons() const;
	// ���� ���� �� ����
	void SetAiming(bool bIsAiming);
	// ���� ������
	void Reload();
	// ���� �߻� �� ����
	void FireButtonPressed(bool bPressed);
	// ����ź ��ô
	void ThrowGrenade();
	// �Ѿ� ������ ȹ��
	void PickupAmmo(EWeaponType WeaponType, int32 AmmoAmount);
	// ���� ���� ����
	void JumpToShotgunEnd();

private:
	// �ʱ�ȭ
	void CheckInit();

	// ������ ���� �Լ�
	void TraceUnderCrosshairs(FHitResult& TraceHitResult);
	void SetHUDCrosshairs(float DeltaTime);

	// ���� ���� ���� �Լ�
	void EquipPrimaryWeapon(AWeapon* WeaponToEquip);
	void EquipSecondaryWeapon(AWeapon* WeaponToEquip);
	void DropEquippedWeapon();
	void AttachActorToRightHand(AActor* ActorToAttach);
	void AttachActorToLeftHand(AActor* ActorToAttach);
	void AttachActorToBackpack(AActor* ActorToAttach);
	void PlayEquipWeaponSound(AWeapon* WeaponToEquip);

	// ���� ���� ���� �Լ�
	UFUNCTION(BlueprintCallable)
	void FinishSwapAttachWeapons();
	UFUNCTION(BlueprintCallable)
	void FinishSwap();

	// ���� ���� ���� �Լ�
	void InterpFOV(float DeltaTime);
	UFUNCTION(Server, Reliable)
	void ServerSetAiming(bool bIsAiming);

	// ���� ������ ���� �Լ�
	UFUNCTION(BlueprintCallable)
	void FinishReloading();
	UFUNCTION(BlueprintCallable)
	void ShotgunShellReload();
	UFUNCTION(Server, Reliable)
	void ServerReload();
	void HandleReload();
	int32 AmountToReload() const;
	void ReloadEmptyWeapon();

	// ���� �߻� ���� �Լ�
	bool CanFire() const;
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerFire(const FVector_NetQuantize& TraceHitTarget, float FireDelay);
	UFUNCTION(NetMulticast, Reliable)
	void MulticastFire(const FVector_NetQuantize& TraceHitTarget);
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerShotgunFire(const TArray<FVector_NetQuantize>& TraceHitTargets, float FireDelay);
	UFUNCTION(NetMulticast, Reliable)
	void MulticastShotgunFire(const TArray<FVector_NetQuantize>& TraceHitTargets);
	void Fire();
	void FireProjectileWeapon();
	void FireHitScanWeapon();
	void FireShotgun();
	void LocalFire(const FVector_NetQuantize& TraceHitTarget);
	void LocalShotgunFire(const TArray<FVector_NetQuantize>& TraceHitTargets);
	void StartFireTimer();
	void FireTimerFinished();

	// ź�� ���� �Լ�
	void InitializeCarriedAmmo();
	void UpdateAmmoValues();
	void UpdateShotgunAmmoValues();
	void UpdateCarriedAmmo();

	// ����ź ���� �Լ�
	UFUNCTION(BlueprintCallable)
	void LaunchGrenade();
	UFUNCTION(Server, Reliable)
	void ServerThrowGrenade();
	UFUNCTION(Server, Reliable)
	void ServerLaunchGrenade(const FVector_NetQuantize& Target);
	UFUNCTION(BlueprintCallable)
	void ThrowGrenadeFinished();
	void ShowAttachedGrenade(bool bShowGrenade);
	void UpdateHUDGrenades();

	// ��� ���� �Լ�
	void AttachFlagToLeftHand(AWeapon* Flag);

private:
	// ���� ����
	UPROPERTY(ReplicatedUsing = OnRep_CombatState)
	ECombatState CombatState = ECombatState::ECS_Unoccupied;
	UFUNCTION()
	void OnRep_CombatState();

	// ������ ���� ����
	// ������ �ؽ�ó
	FHUDPackage HUDPackage;
	// ������ �ӵ�
	double CrosshairVelocityFactor;
	double CrosshairInAirFactor;
	double CrosshairAimFactor;
	double CrosshairShootingFactor;

	// ���� ���� ���� ����
	UPROPERTY(ReplicatedUsing = OnRep_EquippedWeapon)
	TObjectPtr<AWeapon> EquippedWeapon;
	UFUNCTION()
	void OnRep_EquippedWeapon();
	UPROPERTY(ReplicatedUsing = OnRep_SecondaryWeapon)
	TObjectPtr<AWeapon> SecondaryWeapon;
	UFUNCTION()
	void OnRep_SecondaryWeapon();

	// ���� ���� ���� ����
	// ���� ���ΰ�?
	UPROPERTY(ReplicatedUsing = OnRep_Aiming)
	bool bAiming = false;
	UFUNCTION()
	void OnRep_Aiming();
	// ���� ��ư�� ���� �����ΰ�?
	bool bAimButtonPressed = false;
	// FOV
	UPROPERTY(EditAnywhere, Category = "Combat")
	float ZoomedFOV = 30.f;
	float DefaultFOV;
	float CurrentFOV;
	// ���� �ӵ�
	UPROPERTY(EditAnywhere, Category = "Combat")
	float ZoomInterpSpeed = 20.f;
	// ���� �� �̵��ӵ� ����
	UPROPERTY(EditAnywhere, Category = "Combat")
	float BaseWalkSpeed = 600.f;
	UPROPERTY(EditAnywhere, Category = "Combat")
	float AimWalkSpeed = 450.f;

	// ���� ������ ���� ����
	// ������ ���ΰ�?
	bool bLocallyReloading = false;

	// ���� �߻� ���� ����
	// ��� ������ �����ΰ�?
	bool bCanFire = true;
	// �߻� ��ư�� �����°�? Ȥ�� �ڵ���� ���ΰ�?
	bool bFireButtonPressed;
	// �߻�ü�� ���� ��
	FVector HitTarget;
	// ��� ���� Ÿ�̸�. ������ �ڵ������ �ϴ�, �������� �ϴ�����
	FTimerHandle FireTimer;
	
	// ���� ź�� ���� ����
	// ������ �� ������ �ִ� ź�� ��
	UPROPERTY(EditAnywhere, Category = "Combat")
	int32 StartingARAmmo = 30;
	UPROPERTY(EditAnywhere, Category = "Combat")
	int32 StartingRocketAmmo = 0;
	UPROPERTY(EditAnywhere, Category = "Combat")
	int32 StartingPistolAmmo = 0;
	UPROPERTY(EditAnywhere, Category = "Combat")
	int32 StartingSMGAmmo = 0;
	UPROPERTY(EditAnywhere, Category = "Combat")
	int32 StartingShotgunAmmo = 0;
	UPROPERTY(EditAnywhere, Category = "Combat")
	int32 StartingSniperAmmo = 0;
	UPROPERTY(EditAnywhere, Category = "Combat")
	int32 StartingGrenaderLauncherAmmo = 0;
	UPROPERTY(EditAnywhere, Category = "Combat")
	int32 MaxCarriedAmmo = 500;
	// ���� �������ִ� ź��
	// TMap�� ���ø����̼��� �ȵǼ� ���ø����̼��ϱ� ���� ������
	UPROPERTY(ReplicatedUsing = OnRep_CarriedAmmo)
	int32 CarriedAmmo;
	UFUNCTION()
	void OnRep_CarriedAmmo();
	// �÷��̾ ������ �ִ� ���� ������ ź��
	// �ش� ������ ���ø����̼����� �ʰ� CarriedAmmo�� ���� ������ �ִ� ������ TMap�� �ؽ������� ���ø����̼��� �ȵȴ�.
	TMap<EWeaponType, int32> CarriedAmmoMap;

	// ����ź ���� ����
	// ����ź Ŭ����
	UPROPERTY(EditAnywhere, Category = "Combat")
	TSubclassOf<AProjectile> GrenadeClass;
	// �ִ� ����ź ��
	UPROPERTY(EditAnywhere, Category = "Combat")
	int32 MaxGrenades = 4;
	// ���� ����ź ��
	UPROPERTY(ReplicatedUsing = OnRep_Grenades)
	int32 Grenades = 4;
	UFUNCTION()
	void OnRep_Grenades();

	// ��� ���� ����
	// ��� ������
	UPROPERTY(ReplicatedUsing = OnRep_TheFlag)
	AWeapon* TheFlag;
	UFUNCTION()
	void OnRep_TheFlag();
	// ����� ������ �ִ°�?
	UPROPERTY(ReplicatedUsing = OnRep_HoldingTheFlag)
	bool bHoldingTheFlag = false;
	UFUNCTION()
	void OnRep_HoldingTheFlag();

	// ���� ������
	UPROPERTY()
	TObjectPtr<ABlasterCharacter> Character;
	UPROPERTY()
	TObjectPtr<ABlasterPlayerController> Controller;
	UPROPERTY()
	TObjectPtr<ABlasterHUD> HUD;
};
