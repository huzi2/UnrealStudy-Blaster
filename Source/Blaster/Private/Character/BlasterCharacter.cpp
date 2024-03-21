// Fill out your copyright notice in the Description page of Project Settings.

#include "Character/BlasterCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/WidgetComponent.h"
#include "Net/UnrealNetwork.h"
#include "Weapon/Weapon.h"
#include "BlasterComponents/CombatComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Character/BlasterAnimInstance.h"
#include "Blaster/Blaster.h"
#include "PlayerController/BlasterPlayerController.h"
#include "GameMode/BlasterGameMode.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"
#include "Particles/ParticleSystemComponent.h"
#include "PlayerState/BlasterPlayerState.h"
#include "Weapon/WeaponTypes.h"
#include "BlasterComponents/BuffComponent.h"
#include "BlasterComponents/LagCompensationComponent.h"
#include "Components/BoxComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "InputActionValue.h"

ABlasterCharacter::ABlasterCharacter()
	: CameraThreshlod(200.0)
	, MaxHealth(100.f)
	, Health(100.f)
	, MaxShield(100.f)
	, Shield(0.f)
	, ElimDelay(3.f)
	, bDisableGameplay(false)
	, TurnThreshold(0.5f)
	, bElimmed(false)
	, bFinishedSwapping(false)
	, bLeftGame(false)
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	// 리스폰 시 겹치더라도 무조건 스폰
	SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetMesh());
	CameraBoom->TargetArmLength = 600.f;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;

	OverheadWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("OverheadWidget"));
	OverheadWidget->SetupAttachment(GetRootComponent());

	Combat = CreateDefaultSubobject<UCombatComponent>(TEXT("Combat"));
	Combat->SetIsReplicated(true);

	Buff = CreateDefaultSubobject<UBuffComponent>(TEXT("Buff"));
	Buff->SetIsReplicated(true);

	LagCompensation = CreateDefaultSubobject<ULagCompensationComponent>(TEXT("LagCompensation"));

	DissolveTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("DissolveTimeline"));

	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0, 0.0, 850.0);

	// 카메라가 메쉬나 캡슐과 충돌해서 줌인되는 것을 방지
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);
	// 캡슐이 아닌 메시가 충돌하도록(헤드샷 등의 구현을 위해) 커스텀 오브젝트 채널로 설정
	GetMesh()->SetCollisionObjectType(ECC_SkeletalMesh);

	TurningInPlace = ETurningInPlace::ETIP_NotTurning;

	AttachedGrenade = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AttachedGrenade"));
	AttachedGrenade->SetupAttachment(GetMesh(), TEXT("GrenadeSocket"));
	AttachedGrenade->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	HitCollisionBoxes.Reserve(18);

	head = CreateDefaultSubobject<UBoxComponent>("head");
	head->SetupAttachment(GetMesh(), TEXT("head"));
	HitCollisionBoxes.Add(TEXT("head"), head);

	pelvis = CreateDefaultSubobject<UBoxComponent>("pelvis");
	pelvis->SetupAttachment(GetMesh(), TEXT("pelvis"));
	HitCollisionBoxes.Add(TEXT("pelvis"), pelvis);

	spine_02 = CreateDefaultSubobject<UBoxComponent>("spine_02");
	spine_02->SetupAttachment(GetMesh(), TEXT("spine_02"));
	HitCollisionBoxes.Add(TEXT("spine_02"), spine_02);

	spine_03 = CreateDefaultSubobject<UBoxComponent>("spine_03");
	spine_03->SetupAttachment(GetMesh(), TEXT("spine_03"));
	HitCollisionBoxes.Add(TEXT("spine_03"), spine_03);

	upperarm_l = CreateDefaultSubobject<UBoxComponent>("upperarm_l");
	upperarm_l->SetupAttachment(GetMesh(), TEXT("upperarm_l"));
	HitCollisionBoxes.Add(TEXT("upperarm_l"), upperarm_l);

	upperarm_r = CreateDefaultSubobject<UBoxComponent>("upperarm_r");
	upperarm_r->SetupAttachment(GetMesh(), TEXT("upperarm_r"));
	HitCollisionBoxes.Add(TEXT("upperarm_r"), upperarm_r);

	lowerarm_l = CreateDefaultSubobject<UBoxComponent>("lowerarm_l");
	lowerarm_l->SetupAttachment(GetMesh(), TEXT("lowerarm_l"));
	HitCollisionBoxes.Add(TEXT("lowerarm_l"), lowerarm_l);

	lowerarm_r = CreateDefaultSubobject<UBoxComponent>("lowerarm_r");
	lowerarm_r->SetupAttachment(GetMesh(), TEXT("lowerarm_r"));
	HitCollisionBoxes.Add(TEXT("lowerarm_r"), lowerarm_r);

	hand_l = CreateDefaultSubobject<UBoxComponent>("hand_l");
	hand_l->SetupAttachment(GetMesh(), TEXT("hand_l"));
	HitCollisionBoxes.Add(TEXT("hand_l"), hand_l);

	hand_r = CreateDefaultSubobject<UBoxComponent>("hand_r");
	hand_r->SetupAttachment(GetMesh(), TEXT("hand_r"));
	HitCollisionBoxes.Add(TEXT("hand_r"), hand_r);

	backpack = CreateDefaultSubobject<UBoxComponent>("backpack");
	backpack->SetupAttachment(GetMesh(), TEXT("backpack"));
	HitCollisionBoxes.Add(TEXT("backpack"), backpack);

	blanket = CreateDefaultSubobject<UBoxComponent>("blanket");
	blanket->SetupAttachment(GetMesh(), TEXT("backpack"));
	HitCollisionBoxes.Add(TEXT("blanket"), blanket);

	thigh_l = CreateDefaultSubobject<UBoxComponent>("thigh_l");
	thigh_l->SetupAttachment(GetMesh(), TEXT("thigh_l"));
	HitCollisionBoxes.Add(TEXT("thigh_l"), thigh_l);

	thigh_r = CreateDefaultSubobject<UBoxComponent>("thigh_r");
	thigh_r->SetupAttachment(GetMesh(), TEXT("thigh_r"));
	HitCollisionBoxes.Add(TEXT("thigh_r"), thigh_r);

	calf_l = CreateDefaultSubobject<UBoxComponent>("calf_l");
	calf_l->SetupAttachment(GetMesh(), TEXT("calf_l"));
	HitCollisionBoxes.Add(TEXT("calf_l"), calf_l);

	calf_r = CreateDefaultSubobject<UBoxComponent>("calf_r");
	calf_r->SetupAttachment(GetMesh(), TEXT("calf_r"));
	HitCollisionBoxes.Add(TEXT("calf_r"), calf_r);

	foot_l = CreateDefaultSubobject<UBoxComponent>("foot_l");
	foot_l->SetupAttachment(GetMesh(), TEXT("foot_l"));
	HitCollisionBoxes.Add(TEXT("foot_l"), foot_l);

	foot_r = CreateDefaultSubobject<UBoxComponent>("foot_r");
	foot_r->SetupAttachment(GetMesh(), TEXT("foot_r"));
	HitCollisionBoxes.Add(TEXT("foot_r"), foot_r);

	for (auto& Box : HitCollisionBoxes)
	{
		if (Box.Value)
		{
			Box.Value->SetCollisionObjectType(ECC_HitBox);
			Box.Value->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
			Box.Value->SetCollisionResponseToChannel(ECC_HitBox, ECollisionResponse::ECR_Block);
			Box.Value->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}

	NetUpdateFrequency = 66.f;
	MinNetUpdateFrequency = 33.f;
}

