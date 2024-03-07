// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "BlasterTypes/TurningInPlace.h"
#include "Interfaces/InteractWithCrosshairsInterface.h"
#include "Components/TimelineComponent.h"
#include "BlasterTypes/CombatState.h"
#include "BlasterCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UWidgetComponent;
class UCombatComponent;
class UBuffComponent;
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

	UFUNCTION(BlueprintImplementableEvent)
	void ShowSniperScopeWidget(bool bShowScope);

private:
	UFUNCTION(Server, Reliable)
	void ServerEquipButtonPressed();

	UFUNCTION()
	void ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatorController, AActor* DamageCauser);

	UFUNCTION()
	void UpdateDissolveMaterial(float DissolveValue);

public:
	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }
	FORCEINLINE UCombatComponent* GetCombat() const { return Combat; }
	FORCEINLINE UBuffComponent* GetBuff() const { return Buff; }
	FORCEINLINE UAnimMontage* GetReloadMontage() const { return ReloadMontage; }
	FORCEINLINE float GetHealth() const { return Health; }
	FORCEINLINE void SetHealth(float Amount) { Health = Amount; }
	FORCEINLINE float GetMaxHealth() const { return MaxHealth; }
	FORCEINLINE float GetAOYaw() const { return AO_Yaw; }
	FORCEINLINE float GetAOPitch() const { return AO_Pitch; }
	FORCEINLINE ETurningInPlace GetTurningInPlace() const { return TurningInPlace; }
	FORCEINLINE bool ShouldRotateRootBone() const { return bRotateRootBone; }
	FORCEINLINE bool IsElimmed() const { return bElimmed; }
	FORCEINLINE UStaticMeshComponent* GetAttachedGrenade() const { return AttachedGrenade; }
	FORCEINLINE bool GetDisableGameplay() const { return bDisableGameplay; }
	FORCEINLINE void SetDisableGameplay(bool bDisable) { bDisableGameplay = bDisable; }
	AWeapon* GetEquippedWeapon() const;
	FVector GetHitTarget() const;
	ECombatState GetCombatState() const;

	void SetOverlappingWeapon(AWeapon* Weapon);
	bool IsWeaponEquipped() const;
	bool IsAiming() const;
	void PlayFireMontage(bool bAiming);
	void PlayReloadMontage();
	void PlayHitReactMontage();
	void PlayElimMontage();
	void PlayThrowGrenadeMontage();
	void Elim();
	void UpdateHUDHealth();

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
	void ReloadButtonPressed();
	void ThrowGrenadeButtonPressed();

	void AimOffset(float DeltaTime);
	void SimProxiesTurn();
	void TurnInPlace(float DeltaTime);
	void HideCameraIfCharacterClose();
	void CalculateAO_Pitch();
	double CaculateSpeed() const;
	void ElimTimerFinished();
	void StartDissolve();
	void PollInit();
	void RotateInPlace(float DeltaTime);

private:
	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY(EditAnywhere, Category = "Camera")
	double CameraThreshlod;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UWidgetComponent> OverheadWidget;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCombatComponent> Combat;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UBuffComponent> Buff;

	UPROPERTY(ReplicatedUsing = OnRep_OverlappingWeapon)
	TObjectPtr<AWeapon> OverlappingWeapon;

	UFUNCTION()
	void OnRep_OverlappingWeapon(AWeapon* LastWeapon);

	UPROPERTY(EditAnywhere, Category = "Combat")
	TObjectPtr<UAnimMontage> FireWeaponMontage;

	UPROPERTY(EditAnywhere, Category = "Combat")
	TObjectPtr<UAnimMontage> ReloadMontage;

	UPROPERTY(EditAnywhere, Category = "Combat")
	TObjectPtr<UAnimMontage> HitReactMontage;

	UPROPERTY(EditAnywhere, Category = "Combat")
	TObjectPtr<UAnimMontage> ElimMontage;

	UPROPERTY(EditAnywhere, Category = "Combat")
	TObjectPtr<UAnimMontage> ThrowGrenadeMontage;

	UPROPERTY(EditAnywhere, Category = "Player Stats")
	float MaxHealth;

	UPROPERTY(ReplicatedUsing = OnRep_Health, VisibleAnywhere, Category = "Player Stats")
	float Health;

	UFUNCTION()
	void OnRep_Health(float LastHealth);

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

	UPROPERTY(VisibleAnywhere, Category = "Grenade")
	TObjectPtr<UStaticMeshComponent> AttachedGrenade;

	// 쿨다운 상태일 때 특정 입력을 막기 위함
	UPROPERTY(Replicated)
	bool bDisableGameplay;

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

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> ReloadInputAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> ThrowGrenadeInputAction;

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
