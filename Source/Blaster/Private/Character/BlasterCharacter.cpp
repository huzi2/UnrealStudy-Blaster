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

	// ������ �� ��ġ���� ������ ����
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

	// ī�޶� �޽��� ĸ���� �浹�ؼ� ���εǴ� ���� ����
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);
	// ĸ���� �ƴ� �޽ð� �浹�ϵ���(��弦 ���� ������ ����) Ŀ���� ������Ʈ ä�η� ����
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

	// ���� ������ �� ü�� �ʱ�ȭ
	// ������ ���� �߿� ĳ���Ͱ� ���ŵǰ� ������� ���� ��Ʈ�ѷ��� ������� ���� ���¿��� �����Ǽ� �̺κ��� ȣ��ȵ� ���� �ִ�.
	// �׷��� ���� �� ������� ���ؼ��� ��Ʈ�ѷ��� OnPossess()���� ó���Ѵ�.
	// �ٵ� ���⿡ �� �Լ��� ���ܵδ� �� ���� ������ ���� ��Ʈ�ѷ����� UI�� ���߿� �����Ǹ� OnPossess()�� �ǹ̰� ���� ����
	UpdateHUDHealth();
	UpdateHUDShield();

	// ������ ó���� ������
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

	// �÷��̾����Ʈ�� �ʱ�ȭ Ÿ�̹��� �ָ��ؼ� Tick���� �ʱ�ȭ��
	// ���ο��� �÷��̾����Ʈ�� ���ʷ� ������ ���� �ʱ�ȭ�ϵ��� �س���
	PollInit();

	// ������ �����ϰų� ������ ��� ���ӵ��� ��ü�� ������
	RotateInPlace(DeltaTime);
	
	// ī�޶�� ĳ���Ͱ� �ʹ� ��������� ĳ���͸� ����
	HideCameraIfCharacterClose();
}

void ABlasterCharacter::Destroyed()
{
	Super::Destroyed();

	// ���� ���� ���� Ÿ�̸Ӱ� ������ ȣ���Ѵ�. ������ �̷��� ���������� ���ŵ�
	// Ŭ���̾�Ʈ������ ĳ���Ϳ� ���� �����ϵ�����
	if (ElimBotComponent)
	{
		ElimBotComponent->DestroyComponent();
	}

	// ��ġ ���°� �ƴ� �� ���ŵǸ� ���⵵ ���� ����
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

// ���ø�����Ʈ �� �� ȣ��Ǵ� �Լ�
void ABlasterCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// COND_OwnerOnly�� ��ü �����ڸ� ���ø�����Ʈ �ǵ�����
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
	// �� ƽ���� �ݾ�����Ʈ�� �̷�������ʴ´�.
	// �׷��� �̵��� ���õ� ƽ ���� ����� �̰����� �ؼ� �����Ʈ�� ���ŵ� ������ �����ų �� �ִ�.
	Super::OnRep_ReplicatedMovement();

	// Tick���� ������ �ڽ��� Ŭ��� ���ӵ��� ��ü�� ���� ����������
	// ������ Ŭ����� ��Ʈ���� �������� �ʰ� ���ӵ��� ��ü ȸ�� ��Ų��.
	// �����Ʈ�� ���õ� ����̹Ƿ� Tick���� �ϴ°� �ƴ϶� ���⼭ ����
	SimProxiesTurn();
	TimeSinceLastMovementReplication = 0.f;
}

void ABlasterCharacter::MulticastElim_Implementation(bool bPlayerLeftGame)
{
	bLeftGame = bPlayerLeftGame;

	// ���� ó�� ��ü�� ���Ӹ�忡�� ������ ó������
	// �����鼭 �����ؾ��ϴ� �۾��� ��� Ŭ�� �ؾߵǼ� ��Ƽĳ��Ʈ�� �Լ� ����
	bElimmed = true;
	PlayElimMontage();

	// ź�� �ʱ�ȭ
	InitPlayerController();
	if (BlasterPlayerController)
	{
		BlasterPlayerController->SetHUDWeaponAmmo(0);
	}

	// ���� �� ������� ����Ʈ�� ���� ��Ƽ���� �ν��Ͻ��� ���� �� ����
	if (GetMesh() && DissolveMaterialInstance)
	{
		DynamicDissolveMaterialInstance = UMaterialInstanceDynamic::Create(DissolveMaterialInstance, this);
		GetMesh()->SetMaterial(0, DynamicDissolveMaterialInstance);
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), 0.55f);
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Glow"), 200.f);
	}

	StartDissolve();

	// �����Ʈ ����
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->DisableMovement();
		GetCharacterMovement()->StopMovementImmediately();
	}

	// Ư�� �ൿ�� ���ϰ� ���´�.
	bDisableGameplay = true;
	if (Combat)
	{
		// ���Ⱑ ������ �� �ڵ��߻縦 ����
		Combat->FireButtonPressed(false);
	}

	// �浹 ����
	if (GetCapsuleComponent())
	{
		GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	if (GetMesh())
	{
		GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	// �Ӹ��� �κ� ���� ȿ��
	if (ElimBotEffect)
	{
		const FVector ElimBotSpawnPoint(GetActorLocation().X, GetActorLocation().Y, GetActorLocation().Z + 200.0);
		ElimBotComponent = UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ElimBotEffect, ElimBotSpawnPoint, GetActorRotation());
	}

	if (ElimBotSound)
	{
		UGameplayStatics::SpawnSoundAtLocation(this, ElimBotSound, GetActorLocation());
	}

	// �������� ���ϰ� �ִ� ���¿��� �� �ܾƿ��ϵ�����
	if (IsLocallyControlled() && Combat && Combat->bAiming && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponType() == EWeaponType::EWT_SniperRifle)
	{
		ShowSniperScopeWidget(false);
	}

	// ĳ���� ��Ȱ�� ���� Ÿ�̸� ����
	GetWorldTimerManager().SetTimer(ElimTimer, this, &ThisClass::ElimTimerFinished, ElimDelay);
}