void ABlasterCharacter::BeginPlay()
{
	Super::BeginPlay();

	SpawnDefaultWeapon();
	UpdateHUDAmmo();

	// 게임 시작할 때 체력 초기화
	// 하지만 게임 중에 캐릭터가 제거되고 재생성될 때는 컨트롤러가 연결되지 않은 상태에서 생성되서 이부분이 호출안될 수도 있다.
	// 그래서 게임 중 재생성에 대해서는 컨트롤러의 OnPossess()에서 처리한다.
	// 근데 여기에 이 함수를 남겨두는 건 최초 실행할 때는 컨트롤러보다 UI가 나중에 생성되면 OnPossess()가 의미가 없기 때문
	UpdateHUDHealth();
	UpdateHUDShield();

	// 데미지 처리는 서버만
	if (HasAuthority())
	{
		OnTakeAnyDamage.AddDynamic(this, &ThisClass::ReceiveDamage);
	}

	if (AttachedGrenade)
	{
		AttachedGrenade->SetVisibility(false);
	}
}

void ABlasterCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (Combat)
	{
		Combat->Character = this;
	}

	if (Buff)
	{
		Buff->Character = this;

		if (GetCharacterMovement())
		{
			Buff->SetInitialSpeeds(GetCharacterMovement()->MaxWalkSpeed, GetCharacterMovement()->MaxWalkSpeedCrouched);
			Buff->SetInitialJumpVelocity(GetCharacterMovement()->JumpZVelocity);
		}
	}

	if (LagCompensation)
	{
		LagCompensation->Character = this;
		LagCompensation->Controller = Cast<ABlasterPlayerController>(Controller);
	}
}

void ABlasterCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 플레이어스테이트의 초기화 타이밍이 애매해서 Tick에서 초기화함
	// 내부에서 플레이어스테이트가 최초로 생성될 때만 초기화하도록 해놨음
	PollInit();

	// 스스로 제어하거나 서버일 경우 에임따라 상체가 움직임
	RotateInPlace(DeltaTime);
	
	// 카메라와 캐릭터가 너무 가까워지면 캐릭터를 숨김
	HideCameraIfCharacterClose();
}

void ABlasterCharacter::Destroyed()
{
	Super::Destroyed();

	// 원래 기존 제거 타이머가 끝나면 호출한다. 하지만 이러면 서버에서만 제거됨
	// 클라이언트에서는 캐릭터와 같이 제거하도록함
	if (ElimBotComponent)
	{
		ElimBotComponent->DestroyComponent();
	}

	// 매치 상태가 아닐 때 제거되면 무기도 같이 제거
	ABlasterGameMode* BlasterGameMode = GetWorld()->GetAuthGameMode<ABlasterGameMode>();
	if (BlasterGameMode && BlasterGameMode->GetMatchState() != MatchState::InProgress)
	{
		if (Combat && Combat->EquippedWeapon)
		{
			Combat->EquippedWeapon->Destroy();
		}
	}
}

void ABlasterCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		Input->BindAction(JumpInputAction, ETriggerEvent::Triggered, this, &ThisClass::Jump);
		Input->BindAction(MoveForwardInputAction, ETriggerEvent::Triggered, this, &ThisClass::MoveForward);
		Input->BindAction(MoveRightInputAction, ETriggerEvent::Triggered, this, &ThisClass::MoveRight);
		Input->BindAction(TurnInputAction, ETriggerEvent::Triggered, this, &ThisClass::Turn);
		Input->BindAction(LookUpInputAction, ETriggerEvent::Triggered, this, &ThisClass::LookUp);
		Input->BindAction(EquipInputAction, ETriggerEvent::Started, this, &ThisClass::EquipButtonPressed);
		Input->BindAction(CrouchInputAction, ETriggerEvent::Started, this, &ThisClass::CrouchButtonPressed);
		Input->BindAction(AimInputAction, ETriggerEvent::Started, this, &ThisClass::AimButtonPressed);
		Input->BindAction(AimInputAction, ETriggerEvent::Completed, this, &ThisClass::AimButtonReleased);
		Input->BindAction(FireInputAction, ETriggerEvent::Started, this, &ThisClass::FireButtonPressed);
		Input->BindAction(FireInputAction, ETriggerEvent::Completed, this, &ThisClass::FireButtonReleased);
		Input->BindAction(ReloadInputAction, ETriggerEvent::Started, this, &ThisClass::ReloadButtonPressed);
		Input->BindAction(ThrowGrenadeInputAction, ETriggerEvent::Started, this, &ThisClass::ThrowGrenadeButtonPressed);
	}
}

// 레플리케이트 될 때 호출되는 함수
void ABlasterCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// COND_OwnerOnly은 객체 소유자만 레플리케이트 되도록함
	DOREPLIFETIME_CONDITION(ABlasterCharacter, OverlappingWeapon, COND_OwnerOnly);

	DOREPLIFETIME(ABlasterCharacter, Health);
	DOREPLIFETIME(ABlasterCharacter, Shield);
	DOREPLIFETIME(ABlasterCharacter, bDisableGameplay);
}

void ABlasterCharacter::Jump()
{
	if (bDisableGameplay) return;

	if (bIsCrouched)
	{
		UnCrouch();
	}
	else
	{
		Super::Jump();
	}
}

void ABlasterCharacter::OnRep_ReplicatedMovement()
{
	// 매 틱마다 넷업데이트가 이루어지지않는다.
	// 그래서 이동에 관련된 틱 관련 계산은 이곳에서 해서 무브먼트가 갱신될 때마다 적용시킬 수 있다.
	Super::OnRep_ReplicatedMovement();

	// Tick에서 서버와 자신의 클라는 에임따라 상체가 따로 움직이지만
	// 나머지 클라들은 루트본은 움직이지 않고 에임따라 전체 회전 시킨다.
	// 무브먼트와 관련된 계산이므로 Tick에서 하는게 아니라 여기서 진행
	SimProxiesTurn();
	TimeSinceLastMovementReplication = 0.f;
}

