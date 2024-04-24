// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "BlasterTypes/TurningInPlace.h"
#include "Interfaces/InteractWithCrosshairsInterface.h"
#include "Components/TimelineComponent.h"
#include "BlasterTypes/CombatState.h"
#include "BlasterTypes/Team.h"
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
class UBoxComponent;
class ULagCompensationComponent;
class ABlasterGameMode;
class UNiagaraSystem;
class UNiagaraComponent;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

// ĳ���Ͱ� ������ ������ �� ����� ��������Ʈ
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLeftGame);

/**
 * �÷��̾� ĳ���� Ŭ����
 */
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
	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }
	FORCEINLINE UCombatComponent* GetCombat() const { return Combat; }
	FORCEINLINE UBuffComponent* GetBuff() const { return Buff; }
	FORCEINLINE ULagCompensationComponent* GetLagCompensation() const { return LagCompensation; }
	FORCEINLINE UAnimMontage* GetReloadMontage() const { return ReloadMontage; }
	FORCEINLINE float GetHealth() const { return Health; }
	FORCEINLINE void SetHealth(float Amount) { Health = Amount; }
	FORCEINLINE float GetMaxHealth() const { return MaxHealth; }
	FORCEINLINE float GetShield() const { return Shield; }
	FORCEINLINE void SetShield(float Amount) { Shield = Amount; }
	FORCEINLINE float GetMaxShield() const { return MaxShield; }
	FORCEINLINE float GetAOYaw() const { return AO_Yaw; }
	FORCEINLINE float GetAOPitch() const { return AO_Pitch; }
	FORCEINLINE ETurningInPlace GetTurningInPlace() const { return TurningInPlace; }
	FORCEINLINE bool ShouldRotateRootBone() const { return bRotateRootBone; }
	FORCEINLINE bool IsElimmed() const { return bElimmed; }
	FORCEINLINE UStaticMeshComponent* GetAttachedGrenade() const { return AttachedGrenade; }
	FORCEINLINE bool GetDisableGameplay() const { return bDisableGameplay; }
	FORCEINLINE void SetDisableGameplay(bool bDisable) { bDisableGameplay = bDisable; }
	FORCEINLINE const TMap<FName, UBoxComponent*>& GetHitCollisionBoxes() const { return HitCollisionBoxes; }
	FORCEINLINE bool IsbFinishedSwapping() const { return bFinishedSwapping; }
	FORCEINLINE void SetIsbFinishedSwapping(bool Finished) { bFinishedSwapping = Finished; }
	AWeapon* GetEquippedWeapon() const;
	FVector GetHitTarget() const;
	ECombatState GetCombatState() const;
	bool IsWeaponEquipped() const;
	bool IsAiming() const;
	bool IsLocallyReloading() const;

	// �����ѿ� �ܾƿ�
	UFUNCTION(BlueprintImplementableEvent)
	void ShowSniperScopeWidget(bool bShowScope);

	// ���� ó��
	UFUNCTION(NetMulticast, Reliable)
	void MulticastElim(bool bPlayerLeftGame);
	void Elim(bool bPlayerLeftGame);
	// ���� ��忡�� �÷��̾� ���� ó�� ��û
	UFUNCTION(Server, Reliable)
	void ServerLeaveGame();

	// ���α��� �����ؼ� �հ��� ��°� ��� Ŭ��� ������ �ٺ����ϴϱ� ��Ƽĳ��Ʈ
	UFUNCTION(NetMulticast, Reliable)
	void MulticastGainedTheLead();
	// ���α� ����
	UFUNCTION(NetMulticast, Reliable)
	void MulticastLostTheLead();

	// �ִϸ��̼� ����
	void PlayFireMontage(bool bAiming);
	void PlayReloadMontage();
	void PlayHitReactMontage();
	void PlayElimMontage();
	void PlayThrowGrenadeMontage();
	void PlaySwapMontage();

	// ���� ��ħ
	void SetOverlappingWeapon(AWeapon* Weapon);

	// HUD ������Ʈ
	void UpdateHUDHealth();
	void UpdateHUDShield();

	// �⺻ ���� ����
	void SpawnDefaultWeapon();

	// ���, �� ����
	ETeam GetTeam();
	void SetTeamColor(ETeam Team);
	bool IsHoldingTheFlag() const;
	void SetHoldingTheFlag(bool bHolding);

