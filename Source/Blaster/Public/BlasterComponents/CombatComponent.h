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
 * 캐릭터의 무기 관리 컴포넌트
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

	// 무기 장착
	void EquipWeapon(AWeapon* WeaponToEquip);
	// 무기 스왑
	void SwapWeapons();
	bool ShouldSwapWeapons() const;
	// 무기 조준 및 해제
	void SetAiming(bool bIsAiming);
	// 무기 재장전
	void Reload();
	// 무기 발사 및 해제
	void FireButtonPressed(bool bPressed);
	// 수류탄 투척
	void ThrowGrenade();
	// 총알 아이템 획득
	void PickupAmmo(EWeaponType WeaponType, int32 AmmoAmount);
	// 샷건 장전 종료
	void JumpToShotgunEnd();

private:
	// 초기화
	void CheckInit();

	// 조준점 관련 함수
	void TraceUnderCrosshairs(FHitResult& TraceHitResult);
	void SetHUDCrosshairs(float DeltaTime);

	// 무기 장착 관련 함수
	void EquipPrimaryWeapon(AWeapon* WeaponToEquip);
	void EquipSecondaryWeapon(AWeapon* WeaponToEquip);
	void DropEquippedWeapon();
	void AttachActorToRightHand(AActor* ActorToAttach);
	void AttachActorToLeftHand(AActor* ActorToAttach);
	void AttachActorToBackpack(AActor* ActorToAttach);
	void PlayEquipWeaponSound(AWeapon* WeaponToEquip);

	// 무기 스왑 관련 함수
	UFUNCTION(BlueprintCallable)
	void FinishSwapAttachWeapons();
	UFUNCTION(BlueprintCallable)
	void FinishSwap();

	// 무기 조준 관련 함수
	void InterpFOV(float DeltaTime);
	UFUNCTION(Server, Reliable)
	void ServerSetAiming(bool bIsAiming);

	// 무기 재장전 관련 함수
	UFUNCTION(BlueprintCallable)
	void FinishReloading();
	UFUNCTION(BlueprintCallable)
	void ShotgunShellReload();
	UFUNCTION(Server, Reliable)
	void ServerReload();
	void HandleReload();
	int32 AmountToReload() const;
	void ReloadEmptyWeapon();

	// 무기 발사 관련 함수
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

	// 탄약 관련 함수
	void InitializeCarriedAmmo();
	void UpdateAmmoValues();
	void UpdateShotgunAmmoValues();
	void UpdateCarriedAmmo();

	// 수류탄 관련 함수
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

	// 깃발 관련 함수
	void AttachFlagToLeftHand(AWeapon* Flag);

private:
	// 전투 상태
	UPROPERTY(ReplicatedUsing = OnRep_CombatState)
	ECombatState CombatState = ECombatState::ECS_Unoccupied;
	UFUNCTION()
	void OnRep_CombatState();

	// 조준점 관련 변수
	// 조준점 텍스처
	FHUDPackage HUDPackage;
	// 조준점 속도
	double CrosshairVelocityFactor;
	double CrosshairInAirFactor;
	double CrosshairAimFactor;
	double CrosshairShootingFactor;

	// 무기 장착 관련 변수
	UPROPERTY(ReplicatedUsing = OnRep_EquippedWeapon)
	TObjectPtr<AWeapon> EquippedWeapon;
	UFUNCTION()
	void OnRep_EquippedWeapon();
	UPROPERTY(ReplicatedUsing = OnRep_SecondaryWeapon)
	TObjectPtr<AWeapon> SecondaryWeapon;
	UFUNCTION()
	void OnRep_SecondaryWeapon();

	// 무기 조준 관련 변수
	// 조준 중인가?
	UPROPERTY(ReplicatedUsing = OnRep_Aiming)
	bool bAiming = false;
	UFUNCTION()
	void OnRep_Aiming();
	// 조준 버튼을 누른 상태인가?
	bool bAimButtonPressed = false;
	// FOV
	UPROPERTY(EditAnywhere, Category = "Combat")
	float ZoomedFOV = 30.f;
	float DefaultFOV;
	float CurrentFOV;
	// 조준 속도
	UPROPERTY(EditAnywhere, Category = "Combat")
	float ZoomInterpSpeed = 20.f;
	// 조준 시 이동속도 설정
	UPROPERTY(EditAnywhere, Category = "Combat")
	float BaseWalkSpeed = 600.f;
	UPROPERTY(EditAnywhere, Category = "Combat")
	float AimWalkSpeed = 450.f;

	// 무기 재장전 관련 변수
	// 재장전 중인가?
	bool bLocallyReloading = false;

	// 무기 발사 관련 변수
	// 사격 가능한 상태인가?
	bool bCanFire = true;
	// 발사 버튼을 눌렀는가? 혹은 자동사격 중인가?
	bool bFireButtonPressed;
	// 발사체가 닿을 곳
	FVector HitTarget;
	// 사격 관련 타이머. 끝나면 자동사격을 하던, 재장전을 하던가함
	FTimerHandle FireTimer;
	
	// 무기 탄약 관련 변수
	// 시작할 때 가지고 있는 탄약 수
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
	// 지금 가지고있는 탄약
	// TMap은 레플리케이션이 안되서 레플리케이션하기 위한 변수임
	UPROPERTY(ReplicatedUsing = OnRep_CarriedAmmo)
	int32 CarriedAmmo;
	UFUNCTION()
	void OnRep_CarriedAmmo();
	// 플레이어가 가지고 있는 무기 종류별 탄약
	// 해당 변수를 레플리케이션하지 않고 CarriedAmmo로 따로 가지고 있는 이유는 TMap은 해쉬맵으로 레플리케이션이 안된다.
	TMap<EWeaponType, int32> CarriedAmmoMap;

	// 수류탄 관련 변수
	// 수류탄 클래스
	UPROPERTY(EditAnywhere, Category = "Combat")
	TSubclassOf<AProjectile> GrenadeClass;
	// 최대 수류탄 수
	UPROPERTY(EditAnywhere, Category = "Combat")
	int32 MaxGrenades = 4;
	// 현재 수류탄 수
	UPROPERTY(ReplicatedUsing = OnRep_Grenades)
	int32 Grenades = 4;
	UFUNCTION()
	void OnRep_Grenades();

	// 깃발 관련 변수
	// 깃발 포인터
	UPROPERTY(ReplicatedUsing = OnRep_TheFlag)
	AWeapon* TheFlag;
	UFUNCTION()
	void OnRep_TheFlag();
	// 깃발을 가지고 있는가?
	UPROPERTY(ReplicatedUsing = OnRep_HoldingTheFlag)
	bool bHoldingTheFlag = false;
	UFUNCTION()
	void OnRep_HoldingTheFlag();

	// 참조 변수들
	UPROPERTY()
	TObjectPtr<ABlasterCharacter> Character;
	UPROPERTY()
	TObjectPtr<ABlasterPlayerController> Controller;
	UPROPERTY()
	TObjectPtr<ABlasterHUD> HUD;
};
