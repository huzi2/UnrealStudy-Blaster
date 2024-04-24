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

// 캐릭터가 게임을 나갔을 때 사용할 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLeftGame);

/**
 * 플레이어 캐릭터 클래스
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

	// 저격총용 줌아웃
	UFUNCTION(BlueprintImplementableEvent)
	void ShowSniperScopeWidget(bool bShowScope);

	// 죽음 처리
	UFUNCTION(NetMulticast, Reliable)
	void MulticastElim(bool bPlayerLeftGame);
	void Elim(bool bPlayerLeftGame);
	// 게임 모드에게 플레이어 나감 처리 요청
	UFUNCTION(Server, Reliable)
	void ServerLeaveGame();

	// 선두권을 차지해서 왕관을 얻는건 모든 클라와 서버가 다봐야하니까 멀티캐스트
	UFUNCTION(NetMulticast, Reliable)
	void MulticastGainedTheLead();
	// 선두권 잃음
	UFUNCTION(NetMulticast, Reliable)
	void MulticastLostTheLead();

	// 애니메이션 실행
	void PlayFireMontage(bool bAiming);
	void PlayReloadMontage();
	void PlayHitReactMontage();
	void PlayElimMontage();
	void PlayThrowGrenadeMontage();
	void PlaySwapMontage();

	// 무기 겹침
	void SetOverlappingWeapon(AWeapon* Weapon);

	// HUD 업데이트
	void UpdateHUDHealth();
	void UpdateHUDShield();

	// 기본 무기 스폰
	void SpawnDefaultWeapon();

	// 깃발, 팀 관련
	ETeam GetTeam();
	void SetTeamColor(ETeam Team);
	bool IsHoldingTheFlag() const;
	void SetHoldingTheFlag(bool bHolding);

private:
	// 초기화
	void PollInit();
	void InitPlayerController();
	void OnPlayerStateInitialized();
	void SetSpawnPoint();
	void UpdateHUDAmmo();

	// 조작 관련 함수
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

	// 데미지 처리
	UFUNCTION()
	void ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatorController, AActor* DamageCauser);

	// 죽음 처리
	void StartDissolve();
	UFUNCTION()
	void UpdateDissolveMaterial(float DissolveValue);
	void ElimTimerFinished();

	// 조준
	void AimOffset(float DeltaTime);
	double CaculateSpeed() const;
	void CalculateAO_Pitch();

	// 회전
	void RotateInPlace(float DeltaTime);
	void SimProxiesTurn();
	void TurnInPlace(float DeltaTime);

	// 카메라와 캐릭터가 가까워지면 캐릭터 숨기기
	void HideCameraIfCharacterClose();
	
	// 무기 드랍
	void DropOrDestroyWeapon(AWeapon* Weapon);
	void DropOrDestroyWeapons();

public:
	// 캐릭터가 게임을 나갔을 때 사용할 델리게이트
	FOnLeftGame OnLeftGame;

private:
	// 카메라 변수
	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;
	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<UCameraComponent> FollowCamera;
	UPROPERTY(EditAnywhere, Category = "Camera")
	double CameraThreshlod = 200.0;

	// 추가 컴포넌트
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UWidgetComponent> OverheadWidget;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCombatComponent> Combat;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UBuffComponent> Buff;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<ULagCompensationComponent> LagCompensation;

	// 무기 관련 변수
	// 처음 들고있을 무기
	UPROPERTY(EditAnywhere, Category = "Combat")
	TSubclassOf<AWeapon> DefaultWeaponClass;
	// 캐릭터와 겹친 무기
	UPROPERTY(ReplicatedUsing = OnRep_OverlappingWeapon)
	TObjectPtr<AWeapon> OverlappingWeapon;
	UFUNCTION()
	void OnRep_OverlappingWeapon(AWeapon* LastWeapon);
	UPROPERTY(VisibleAnywhere, Category = "Grenade")
	TObjectPtr<UStaticMeshComponent> AttachedGrenade;
	bool bFinishedSwapping = false;

	// 줌인 줌아웃 관련 변수
	float AO_Yaw;
	float InterpAO_Yaw;
	float AO_Pitch;
	FRotator StartingAimRotation;

	// 회전 관련 변수
	ETurningInPlace TurningInPlace;
	bool bRotateRootBone;
	double TurnThreshold = 0.5;
	FRotator ProxyRotationLastFrame;
	FRotator ProxyRotation;
	double ProxyYaw;
	float TimeSinceLastMovementReplication;

	// 애니메이션
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

	// 능력치
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

	// 죽음 관련 변수
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
	// 쿨다운 상태일 때 특정 입력을 막기 위함
	UPROPERTY(Replicated)
	bool bDisableGameplay = false;
	bool bElimmed = false;
	bool bLeftGame = false;
	FTimerHandle ElimTimer;
	FOnTimelineFloat DissolveTrack;

	// 1등일 때 표시할 이펙트
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

	// 서버 되감기에서 사용할 히트박스
	// 서버 되감기를 위한 히트 박스들. 붙일 본의 이름과 동일하게 함
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

	// 입력 관련 변수
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

	// 참조 변수
	UPROPERTY()
	TObjectPtr<ABlasterPlayerController> BlasterPlayerController;
	UPROPERTY()
	TObjectPtr<ABlasterPlayerState> BlasterPlayerState;
	UPROPERTY()
	TObjectPtr<ABlasterGameMode> BlasterGameMode;
	UPROPERTY()
	TObjectPtr<UNiagaraComponent> CrownComponent;
};
