// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Weapon.generated.h"

UENUM(BlueprintType)
enum class EWeaponState : uint8
{
	EWS_Initial		UMETA(DisplayName = "Initial State"),
	EWS_Equipped	UMETA(DisplayName = "Equipped"),
	EWS_Dropped		UMETA(DisplayName = "Dropped"),
	EWS_MAX			UMETA(DisplayName = "DefaultMAX")
};

class USphereComponent;
class UWidgetComponent;
class ACasing;

UCLASS()
class BLASTER_API AWeapon : public AActor
{
	GENERATED_BODY()
	
protected:
	AWeapon();

private:
	virtual void BeginPlay() final;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const final;

public:
	virtual void Fire(const FVector& HitTarget);

protected:
	UFUNCTION()
	void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

public:
	FORCEINLINE USphereComponent* GetAreaSphere() const { return AreaSphere; }
	FORCEINLINE USkeletalMeshComponent* GetWeaponMesh() const { return WeaponMesh; }
	FORCEINLINE UTexture2D* GetCrosshairsCenter() const { return CrosshairsCenter; }
	FORCEINLINE UTexture2D* GetCrosshairsLeft() const { return CrosshairsLeft; }
	FORCEINLINE UTexture2D* GetCrosshairsRight() const { return CrosshairsRight; }
	FORCEINLINE UTexture2D* GetCrosshairsTop() const { return CrosshairsTop; }
	FORCEINLINE UTexture2D* GetCrosshairsBottom() const { return CrosshairsBottom; }
	FORCEINLINE float GetZoomedFOV() const { return ZoomedFOV; }
	FORCEINLINE float GetZoomInterpSpeed() const { return ZoomInterpSpeed; }
	FORCEINLINE bool IsAutomatic() const { return bAutomatic; }
	FORCEINLINE float GetFireDelay() const { return FireDelay; }
	void SetWeaponState(EWeaponState State);
	void ShowPickupWidget(bool bShowWidget);
	void Dropped();

private:
	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	TObjectPtr<USkeletalMeshComponent> WeaponMesh;

	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	TObjectPtr<USphereComponent> AreaSphere;

	UPROPERTY(ReplicatedUsing = OnRep_WeaponState, VisibleAnywhere, Category = "Weapon Properties")
	EWeaponState WeaponState;

	UFUNCTION()
	void OnRep_WeaponState();

	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	TObjectPtr<UWidgetComponent> PickupWidget;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	TObjectPtr<UAnimationAsset> FireAnimation;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	TSubclassOf<ACasing> CasingClass;

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
};