void ABlasterCharacter::MulticastElim_Implementation(bool bPlayerLeftGame)
{
	bLeftGame = bPlayerLeftGame;

	// 죽음 처리 자체는 게임모드에서 서버가 처리해줌
	// 죽으면서 수행해야하는 작업은 모든 클라가 해야되서 멀티캐스트로 함수 생성
	bElimmed = true;
	PlayElimMontage();

	// 탄약 초기화
	InitPlayerController();
	if (BlasterPlayerController)
	{
		BlasterPlayerController->SetHUDWeaponAmmo(0);
	}

	// 죽을 때 사라지는 이펙트를 위해 머티리얼 인스턴스를 생성 후 적용
	if (GetMesh() && DissolveMaterialInstance)
	{
		DynamicDissolveMaterialInstance = UMaterialInstanceDynamic::Create(DissolveMaterialInstance, this);
		GetMesh()->SetMaterial(0, DynamicDissolveMaterialInstance);
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), 0.55f);
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Glow"), 200.f);
	}

	StartDissolve();

	// 무브먼트 제거
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->DisableMovement();
		GetCharacterMovement()->StopMovementImmediately();
	}

	// 특정 행동을 못하게 막는다.
	bDisableGameplay = true;
	if (Combat)
	{
		// 무기가 오토일 때 자동발사를 막음
		Combat->FireButtonPressed(false);
	}

	// 충돌 제거
	if (GetCapsuleComponent())
	{
		GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	if (GetMesh())
	{
		GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	// 머리에 로봇 생성 효과
	if (ElimBotEffect)
	{
		const FVector ElimBotSpawnPoint(GetActorLocation().X, GetActorLocation().Y, GetActorLocation().Z + 200.0);
		ElimBotComponent = UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ElimBotEffect, ElimBotSpawnPoint, GetActorRotation());
	}

	if (ElimBotSound)
	{
		UGameplayStatics::SpawnSoundAtLocation(this, ElimBotSound, GetActorLocation());
	}

	// 스나이퍼 줌하고 있는 상태였을 때 줌아웃하도록함
	if (IsLocallyControlled() && Combat && Combat->bAiming && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponType() == EWeaponType::EWT_SniperRifle)
	{
		ShowSniperScopeWidget(false);
	}

	// 캐릭터 부활을 위해 타이머 설정
	GetWorldTimerManager().SetTimer(ElimTimer, this, &ThisClass::ElimTimerFinished, ElimDelay);
}

void ABlasterCharacter::ServerLeaveGame_Implementation()
{
	if (!GetWorld()) return;

	PollInit();

	if (!BlasterPlayerState) return;

	// 서버의 게임모드에 플레이어 나감 처리 요청
	if (ABlasterGameMode* BlasterGameMode = GetWorld()->GetAuthGameMode<ABlasterGameMode>())
	{
		BlasterGameMode->PlayerLeftGame(BlasterPlayerState);
	}
}

void ABlasterCharacter::ServerEquipButtonPressed_Implementation()
{
	if (Combat)
	{
		// 무기와 겹친 상태면 그 무기 장착
		if (OverlappingWeapon)
		{
			Combat->EquipWeapon(OverlappingWeapon);
		}
		// 아닌 상태면 보조 무기와 스왑
		else if (Combat->ShouldSwapWeapons())
		{
			Combat->SwapWeapons();
		}
	}
}

void ABlasterCharacter::ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatorController, AActor* DamageCauser)
{
	if (bElimmed) return;

	// 데미지 처리는 서버에서만 수행됨. 여기 내용들은 서버에서 실행된다.
	float DamageToHealth = Damage;
	if (Shield > 0.f)
	{
		if (Shield >= Damage)
		{
			Shield = FMath::Clamp(Shield - Damage, 0.f, MaxShield);
			DamageToHealth = 0.f;
		}
		else
		{
			Shield = 0.f;
			DamageToHealth = FMath::Clamp(DamageToHealth - Shield, 0.f, Damage);
		}
	}

	Health = FMath::Clamp(Health - DamageToHealth, 0.f, MaxHealth);

	UpdateHUDHealth();
	UpdateHUDShield();
	PlayHitReactMontage();

	// 체력이 0이되면 게임모드에서 캐릭터 제거
	// 게임모드는 오로지 서버에만 존재하기에 클라에서 호출하면 null이다.
	// 어차피 데미지 처리 자체가 서버에서만 호출되기는함..
	if (Health == 0.f)
	{
		if (GetWorld())
		{
			if (ABlasterGameMode* BlasterGameMode = GetWorld()->GetAuthGameMode<ABlasterGameMode>())
			{
				ABlasterPlayerController* AttackerController = Cast<ABlasterPlayerController>(InstigatorController);
				InitPlayerController();
				BlasterGameMode->PlayerEliminated(this, BlasterPlayerController, AttackerController);
			}
		}
	}
}

void ABlasterCharacter::UpdateDissolveMaterial(float DissolveValue)
{
	if (DynamicDissolveMaterialInstance)
	{
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), DissolveValue);
	}
}

