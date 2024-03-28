// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Weapon/WeaponTypes.h"
#include "BlasterTypes/Team.h"
#include "Weapon.generated.h"

UENUM(BlueprintType)
enum class EWeaponState : uint8
{
	EWS_Initial				UMETA(DisplayName = "Initial State"),
	EWS_Equipped			UMETA(DisplayName = "Equipped"),
	EWS_EquippedSecondary	UMETA(DisplayName = "Equipped Secondary"),
	EWS_Dropped				UMETA(DisplayName = "Dropped"),

	EWS_MAX					UMETA(DisplayName = "DefaultMAX")
};

UENUM(BlueprintType)
enum class EFireType : uint8
{
	EFT_HitScan		UMETA(DisplayName = "Hit Scan Weapon"),
	EFT_Projectile	UMETA(DisplayName = "Projectile Weapon"),
	EFT_Shotgun		UMETA(DisplayName = "Shotgun Weapon"),

	EFT_MAX			UMETA(DisplayName = "DefaultMAX")
};

class USphereComponent;
class UWidgetComponent;
class ACasing;
class ABlasterCharacter;
class ABlasterPlayerController;
class USoundCue;

UCLASS()
class BLASTER_API AWeapon : public AActor
{
	GENERATED_BODY()
	
protected:
	AWeapon();

private:
	virtual void BeginPlay() final;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const final;
	virtual void OnRep_Owner() final;

public:
	virtual void Fire(const FVector& HitTarget);
	virtual void Dropped();

private:
	virtual void OnEquipped();
	virtual void OnDropped();

protected:
	UFUNCTION()
	void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

private:
	UFUNCTION(Client, Reliable)
	void ClientUpdateAmmo(int32 ServerAmmo);

	UFUNCTION(Client, Reliable)
	void ClientAddAmmo(int32 AmmoToAdd);

	UFUNCTION()
	void OnPingTooHigh(bool bPingTooHigh);

public:
	FORCEINLINE float GetDamage() const { return Damage; }
	FORCEINLINE float GetHeadShotDamage() const { return HeadShotDamage; }
	FORCEINLINE USkeletalMeshComponent* GetWeaponMesh() const { return WeaponMesh; }
	FORCEINLINE USphereComponent* GetAreaSphere() const { return AreaSphere; }
	FORCEINLINE EWeaponType GetWeaponType() const { return WeaponType; }
	FORCEINLINE EFireType GetFireType() const { return FireType; }
	FORCEINLINE UTexture2D* GetCrosshairsCenter() const { return CrosshairsCenter; }
	FORCEINLINE UTexture2D* GetCrosshairsLeft() const { return CrosshairsLeft; }
	FORCEINLINE UTexture2D* GetCrosshairsRight() const { return CrosshairsRight; }
	FORCEINLINE UTexture2D* GetCrosshairsTop() const { return CrosshairsTop; }
	FORCEINLINE UTexture2D* GetCrosshairsBottom() const { return CrosshairsBottom; }
	FORCEINLINE float GetZoomedFOV() const { return ZoomedFOV; }
	FORCEINLINE float GetZoomInterpSpeed() const { return ZoomInterpSpeed; }
	FORCEINLINE bool IsAutomatic() const { return bAutomatic; }
	FORCEINLINE float GetFireDelay() const { return FireDelay; }
	FORCEINLINE int32 GetAmmo() const { return Ammo; }
	FORCEINLINE int32 GetMagCapacity() const { return MagCapacity; }
	FORCEINLINE USoundCue* GetEquipSound() const { return EquipSound; }
	FORCEINLINE bool GetDestroyWeapon() const { return bDestroyWeapon; }
	FORCEINLINE void SetDestroyWeapon(bool bDestroy) { bDestroyWeapon = bDestroy; }
	FORCEINLINE bool GetUseScatter() const { return bUseScatter; }
	void SetWeaponState(EWeaponState State);
	void ShowPickupWidget(bool bShowWidget);
	void SetHUDAmmo();
	bool IsEmpty() const;
	bool IsFull() const;
	void AddAmmo(int32 AmmoToAdd);
	void EnableCustomDepth(bool bEnable);
	FVector TraceEndWithScatter(const FVector& HitTarget) const;

protected:
	void CheckInit();

private:
	void SpendRound();
	void OnWeaponStateSet();
	void OnEquippedSecondary();

protected:
	UPROPERTY(EditAnywhere, Category = "Weapon Scatter")
	float DistanceToSphere;

	UPROPERTY(EditAnywhere, Category = "Weapon Scatter")
	float SphereRadius;

	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	TObjectPtr<USphereComponent> AreaSphere;

	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	TObjectPtr<UWidgetComponent> PickupWidget;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	float Damage;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	float HeadShotDamage;

	UPROPERTY(Replicated, EditAnywhere, Category = "Server Side Rewind")
	bool bUseServerSideRewind;

	UPROPERTY()
	TObjectPtr<ABlasterCharacter> BlasterOwnerCharacter;

	UPROPERTY()
	TObjectPtr<ABlasterPlayerController> BlasterOwnerController;

private:
	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	TObjectPtr<USkeletalMeshComponent> WeaponMesh;

	UPROPERTY(ReplicatedUsing = OnRep_WeaponState, VisibleAnywhere, Category = "Weapon Properties")
	EWeaponState WeaponState;

	UFUNCTION()
	void OnRep_WeaponState();

	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	TObjectPtr<UAnimationAsset> FireAnimation;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	TSubclassOf<ACasing> CasingClass;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	EWeaponType WeaponType;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	EFireType FireType;

	UPROPERTY(EditAnywhere, Category = "Crosshairs")
	TObjectPtr<UTexture2D> CrosshairsCenter;

	UPROPERTY(EditAnywhere, Category = "Crosshairs")
	TObjectPtr<UTexture2D> CrosshairsLeft;

	UPROPERTY(EditAnywhere, Category = "Crosshairs")
	TObjectPtr<UTexture2D> CrosshairsRight;

	UPROPERTY(EditAnywhere, Category = "Crosshairs")
	TObjectPtr<UTexture2D> CrosshairsTop;

	UPROPERTY(EditAnywhere, Category = "Crosshairs")
	TObjectPtr<UTexture2D> CrosshairsBottom;

	UPROPERTY(EditAnywhere, Category = "Zoom")
	float ZoomedFOV;

	UPROPERTY(EditAnywhere, Category = "Zoom")
	float ZoomInterpSpeed;

	UPROPERTY(EditAnywhere, Category = "Automatic")
	bool bAutomatic;

	UPROPERTY(EditAnywhere, Category = "Automatic")
	float FireDelay;

	UPROPERTY(EditAnywhere, Category = "Ammo")
	int32 Ammo;

	UPROPERTY(EditAnywhere, Category = "Ammo")
	int32 MagCapacity;

	UPROPERTY(EditAnywhere, Category = "Sound")
	TObjectPtr<USoundCue> EquipSound;

	UPROPERTY(EditAnywhere, Category = "Weapon Scatter")
	bool bUseScatter;

	UPROPERTY(EditAnywhere, Category = "Flag")
	ETeam Team;

private:
	bool bDestroyWeapon;

	// Ammo에 대해 처리되지 않은 서버 요청 수
	// SpendRound()에서 증가하고, ClientUpdateAmmo()에서 줄어들 것
	int32 Sequence;
};