private:
	// �ʱ�ȭ
	void PollInit();
	void InitPlayerController();
	void OnPlayerStateInitialized();
	void SetSpawnPoint();
	void UpdateHUDAmmo();

	// ���� ���� �Լ�
	void MoveForward(const FInputActionValue& Value);
	void MoveRight(const FInputActionValue& Value);
	void Turn(const FInputActionValue& Value);
	void LookUp(const FInputActionValue& Value);
	void EquipButtonPressed();
	UFUNCTION(Server, Reliable)
	void ServerEquipButtonPressed();
	void CrouchButtonPressed();
	void AimButtonPressed();
	void AimButtonReleased();
	void FireButtonPressed();
	void FireButtonReleased();
	void ReloadButtonPressed();
	void ThrowGrenadeButtonPressed();

	// ������ ó��
	UFUNCTION()
	void ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatorController, AActor* DamageCauser);

	// ���� ó��
	void StartDissolve();
	UFUNCTION()
	void UpdateDissolveMaterial(float DissolveValue);
	void ElimTimerFinished();

	// ����
	void AimOffset(float DeltaTime);
	double CaculateSpeed() const;
	void CalculateAO_Pitch();

	// ȸ��
	void RotateInPlace(float DeltaTime);
	void SimProxiesTurn();
	void TurnInPlace(float DeltaTime);

	// ī�޶�� ĳ���Ͱ� ��������� ĳ���� �����
	void HideCameraIfCharacterClose();
	
	// ���� ���
	void DropOrDestroyWeapon(AWeapon* Weapon);
	void DropOrDestroyWeapons();

public:
	// ĳ���Ͱ� ������ ������ �� ����� ��������Ʈ
	FOnLeftGame OnLeftGame;