AWeapon* ABlasterCharacter::GetEquippedWeapon() const
{
	if (!Combat) return nullptr;
	return Combat->EquippedWeapon;
}

FVector ABlasterCharacter::GetHitTarget() const
{
	if(!Combat) return FVector();
	return Combat->HitTarget;
}

ECombatState ABlasterCharacter::GetCombatState() const
{
	if (!Combat) return ECombatState::ECS_MAX;
	return Combat->CombatState;
}

void ABlasterCharacter::SetOverlappingWeapon(AWeapon* Weapon)
{
	// 충돌 처리는 서머만 하도록 해서 이 함수는 서버만 들어옴
	// 클라들은 레플리케이트 처리되지만 서버는 여기서 처리
	if (OverlappingWeapon)
	{
		OverlappingWeapon->ShowPickupWidget(false);
	}

	OverlappingWeapon = Weapon;
	if (IsLocallyControlled())
	{
		if (OverlappingWeapon)
		{
			OverlappingWeapon->ShowPickupWidget(true);
		}
	}
}

bool ABlasterCharacter::IsWeaponEquipped() const
{
	return Combat && Combat->EquippedWeapon;
}

bool ABlasterCharacter::IsAiming() const
{
	return Combat && Combat->bAiming;
}

void ABlasterCharacter::PlayFireMontage(bool bAiming)
{
	if (!Combat) return;
	if (!Combat->EquippedWeapon) return;
	if (!GetMesh()) return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && FireWeaponMontage)
	{
		AnimInstance->Montage_Play(FireWeaponMontage);
		const FName SectionName = bAiming ? TEXT("RifleAim") : TEXT("RifleHip");
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void ABlasterCharacter::PlayReloadMontage()
{
	if (!Combat) return;
	if (!Combat->EquippedWeapon) return;
	if (!GetMesh()) return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && ReloadMontage)
	{
		AnimInstance->Montage_Play(ReloadMontage);

		// 무기에 따라 다른 섹션 사용
		FName SectionName = TEXT("Rifle");
		switch (Combat->EquippedWeapon->GetWeaponType())
		{
		case EWeaponType::EWT_AssaultRifle:
			SectionName = TEXT("Rifle");
			break;
		case EWeaponType::EWT_RocketLauncher:
			SectionName = TEXT("RocketLauncher");
			break;
		case EWeaponType::EWT_Pistol:
			SectionName = TEXT("Pistol");
			break;
		case EWeaponType::EWT_SubmachineGun:
			SectionName = TEXT("Pistol");
			break;
		case EWeaponType::EWT_ShotGun:
			SectionName = TEXT("ShotGun");
			break;
		case EWeaponType::EWT_SniperRifle:
			SectionName = TEXT("SniperRifle");
			break;
		case EWeaponType::EWT_GrenaderLauncher:
			SectionName = TEXT("GrenaderLauncher");
			break;
		default:
			break;
		}

		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void ABlasterCharacter::PlayHitReactMontage()
{
	if (!Combat) return;
	if (!Combat->EquippedWeapon) return;
	if (!GetMesh()) return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && HitReactMontage)
	{
		AnimInstance->Montage_Play(HitReactMontage);
		//const FName SectionName = bAiming ? TEXT("RifleAim") : TEXT("RifleHip");
		//AnimInstance->Montage_JumpToSection(SectionName);
		AnimInstance->Montage_JumpToSection(TEXT("FromFront"));
	}
}

void ABlasterCharacter::PlayElimMontage()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && ElimMontage)
	{
		AnimInstance->Montage_Play(ElimMontage);
	}
}

void ABlasterCharacter::PlayThrowGrenadeMontage()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && ThrowGrenadeMontage)
	{
		AnimInstance->Montage_Play(ThrowGrenadeMontage);
	}
}

void ABlasterCharacter::PlaySwapMontage()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && SwapMontage)
	{
		AnimInstance->Montage_Play(SwapMontage);
	}
}

void ABlasterCharacter::Elim(bool bPlayerLeftGame)
{
	// 캐릭터가 죽는 처리는 서버에서 행한다.
	// 무기를 떨어뜨리는 처리
	DropOrDestroyWeapons();

	// 하지만 죽으면서 애니메이션을 재생하는 등의 행위는 모든 클라가 해야되서 멀티캐스트 함수를 호출한다.
	MulticastElim(bPlayerLeftGame);
}

void ABlasterCharacter::UpdateHUDHealth()
{
	InitPlayerController();
	if (BlasterPlayerController)
	{
		BlasterPlayerController->SetHUDHealth(Health, MaxHealth);
	}
}

