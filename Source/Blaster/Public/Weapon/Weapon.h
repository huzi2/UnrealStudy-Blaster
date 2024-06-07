// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Weapon/WeaponTypes.h"
#include "BlasterTypes/Team.h"
#include "Weapon.generated.h"

/**
 * 무기의 상태
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
 * 무기의 공격 타입
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
 * 무기 클래스
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

	// 하위 클래스에게 상속
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

	// 무기의 상태 변경
	void SetWeaponState(EWeaponState State);

	// E로 줍기 가능 UI 표시
	void ShowPickupWidget(bool bShowWidget);

	// 탄약 관련
	// 탄약 UI 세팅
	void SetHUDAmmo();
	// 탄약 상태 확인
	bool IsEmpty() const;
	bool IsFull() const;
	// 탄약 추가
	void AddAmmo(int32 AmmoToAdd);

	// 외곽 강조 처리 설정
	void EnableCustomDepth(bool bEnable);

	// 공격 도착지점 랜덤하게 세팅
	FVector TraceEndWithScatter(const FVector& HitTarget) const;

protected:
	// 참조 변수 초기화
	void CheckInit();

	// 플레이어가 줍기 위한 충돌 처리
	UFUNCTION()
	void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	UFUNCTION()
	void OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

private:
	// 무기의 상태 설정
	void OnWeaponStateSet();

	// 보조무기 장착
	void OnEquippedSecondary();

	// 탄약 관련
	// 탄약 사용
	void SpendRound();
	// 클라이언트에서 탄약 처리
	UFUNCTION(Client, Reliable)
	void ClientUpdateAmmo(int32 ServerAmmo);
	UFUNCTION(Client, Reliable)
	void ClientAddAmmo(int32 AmmoToAdd);

	// 핑이 너무 높을 때 무기에 서버 되감기 사용
	UFUNCTION()
	void OnPingTooHigh(bool bPingTooHigh);

protected:
	// 무기를 주울 때 사용할 충돌 원
	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	TObjectPtr<USphereComponent> AreaSphere;
	// 무기를 주울 때 띄울 UI
	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	TObjectPtr<UWidgetComponent> PickupWidget;

	// 데미지
	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	float Damage = 20.f;
	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	float HeadShotDamage = 40.f;

	// 공격 시 탄이 퍼지는 위치
	UPROPERTY(EditAnywhere, Category = "Weapon Scatter")
	float DistanceToSphere = 800.f;
	// 공격 시 탄이 퍼지는 범위
	UPROPERTY(EditAnywhere, Category = "Weapon Scatter")
	float SphereRadius = 75.f;

	// 무기의 서버 되감기 사용 유무
	UPROPERTY(Replicated, EditAnywhere, Category = "Server Side Rewind")
	bool bUseServerSideRewind = false;

	// 참조 변수
	UPROPERTY()
	TObjectPtr<ABlasterCharacter> BlasterOwnerCharacter;
	UPROPERTY()
	TObjectPtr<ABlasterPlayerController> BlasterOwnerController;

private:
	// 무기 메쉬
	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	TObjectPtr<USkeletalMeshComponent> WeaponMesh;

	// 무기 타입
	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	EWeaponType WeaponType;

	// 무기의 공격 타입
	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	EFireType FireType;

	// 무기 상태
	UPROPERTY(ReplicatedUsing = OnRep_WeaponState, VisibleAnywhere, Category = "Weapon Properties")
	EWeaponState WeaponState;
	UFUNCTION()
	void OnRep_WeaponState();

	// 장착 사운드
	UPROPERTY(EditAnywhere, Category = "Sound")
	TObjectPtr<USoundCue> EquipSound;

	// 공격 관련
	// 발사 애니메이션
	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	TObjectPtr<UAnimationAsset> FireAnimation;
	// 발사 딜레이
	UPROPERTY(EditAnywhere, Category = "Automatic")
	float FireDelay = 0.15f;
	// 자동 발사 유무
	UPROPERTY(EditAnywhere, Category = "Automatic")
	bool bAutomatic = true;
	// 탄 퍼짐 유무
	UPROPERTY(EditAnywhere, Category = "Weapon Scatter")
	bool bUseScatter = false;

	// 탄약 관련
	// 탄약 수
	UPROPERTY(EditAnywhere, Category = "Ammo")
	int32 Ammo;
	// Ammo에 대해 처리되지 않은 서버 요청 수
	// SpendRound()에서 증가하고, ClientUpdateAmmo()에서 줄어들 것
	int32 Sequence = 0;
	// 탄창 수
	UPROPERTY(EditAnywhere, Category = "Ammo")
	int32 MagCapacity;

	// 탄피 클래스
	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	TSubclassOf<ACasing> CasingClass;

	// 십자선 관련
	// 십자선 텍스쳐
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
	// 줌 FOV
	UPROPERTY(EditAnywhere, Category = "Zoom")
	float ZoomedFOV = 30.f;
	// 줌 속도
	UPROPERTY(EditAnywhere, Category = "Zoom")
	float ZoomInterpSpeed = 20.f;

	// 시작 때 들고가는 무기는 삭제되도록 하기위한 변수
	bool bDestroyWeapon = false;

	// 깃발에서 사용할 팀 정보
	UPROPERTY(EditAnywhere, Category = "Flag")
	ETeam Team;
};