private:
	// ī�޶� ����
	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;
	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<UCameraComponent> FollowCamera;
	UPROPERTY(EditAnywhere, Category = "Camera")
	double CameraThreshlod = 200.0;

	// �߰� ������Ʈ
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UWidgetComponent> OverheadWidget;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCombatComponent> Combat;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UBuffComponent> Buff;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<ULagCompensationComponent> LagCompensation;

	// ���� ���� ����
	// ó�� ������� ����
	UPROPERTY(EditAnywhere, Category = "Combat")
	TSubclassOf<AWeapon> DefaultWeaponClass;
	// ĳ���Ϳ� ��ģ ����
	UPROPERTY(ReplicatedUsing = OnRep_OverlappingWeapon)
	TObjectPtr<AWeapon> OverlappingWeapon;
	UFUNCTION()
	void OnRep_OverlappingWeapon(AWeapon* LastWeapon);
	UPROPERTY(VisibleAnywhere, Category = "Grenade")
	TObjectPtr<UStaticMeshComponent> AttachedGrenade;
	bool bFinishedSwapping = false;

	// ���� �ܾƿ� ���� ����
	float AO_Yaw;
	float InterpAO_Yaw;
	float AO_Pitch;
	FRotator StartingAimRotation;

	// ȸ�� ���� ����
	ETurningInPlace TurningInPlace;
	bool bRotateRootBone;
	double TurnThreshold = 0.5;
	FRotator ProxyRotationLastFrame;
	FRotator ProxyRotation;
	double ProxyYaw;
	float TimeSinceLastMovementReplication;

	// �ִϸ��̼�
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
	UPROPERTY(EditAnywhere, Category = "Combat")
	TObjectPtr<UAnimMontage> SwapMontage;

	// �ɷ�ġ
	UPROPERTY(EditAnywhere, Category = "Player Stats")
	float MaxHealth = 100.f;
	UPROPERTY(ReplicatedUsing = OnRep_Health, VisibleAnywhere, Category = "Player Stats")
	float Health = 100.f;
	UFUNCTION()
	void OnRep_Health(float LastHealth);
	UPROPERTY(EditAnywhere, Category = "Player Stats")
	float MaxShield = 100.f;
	UPROPERTY(ReplicatedUsing = OnRep_Shield, EditAnywhere, Category = "Player Stats")
	float Shield = 0.f;
	UFUNCTION()
	void OnRep_Shield(float LastShield);

	// ���� ���� ����
	UPROPERTY(EditDefaultsOnly, Category = "Elim")
	float ElimDelay = 3.f;
	UPROPERTY(VisibleAnywhere, Category = "Elim")
	TObjectPtr<UTimelineComponent> DissolveTimeline;
	UPROPERTY(EditAnywhere, Category = "Elim")
	TObjectPtr<UCurveFloat> DissolveCurve;
	UPROPERTY(VisibleAnywhere, Category = "Elim")
	TObjectPtr<UMaterialInstanceDynamic> DynamicDissolveMaterialInstance;
	UPROPERTY(VisibleAnywhere, Category = "Elim")
	TObjectPtr<UMaterialInstance> DissolveMaterialInstance;
	UPROPERTY(EditAnywhere, Category = "Elim")
	TObjectPtr<UParticleSystem> ElimBotEffect;
	UPROPERTY(VisibleAnywhere, Category = "Elim")
	TObjectPtr<UParticleSystemComponent> ElimBotComponent;
	UPROPERTY(EditAnywhere, Category = "Elim")
	TObjectPtr<USoundCue> ElimBotSound;
	// ��ٿ� ������ �� Ư�� �Է��� ���� ����
	UPROPERTY(Replicated)
	bool bDisableGameplay = false;
	bool bElimmed = false;
	bool bLeftGame = false;
	FTimerHandle ElimTimer;
	FOnTimelineFloat DissolveTrack;

	// 1���� �� ǥ���� ����Ʈ
	UPROPERTY(EditAnywhere)
	TObjectPtr<UNiagaraSystem> CrownSystem;
	UPROPERTY(EditAnywhere, Category = "Team")
	TObjectPtr<UMaterialInstance> RedMaterial;
	UPROPERTY(EditAnywhere, Category = "Team")
	TObjectPtr<UMaterialInstance> BlueMaterial;
	UPROPERTY(EditAnywhere, Category = "Team")
	TObjectPtr<UMaterialInstance> OriginalMaterial;
	UPROPERTY(EditAnywhere, Category = "Team")
	TObjectPtr<UMaterialInstance> RedDissolveMaterialInstance;
	UPROPERTY(EditAnywhere, Category = "Team")
	TObjectPtr<UMaterialInstance> BlueDissolveMaterialInstance;

	// ���� �ǰ��⿡�� ����� ��Ʈ�ڽ�
	// ���� �ǰ��⸦ ���� ��Ʈ �ڽ���. ���� ���� �̸��� �����ϰ� ��
	UPROPERTY()
	TMap<FName, UBoxComponent*> HitCollisionBoxes;
	UPROPERTY(EditAnywhere, Category = "Hit Box")
	TObjectPtr<UBoxComponent> head;
	UPROPERTY(EditAnywhere, Category = "Hit Box")
	TObjectPtr<UBoxComponent> pelvis;
	UPROPERTY(EditAnywhere, Category = "Hit Box")
	TObjectPtr<UBoxComponent> spine_02;
	UPROPERTY(EditAnywhere, Category = "Hit Box")
	TObjectPtr<UBoxComponent> spine_03;
	UPROPERTY(EditAnywhere, Category = "Hit Box")
	TObjectPtr<UBoxComponent> upperarm_l;
	UPROPERTY(EditAnywhere, Category = "Hit Box")
	TObjectPtr<UBoxComponent> upperarm_r;
	UPROPERTY(EditAnywhere, Category = "Hit Box")
	TObjectPtr<UBoxComponent> lowerarm_l;
	UPROPERTY(EditAnywhere, Category = "Hit Box")
	TObjectPtr<UBoxComponent> lowerarm_r;
	UPROPERTY(EditAnywhere, Category = "Hit Box")
	TObjectPtr<UBoxComponent> hand_l;
	UPROPERTY(EditAnywhere, Category = "Hit Box")
	TObjectPtr<UBoxComponent> hand_r;
	UPROPERTY(EditAnywhere, Category = "Hit Box")
	TObjectPtr<UBoxComponent> backpack;
	UPROPERTY(EditAnywhere, Category = "Hit Box")
	TObjectPtr<UBoxComponent> blanket;
	UPROPERTY(EditAnywhere, Category = "Hit Box")
	TObjectPtr<UBoxComponent> thigh_l;
	UPROPERTY(EditAnywhere, Category = "Hit Box")
	TObjectPtr<UBoxComponent> thigh_r;
	UPROPERTY(EditAnywhere, Category = "Hit Box")
	TObjectPtr<UBoxComponent> calf_l;
	UPROPERTY(EditAnywhere, Category = "Hit Box")
	TObjectPtr<UBoxComponent> calf_r;
	UPROPERTY(EditAnywhere, Category = "Hit Box")
	TObjectPtr<UBoxComponent> foot_l;
	UPROPERTY(EditAnywhere, Category = "Hit Box")
	TObjectPtr<UBoxComponent> foot_r;

	// �Է� ���� ����
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

	// ���� ����
	UPROPERTY()
	TObjectPtr<ABlasterPlayerController> BlasterPlayerController;
	UPROPERTY()
	TObjectPtr<ABlasterPlayerState> BlasterPlayerState;
	UPROPERTY()
	TObjectPtr<ABlasterGameMode> BlasterGameMode;
	UPROPERTY()
	TObjectPtr<UNiagaraComponent> CrownComponent;
};