void ABlasterCharacter::UpdateHUDShield()
{
	InitPlayerController();
	if (BlasterPlayerController)
	{
		BlasterPlayerController->SetHUDShield(Shield, MaxShield);
	}
}

void ABlasterCharacter::SpawnDefaultWeapon()
{
	if (!GetWorld()) return;
	if (!DefaultWeaponClass) return;
	if (!Combat) return;
	if (bElimmed) return;

	if (ABlasterGameMode* BlasterGameMode = Cast<ABlasterGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		if (AWeapon* StartingWeapon = GetWorld()->SpawnActor<AWeapon>(DefaultWeaponClass))
		{
			StartingWeapon->SetDestroyWeapon(true);
			Combat->EquipWeapon(StartingWeapon);
		}
	}
}

bool ABlasterCharacter::IsLocallyReloading() const
{
	if (!Combat) return false;
	return Combat->GetLocallyReloading();
}

void ABlasterCharacter::MoveForward(const FInputActionValue& Value)
{
	if (bDisableGameplay) return;

	const float MovementValue = Value.Get<float>();

	if (Controller && MovementValue != 0.f)
	{
		const FRotator YawRotation(0.f, GetControlRotation().Yaw, 0.f);
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

		AddMovementInput(Direction, MovementValue);
	}
}

void ABlasterCharacter::MoveRight(const FInputActionValue& Value)
{
	if (bDisableGameplay) return;

	const float MovementValue = Value.Get<float>();

	if (Controller && MovementValue != 0.f)
	{
		const FRotator YawRotation(0.f, GetControlRotation().Yaw, 0.f);
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		AddMovementInput(Direction, MovementValue);
	}
}

void ABlasterCharacter::Turn(const FInputActionValue& Value)
{
	const float MovementValue = Value.Get<float>();

	AddControllerYawInput(MovementValue);
}

void ABlasterCharacter::LookUp(const FInputActionValue& Value)
{
	const float MovementValue = Value.Get<float>();

	AddControllerPitchInput(MovementValue);
}

void ABlasterCharacter::EquipButtonPressed()
{
	if (bDisableGameplay) return;

	if (Combat && Combat->CombatState == ECombatState::ECS_Unoccupied)
	{
		// 서버에게 무기장착 요구. 서버면 바로 장착하고, 클라면 서버를 통해서 장착
		ServerEquipButtonPressed();

		// 키를 누른 로컬 클라에서 호출하는 것. 서버 본인은 SwapWeapons()에서 할 것
		if (!HasAuthority() && Combat->ShouldSwapWeapons() && !OverlappingWeapon)
		{
			PlaySwapMontage();
			Combat->CombatState = ECombatState::ECS_SwappingWeapons;
			bFinishedSwapping = false;
		}
	}
}

void ABlasterCharacter::CrouchButtonPressed()
{
	if (bDisableGameplay) return;

	if (bIsCrouched)
	{
		UnCrouch();
	}
	else
	{
		Crouch();
	}
}

void ABlasterCharacter::AimButtonPressed()
{
	if (bDisableGameplay) return;

	if (Combat)
	{
		Combat->SetAiming(true);
	}
}

void ABlasterCharacter::AimButtonReleased()
{
	if (bDisableGameplay) return;

	if (Combat)
	{
		Combat->SetAiming(false);
	}
}

void ABlasterCharacter::FireButtonPressed()
{
	if (bDisableGameplay) return;

	if (Combat)
	{
		Combat->FireButtonPressed(true);
	}
}

void ABlasterCharacter::FireButtonReleased()
{
	if (bDisableGameplay) return;

	if (Combat)
	{
		Combat->FireButtonPressed(false);
	}
}

void ABlasterCharacter::ReloadButtonPressed()
{
	if (bDisableGameplay) return;

	if (Combat)
	{
		Combat->Reload();
	}
}

void ABlasterCharacter::ThrowGrenadeButtonPressed()
{
	if (bDisableGameplay) return;

	if (Combat)
	{
		Combat->ThrowGrenade();
	}
}

void ABlasterCharacter::InitPlayerController()
{
	if (!BlasterPlayerController)
	{
		if (BlasterPlayerController = Cast<ABlasterPlayerController>(Controller))
		{
			if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(BlasterPlayerController->GetLocalPlayer()))
			{
				Subsystem->AddMappingContext(DefaultInputMappingContext, 0);
			}
		}
	}
}

