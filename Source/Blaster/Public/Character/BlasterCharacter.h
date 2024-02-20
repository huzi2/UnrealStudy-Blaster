// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "BlasterTypes/TurningInPlace.h"
#include "Interfaces/InteractWithCrosshairsInterface.h"
#include "Components/TimelineComponent.h"
#include "BlasterCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UWidgetComponent;
class UCombatComponent;
class AWeapon;
class ABlasterPlayerController;
class USoundCue;
class ABlasterPlayerState;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

UCLASS()
class BLASTER_API ABlasterCharacter : public ACharacter, public IInteractWithCrosshairsInterface
{
	GENERATED_BODY()

private:
	ABlasterCharacter();

private:
	virtual void BeginPlay() final;
	virtual void PostInitializeComponents() final;
	virtual void Tick(float DeltaTime) final;
	virtual void Destroyed() final;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) final;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const final;
	virtual void Jump() final;
	virtual void OnRep_ReplicatedMovement() final;

public:
	UFUNCTION(NetMulticast, Reliable)
	void MulticastElim();

private:
	UFUNCTION(Server, Reliable)
	void ServerEquipButtonPressed();

	UFUNCTION()
	void ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatorController, AActor* DamageCauser);

	UFUNCTION()
	void UpdateDissolveMaterial(float DissolveValue);

public:
	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }
	FORCEINLINE float GetHealth() const { return Health; }
	FORCEINLINE float GetMaxHealth() const { return MaxHealth; }
	FORCEINLINE float GetAOYaw() const { return AO_Yaw; }
	FORCEINLINE float GetAOPitch() const { return AO_Pitch; }
	FORCEINLINE ETurningInPlace GetTurningInPlace() const { return TurningInPlace; }
	FORCEINLINE bool ShouldRotateRootBone() const { return bRotateRootBone; }
	FORCEINLINE bool IsElimmed() const { return bElimmed; }
	AWeapon* GetEquippedWeapon() const;
	FVector GetHitTarget() const;

	void SetOverlappingWeapon(AWeapon* Weapon);
	bool IsWeaponEquipped() const;
	bool IsAiming() const;
	void PlayFireMontage(bool bAiming);
	void PlayHitReactMontage();
	void PlayElimMontage();
	void Elim();

private:
	void MoveForward(const FInputActionValue& Value);
	void MoveRight(const FInputActionValue& Value);
	void Turn(const FInputActionValue& Value);
	void LookUp(const FInputActionValue& Value);
	void EquipButtonPressed();
	void CrouchButtonPressed();
	void AimButtonPressed();
	void AimButtonReleased();
	void FireButtonPressed();
	void FireButtonReleased();

	void AimOffset(float DeltaTime);
	void SimProxiesTurn();
	void TurnInPlace(float DeltaTime);
	void HideCameraIfCharacterClose();
	void CalculateAO_Pitch();
	double CaculateSpeed() const;
	void UpdateHUDHealth();
	void ElimTimerFinished();
	void StartDissolve();
	void PollInit();

private:
	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY(EditAnywhere, Category = "Camera")
	double CameraThreshlod;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UWidgetComponent> OverheadWidget;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UCombatComponent> Combat;

	UPROPERTY(ReplicatedUsing = OnRep_OverlappingWeapon)
	TObjectPtr<AWeapon> OverlappingWeapon;

	UFUNCTION()
	void OnRep_OverlappingWeapon(AWeapon* LastWeapon);

	UPROPERTY(EditAnywhere, Category = "Combat")
	TObjectPtr<UAnimMontage> FireWeaponMontage;

	UPROPERTY(EditAnywhere, Category = "Combat")
	TObjectPtr<UAnimMontage> HitReactMontage;

	UPROPERTY(EditAnywhere, Category = "Combat")
	TObjectPtr<UAnimMontage> ElimMontage;

	UPROPERTY(EditAnywhere, Category = "Player Stats")
	float MaxHealth;

	UPROPERTY(ReplicatedUsing = OnRep_Health, VisibleAnywhere, Category = "Player Stats")
	float Health;

	UFUNCTION()
	void OnRep_Health();

	UPROPERTY(EditDefaultsOnly, Category = "Elim")
	float ElimDelay;

	UPROPERTY(VisibleAnywhere, Category = "Elim")
	TObjectPtr<UTimelineComponent> DissolveTimeline;

	UPROPERTY(EditAnywhere, Category = "Elim")
	TObjectPtr<UCurveFloat> DissolveCurve;

	UPROPERTY(VisibleAnywhere, Category = "Elim")
	TObjectPtr<UMaterialInstanceDynamic> DynamicDissolveMaterialInstance;

	UPROPERTY(EditAnywhere, Category = "Elim")
	TObjectPtr<UMaterialInstance> DissolveMaterialInstance;

	UPROPERTY(EditAnywhere, Category = "Elim")
	TObjectPtr<UParticleSystem> ElimBotEffect;

	UPROPERTY(VisibleAnywhere, Category = "Elim")
	TObjectPtr<UParticleSystemComponent> ElimBotComponent;

	UPROPERTY(EditAnywhere, Category = "Elim")
	TObjectPtr<USoundCue> ElimBotSound;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultInputMappingContext;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> JumpInputAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> MoveForwardInputAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> MoveRightInputAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> TurnInputAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> LookUpInputAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> EquipInputAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> CrouchInputAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> AimInputAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> FireInputAction;

	UPROPERTY()
	TObjectPtr<ABlasterPlayerController> BlasterPlayerController;

	UPROPERTY()
	TObjectPtr<ABlasterPlayerState> BlasterPlayerState;

private:
	float AO_Yaw;
	float InterpAO_Yaw;
	float AO_Pitch;
	FRotator StartingAimRotation;
	ETurningInPlace TurningInPlace;
	
	bool bRotateRootBone;

	double TurnThreshold;
	FRotator ProxyRotationLastFrame;
	FRotator ProxyRotation;
	double ProxyYaw;
	float TimeSinceLastMovementReplication;

	bool bElimmed;
	FTimerHandle ElimTimer;
	FOnTimelineFloat DissolveTrack;
};
