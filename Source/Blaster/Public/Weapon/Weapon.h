// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Weapon/WeaponTypes.h"
#include "BlasterTypes/Team.h"
#include "Weapon.generated.h"

/**
 * ������ ����
 */
UENUM(BlueprintType)
enum class EWeaponState : uint8
{
	EWS_Initial				UMETA(DisplayName = "Initial State"),
	EWS_Equipped			UMETA(DisplayName = "Equipped"),
	EWS_EquippedSecondary	UMETA(DisplayName = "Equipped Secondary"),
	EWS_Dropped				UMETA(DisplayName = "Dropped"),

	EWS_MAX					UMETA(DisplayName = "DefaultMAX")
};

/**
 * ������ ���� Ÿ��
 */
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

/**
 * ���� Ŭ����
 */
UCLASS()
class BLASTER_API AWeapon : public AActor
{
	GENERATED_BODY()
	
protected:
	AWeapon();

private:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void OnRep_Owner() override;

protected:
	virtual void BeginPlay() override;

	// ���� Ŭ�������� ���
public:
	virtual void Fire(const FVector& HitTarget);
	virtual void Dropped();

private:
	virtual void OnEquipped();
	virtual void OnDropped();

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
	FORCEINLINE ETeam GetTeam() const { return Team; }

	// ������ ���� ����
	void SetWeaponState(EWeaponState State);

	// E�� �ݱ� ���� UI ǥ��
	void ShowPickupWidget(bool bShowWidget);

	// ź�� ����
	// ź�� UI ����
	void SetHUDAmmo();
	// ź�� ���� Ȯ��
	bool IsEmpty() const;
	bool IsFull() const;
	// ź�� �߰�
	void AddAmmo(int32 AmmoToAdd);

	// �ܰ� ���� ó�� ����
	void EnableCustomDepth(bool bEnable);

	// ���� �������� �����ϰ� ����
	FVector TraceEndWithScatter(const FVector& HitTarget) const;

protected:
	// ���� ���� �ʱ�ȭ
	void CheckInit();

	// �÷��̾ �ݱ� ���� �浹 ó��
	UFUNCTION()
	void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	UFUNCTION()
	void OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

private:
	// ������ ���� ����
	void OnWeaponStateSet();

	// �������� ����
	void OnEquippedSecondary();

	// ź�� ����
	// ź�� ���
	void SpendRound();
	// Ŭ���̾�Ʈ���� ź�� ó��
	UFUNCTION(Client, Reliable)
	void ClientUpdateAmmo(int32 ServerAmmo);
	UFUNCTION(Client, Reliable)
	void ClientAddAmmo(int32 AmmoToAdd);

	// ���� �ʹ� ���� �� ���⿡ ���� �ǰ��� ���
	UFUNCTION()
	void OnPingTooHigh(bool bPingTooHigh);

protected:
	// ���⸦ �ֿ� �� ����� �浹 ��
	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	TObjectPtr<USphereComponent> AreaSphere;
	// ���⸦ �ֿ� �� ��� UI
	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	TObjectPtr<UWidgetComponent> PickupWidget;

	// ������
	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	float Damage = 20.f;
	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	float HeadShotDamage = 40.f;

	// ���� �� ź�� ������ ��ġ
	UPROPERTY(EditAnywhere, Category = "Weapon Scatter")
	float DistanceToSphere = 800.f;
	// ���� �� ź�� ������ ����
	UPROPERTY(EditAnywhere, Category = "Weapon Scatter")
	float SphereRadius = 75.f;

	// ������ ���� �ǰ��� ��� ����
	UPROPERTY(Replicated, EditAnywhere, Category = "Server Side Rewind")
	bool bUseServerSideRewind = false;

	// ���� ����
	UPROPERTY()
	TObjectPtr<ABlasterCharacter> BlasterOwnerCharacter;
	UPROPERTY()
	TObjectPtr<ABlasterPlayerController> BlasterOwnerController;

private:
	// ���� �޽�
	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	TObjectPtr<USkeletalMeshComponent> WeaponMesh;

	// ���� Ÿ��
	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	EWeaponType WeaponType;

	// ������ ���� Ÿ��
	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	EFireType FireType;

	// ���� ����
	UPROPERTY(ReplicatedUsing = OnRep_WeaponState, VisibleAnywhere, Category = "Weapon Properties")
	EWeaponState WeaponState;
	UFUNCTION()
	void OnRep_WeaponState();

	// ���� ����
	UPROPERTY(EditAnywhere, Category = "Sound")
	TObjectPtr<USoundCue> EquipSound;

	// ���� ����
	// �߻� �ִϸ��̼�
	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	TObjectPtr<UAnimationAsset> FireAnimation;
	// �߻� ������
	UPROPERTY(EditAnywhere, Category = "Automatic")
	float FireDelay = 0.15f;
	// �ڵ� �߻� ����
	UPROPERTY(EditAnywhere, Category = "Automatic")
	bool bAutomatic = true;
	// ź ���� ����
	UPROPERTY(EditAnywhere, Category = "Weapon Scatter")
	bool bUseScatter = false;

	// ź�� ����
	// ź�� ��
	UPROPERTY(EditAnywhere, Category = "Ammo")
	int32 Ammo;
	// Ammo�� ���� ó������ ���� ���� ��û ��
	// SpendRound()���� �����ϰ�, ClientUpdateAmmo()���� �پ�� ��
	int32 Sequence = 0;
	// źâ ��
	UPROPERTY(EditAnywhere, Category = "Ammo")
	int32 MagCapacity;

	// ź�� Ŭ����
	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	TSubclassOf<ACasing> CasingClass;

	// ���ڼ� ����
	// ���ڼ� �ؽ���
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
	// �� FOV
	UPROPERTY(EditAnywhere, Category = "Zoom")
	float ZoomedFOV = 30.f;
	// �� �ӵ�
	UPROPERTY(EditAnywhere, Category = "Zoom")
	float ZoomInterpSpeed = 20.f;

	// ���� �� ����� ����� �����ǵ��� �ϱ����� ����
	bool bDestroyWeapon = false;

	// ��߿��� ����� �� ����
	UPROPERTY(EditAnywhere, Category = "Flag")
	ETeam Team;
};