void ABlasterCharacter::AimOffset(float DeltaTime)
{
	// 무기를 들었을 때만 적용
	if (!Combat) return;
	if (!Combat->EquippedWeapon) return;
	if (!GetCharacterMovement()) return;

	const double Speed = CaculateSpeed();
	const bool bIsInAir = GetCharacterMovement()->IsFalling();

	// 가만히 서 있을때는 상하좌우 다 조준 가능
	if (Speed == 0.0 && !bIsInAir)
	{
		bUseControllerRotationYaw = true;
		// 루트본을 움직여서 하체는 가만히 있고 상체가 바라보는 방향으로 움직이도록 한다.
		bRotateRootBone = true;

		FRotator CurrentAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		FRotator DeltaAimRotation = UKismetMathLibrary::NormalizedDeltaRotator(CurrentAimRotation, StartingAimRotation);
		AO_Yaw = DeltaAimRotation.Yaw;
		// 몸체를 회전할 때 사용하기 위해 보정용 변수 저장
		if (TurningInPlace == ETurningInPlace::ETIP_NotTurning)
		{
			InterpAO_Yaw = AO_Yaw;
		}

		// -90이나 90 끝에 다다랐을 경우 몸 자체를 회전시켜야함
		TurnInPlace(DeltaTime);
	}
	// 뛰거나 점프할 때는 위아래로만 조준 가능
	if (Speed > 0.0 || bIsInAir)
	{
		bUseControllerRotationYaw = true;
		bRotateRootBone = false;

		StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		AO_Yaw = 0.f;

		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
	}

	// 위아래 계산
	CalculateAO_Pitch();
}

void ABlasterCharacter::SimProxiesTurn()
{
	if (!Combat) return;
	if (!Combat->EquippedWeapon) return;

	bRotateRootBone = false;

	const double Speed = CaculateSpeed();
	if (Speed > 0.0)
	{
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		return;
	}

	// 일정 각도 이상 회전하면 회전 애니메이션을 실행
	ProxyRotationLastFrame = ProxyRotation;
	ProxyRotation = GetActorRotation();
	ProxyYaw = UKismetMathLibrary::NormalizedDeltaRotator(ProxyRotation, ProxyRotationLastFrame).Yaw;

	if (FMath::Abs(ProxyYaw) > TurnThreshold)
	{
		if (ProxyYaw > TurnThreshold)
		{
			TurningInPlace = ETurningInPlace::ETIP_Right;
		}
		else if (ProxyYaw < -TurnThreshold)
		{
			TurningInPlace = ETurningInPlace::ETIP_Left;
		}
		else
		{
			TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		}
		return;
	}
	TurningInPlace = ETurningInPlace::ETIP_NotTurning;
}

void ABlasterCharacter::TurnInPlace(float DeltaTime)
{
	// 어디로 회전할 지 정하고 애니메이션 블루프린트에서 확인
	if (AO_Yaw > 90.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_Right;
	}
	else if (AO_Yaw < -90.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_Left;
	}

	// 실제 회전값 수정
	if (TurningInPlace != ETurningInPlace::ETIP_NotTurning)
	{
		// 회전하기위해 보정값을 15도까지 낮춤
		InterpAO_Yaw = FMath::FInterpTo(InterpAO_Yaw, 0.f, DeltaTime, 4.f);
		AO_Yaw = InterpAO_Yaw;

		if (FMath::Abs(AO_Yaw) < 15.f)
		{
			TurningInPlace = ETurningInPlace::ETIP_NotTurning;
			StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		}
	}
}

void ABlasterCharacter::HideCameraIfCharacterClose()
{
	// 본인 클라만 구현하면됨. 다른 클라들은 캐릭터가 숨겨지면 안됨
	if (!IsLocallyControlled()) return;
	if (!FollowCamera) return;
	if (!GetMesh()) return;

	// 카메라가 너무 가까우면
	if ((FollowCamera->GetComponentLocation() - GetActorLocation()).Size() < CameraThreshlod)
	{
		// 메쉬를 숨김
		GetMesh()->SetVisibility(false);

		// 무기도 숨김
		if (Combat && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponMesh())
		{
			// Owner가 못보는 옵션
			Combat->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = true;
		}
	}
	else
	{
		GetMesh()->SetVisibility(true);

		// 무기도 숨김
		if (Combat && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponMesh())
		{
			Combat->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = false;
		}
	}
}

void ABlasterCharacter::CalculateAO_Pitch()
{
	AO_Pitch = GetBaseAimRotation().Pitch;

	// 피치값의 경우 -90 ~ 0도는 서버 패킷으로 전송되는 과정에서 270 ~ 360도로 변경된다.
	// 그래서 자신이 컨트롤하는 캐릭터 외의 캐릭터의 피치값은 보정을 해준다.
	if (AO_Pitch > 90.f && !IsLocallyControlled())
	{
		const FVector2D InRange(270.f, 360.f);
		const FVector2D OutRange(-90.f, 0.f);
		AO_Pitch = FMath::GetMappedRangeValueClamped(InRange, OutRange, AO_Pitch);
	}
}