void ABlasterCharacter::ServerLeaveGame_Implementation()
{
	if (!GetWorld()) return;

	PollInit();

	if (!BlasterPlayerState) return;

	// ������ ���Ӹ�忡 �÷��̾� ���� ó�� ��û
	if (ABlasterGameMode* BlasterGameMode = GetWorld()->GetAuthGameMode<ABlasterGameMode>())
	{
		BlasterGameMode->PlayerLeftGame(BlasterPlayerState);
	}
}

void ABlasterCharacter::ServerEquipButtonPressed_Implementation()
{
	if (Combat)
	{
		// ����� ��ģ ���¸� �� ���� ����
		if (OverlappingWeapon)
		{
			Combat->EquipWeapon(OverlappingWeapon);
		}
		// �ƴ� ���¸� ���� ����� ����
		else if (Combat->ShouldSwapWeapons())
		{
			Combat->SwapWeapons();
		}
	}
}

void ABlasterCharacter::ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatorController, AActor* DamageCauser)
{
	if (bElimmed) return;

	// ������ ó���� ���������� �����. ���� ������� �������� ����ȴ�.
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

	// ü���� 0�̵Ǹ� ���Ӹ�忡�� ĳ���� ����
	// ���Ӹ��� ������ �������� �����ϱ⿡ Ŭ�󿡼� ȣ���ϸ� null�̴�.
	// ������ ������ ó�� ��ü�� ���������� ȣ��Ǳ����..
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
	// �浹 ó���� ���Ӹ� �ϵ��� �ؼ� �� �Լ��� ������ ����
	// Ŭ����� ���ø�����Ʈ ó�������� ������ ���⼭ ó��
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

		// ���⿡ ���� �ٸ� ���� ���
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
	// ĳ���Ͱ� �״� ó���� �������� ���Ѵ�.
	// ���⸦ ����߸��� ó��
	DropOrDestroyWeapons();

	// ������ �����鼭 �ִϸ��̼��� ����ϴ� ���� ������ ��� Ŭ�� �ؾߵǼ� ��Ƽĳ��Ʈ �Լ��� ȣ���Ѵ�.
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
		// �������� �������� �䱸. ������ �ٷ� �����ϰ�, Ŭ��� ������ ���ؼ� ����
		ServerEquipButtonPressed();

		// Ű�� ���� ���� Ŭ�󿡼� ȣ���ϴ� ��. ���� ������ SwapWeapons()���� �� ��
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
	// ���⸦ ����� ���� ����
	if (!Combat) return;
	if (!Combat->EquippedWeapon) return;
	if (!GetCharacterMovement()) return;

	const double Speed = CaculateSpeed();
	const bool bIsInAir = GetCharacterMovement()->IsFalling();

	// ������ �� �������� �����¿� �� ���� ����
	if (Speed == 0.0 && !bIsInAir)
	{
		bUseControllerRotationYaw = true;
		// ��Ʈ���� �������� ��ü�� ������ �ְ� ��ü�� �ٶ󺸴� �������� �����̵��� �Ѵ�.
		bRotateRootBone = true;

		FRotator CurrentAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		FRotator DeltaAimRotation = UKismetMathLibrary::NormalizedDeltaRotator(CurrentAimRotation, StartingAimRotation);
		AO_Yaw = DeltaAimRotation.Yaw;
		// ��ü�� ȸ���� �� ����ϱ� ���� ������ ���� ����
		if (TurningInPlace == ETurningInPlace::ETIP_NotTurning)
		{
			InterpAO_Yaw = AO_Yaw;
		}

		// -90�̳� 90 ���� �ٴٶ��� ��� �� ��ü�� ȸ�����Ѿ���
		TurnInPlace(DeltaTime);
	}
	// �ٰų� ������ ���� ���Ʒ��θ� ���� ����
	if (Speed > 0.0 || bIsInAir)
	{
		bUseControllerRotationYaw = true;
		bRotateRootBone = false;

		StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		AO_Yaw = 0.f;

		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
	}

	// ���Ʒ� ���
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

	// ���� ���� �̻� ȸ���ϸ� ȸ�� �ִϸ��̼��� ����
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
	// ���� ȸ���� �� ���ϰ� �ִϸ��̼� �������Ʈ���� Ȯ��
	if (AO_Yaw > 90.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_Right;
	}
	else if (AO_Yaw < -90.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_Left;
	}

	// ���� ȸ���� ����
	if (TurningInPlace != ETurningInPlace::ETIP_NotTurning)
	{
		// ȸ���ϱ����� �������� 15������ ����
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
	// ���� Ŭ�� �����ϸ��. �ٸ� Ŭ����� ĳ���Ͱ� �������� �ȵ�
	if (!IsLocallyControlled()) return;
	if (!FollowCamera) return;
	if (!GetMesh()) return;

	// ī�޶� �ʹ� ������
	if ((FollowCamera->GetComponentLocation() - GetActorLocation()).Size() < CameraThreshlod)
	{
		// �޽��� ����
		GetMesh()->SetVisibility(false);

		// ���⵵ ����
		if (Combat && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponMesh())
		{
			// Owner�� ������ �ɼ�
			Combat->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = true;
		}
	}
	else
	{
		GetMesh()->SetVisibility(true);

		// ���⵵ ����
		if (Combat && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponMesh())
		{
			Combat->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = false;
		}
	}
}

void ABlasterCharacter::CalculateAO_Pitch()
{
	AO_Pitch = GetBaseAimRotation().Pitch;

	// ��ġ���� ��� -90 ~ 0���� ���� ��Ŷ���� ���۵Ǵ� �������� 270 ~ 360���� ����ȴ�.
	// �׷��� �ڽ��� ��Ʈ���ϴ� ĳ���� ���� ĳ������ ��ġ���� ������ ���ش�.
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
	// ���Ӹ�忡�� ĳ���� ��Ȱ ��û. ���Ӹ��� �������� �����ؼ� ���������� ó���� ��
	if (ABlasterGameMode* BlasterGameMode = GetWorld()->GetAuthGameMode<ABlasterGameMode>())
	{
		// ������ ������ ���� ��쿡�� ������
		if (!bLeftGame)
		{
			BlasterGameMode->RequestRespawn(this, Controller);
		}
	}

	// �Ӹ� ���� �κ� ȿ�� ����
	if (ElimBotComponent)
	{
		ElimBotComponent->DestroyComponent();
	}

	// ������ �����ٸ� �����ٰ� ��ε�ĳ��Ʈ
	if (bLeftGame && IsLocallyControlled())
	{
		OnLeftGame.Broadcast();
	}
}

void ABlasterCharacter::StartDissolve()
{
	// ĳ���Ͱ� �װ� ������� ����Ʈ(��Ƽ���� Ŀ�갪�� ��)
	DissolveTrack.BindDynamic(this, &ThisClass::UpdateDissolveMaterial);
	
	if (DissolveTimeline && DissolveCurve)
	{
		DissolveTimeline->AddInterpFloat(DissolveCurve, DissolveTrack);
		DissolveTimeline->Play();
	}
}

void ABlasterCharacter::PollInit()
{
	// BlasterPlayerState�� �ʱ�ȭ�ϸ鼭 ���ʷ� �ѹ��� ����
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
		// �ǽð����� ���ӿ������� ���
		AimOffset(DeltaTime);
	}
	else
	{
		// �������� �ʹ� ���� ������ �ȵǸ� ���� ������
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
	// ���ø�����Ʈ�Ǳ� �� ����� �����
	if (LastWeapon)
	{
		LastWeapon->ShowPickupWidget(false);
	}

	// ���ø�����Ʈ�� ���⸦ ������
	if (OverlappingWeapon)
	{
		OverlappingWeapon->ShowPickupWidget(true);
	}
}

void ABlasterCharacter::OnRep_Health(float LastHealth)
{
	// ������ ó���Ǹ鼭 Health�� ����ǰ� ���ø����̼� �Ǹ鼭 ���� �Լ��� ȣ��ȴ�.
	// �������� ������ ó���ϸ鼭 �ϴ� ������� Ŭ�󿡼��� �Ȱ��� ��
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