double ABlasterCharacter::CaculateSpeed() const
{
	FVector Velocity = GetVelocity();
	Velocity.Z = 0.0;
	return Velocity.Size();
}

void ABlasterCharacter::ElimTimerFinished()
{
	// 게임모드에서 캐릭터 부활 요청. 게임모드는 서버에만 존재해서 서버에서만 처리될 것
	if (ABlasterGameMode* BlasterGameMode = GetWorld()->GetAuthGameMode<ABlasterGameMode>())
	{
		// 게임을 나가지 않은 경우에만 리스폰
		if (!bLeftGame)
		{
			BlasterGameMode->RequestRespawn(this, Controller);
		}
	}

	// 머리 위에 로봇 효과 제거
	if (ElimBotComponent)
	{
		ElimBotComponent->DestroyComponent();
	}

	// 게임을 나갔다면 나갔다고 브로드캐스트
	if (bLeftGame && IsLocallyControlled())
	{
		OnLeftGame.Broadcast();
	}
}

void ABlasterCharacter::StartDissolve()
{
	// 캐릭터가 죽고 사라지는 이펙트(머티리얼에 커브값을 줌)
	DissolveTrack.BindDynamic(this, &ThisClass::UpdateDissolveMaterial);
	
	if (DissolveTimeline && DissolveCurve)
	{
		DissolveTimeline->AddInterpFloat(DissolveCurve, DissolveTrack);
		DissolveTimeline->Play();
	}
}

void ABlasterCharacter::PollInit()
{
	// BlasterPlayerState를 초기화하면서 최초로 한번만 실행
	if (!BlasterPlayerState)
	{
		if (BlasterPlayerState = GetPlayerState<ABlasterPlayerState>())
		{
			BlasterPlayerState->AddToScore(0.f);
			BlasterPlayerState->AddToDefeats(0);
		}
	}

	InitPlayerController();
}

void ABlasterCharacter::RotateInPlace(float DeltaTime)
{
	if (bDisableGameplay)
	{
		bUseControllerRotationYaw = false;
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		return;
	}

	if (GetLocalRole() > ENetRole::ROLE_SimulatedProxy && IsLocallyControlled())
	{
		// 실시간으로 에임오프셋을 계산
		AimOffset(DeltaTime);
	}
	else
	{
		// 움직임이 너무 오래 갱신이 안되면 직접 갱신함
		TimeSinceLastMovementReplication += DeltaTime;
		if (TimeSinceLastMovementReplication > 0.25f)
		{
			OnRep_ReplicatedMovement();
		}
		CalculateAO_Pitch();
	}
}

void ABlasterCharacter::UpdateHUDAmmo()
{
	InitPlayerController();
	if (BlasterPlayerController && Combat && Combat->EquippedWeapon)
	{
		BlasterPlayerController->SetHUDWeaponAmmo(Combat->EquippedWeapon->GetAmmo());
		BlasterPlayerController->SetHUDCarriedAmmo(Combat->CarriedAmmo);
	}
}

void ABlasterCharacter::DropOrDestroyWeapon(AWeapon* Weapon)
{
	if (!Weapon) return;

	if (Weapon->GetDestroyWeapon())
	{
		Weapon->Destroy();
	}
	else
	{
		Weapon->Dropped();
	}
}

void ABlasterCharacter::DropOrDestroyWeapons()
{
	if (Combat)
	{
		DropOrDestroyWeapon(Combat->EquippedWeapon);
		DropOrDestroyWeapon(Combat->SecondaryWeapon);
	}
}

void ABlasterCharacter::OnRep_OverlappingWeapon(AWeapon* LastWeapon)
{
	// 레플리케이트되기 전 무기는 감춘다
	if (LastWeapon)
	{
		LastWeapon->ShowPickupWidget(false);
	}

	// 레플리케이트된 무기를 보여줌
	if (OverlappingWeapon)
	{
		OverlappingWeapon->ShowPickupWidget(true);
	}
}

void ABlasterCharacter::OnRep_Health(float LastHealth)
{
	// 데미지 처리되면서 Health가 변경되고 레플리케이션 되면서 여기 함수가 호출된다.
	// 서버에서 데미지 처리하면서 하는 내용들을 클라에서도 똑같이 함
	UpdateHUDHealth();

	if (Health < LastHealth)
	{
		PlayHitReactMontage();
	}
}

void ABlasterCharacter::OnRep_Shield(float LastShield)
{
	UpdateHUDShield();

	if (Shield < LastShield)
	{
		PlayHitReactMontage();
	}
}
