// Fill out your copyright notice in the Description page of Project Settings.

#include "Weapon/Weapon.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "Character/BlasterCharacter.h"
#include "Net/UnrealNetwork.h"
#include "Weapon/Casing.h"
#include "Engine/SkeletalMeshSocket.h"
#include "PlayerController/BlasterPlayerController.h"
#include "BlasterComponents/CombatComponent.h"

AWeapon::AWeapon()
	: ZoomedFOV(30.f)
	, ZoomInterpSpeed(20.f)
	, bAutomatic(true)
	, FireDelay(0.15f)
	, bDestroyWeapon(false)
{
	PrimaryActorTick.bCanEverTick = false;
	// ���ø����̼��� �Ѽ� ������ ������ Ŭ�� ��� �����ϵ�����
	bReplicates = true;
	// �����Ʈ ���ø�����Ʈ�� true�� �Ͽ� �߷� ���� ���� �ùķ��̼����� �̵����� ���� ����ȭ�� �ȴ�.
	SetReplicateMovement(true);

	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	SetRootComponent(WeaponMesh);
	WeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
	WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
	WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	// ���� �ܰ� ���� ó���� ���� ���ٽ� ���� �� ����
	WeaponMesh->SetCustomDepthStencilValue(CUSTOM_DEPTH_BLUE);
	WeaponMesh->MarkRenderStateDirty();
	EnableCustomDepth(true);

	AreaSphere = CreateDefaultSubobject<USphereComponent>(TEXT("AreaSphere"));
	AreaSphere->SetupAttachment(GetRootComponent());
	// ���������� �浹 ó���ϵ��� �ϱ����� �ϴ� �浹 ó���� ����.
	AreaSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	PickupWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("PickupWidget"));
	PickupWidget->SetupAttachment(GetRootComponent());
}

void AWeapon::BeginPlay()
{
	Super::BeginPlay();

	if (PickupWidget)
	{
		PickupWidget->SetVisibility(false);
	}

	// �浹 ó���� ���������� �����ϵ��� ��
	if (HasAuthority() && AreaSphere)
	{
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		AreaSphere->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
		AreaSphere->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnSphereOverlap);
		AreaSphere->OnComponentEndOverlap.AddDynamic(this, &ThisClass::OnSphereEndOverlap);
	}
}

void AWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AWeapon, WeaponState);
	DOREPLIFETIME(AWeapon, Ammo);
}

void AWeapon::OnRep_Owner()
{
	// Owner ������ ���ø����̼� �� �� ȣ��. ���⼱ Ŭ���̾�Ʈ���� �����
	// ������ ��쿡�� ���� �����ϸ鼭 SetOwner�ϸ鼭 �����ϰ�����
	Super::OnRep_Owner();

	if (!Owner)
	{
		BlasterOwnerCharacter = nullptr;
		BlasterOwnerController = nullptr;
	}
	else
	{
		CheckInit();

		// ���� ���⸦ �� ���� HUD ������Ʈ�� �ϸ� �ȵ�
		if (BlasterOwnerCharacter && BlasterOwnerCharacter->GetEquippedWeapon() && BlasterOwnerCharacter->GetEquippedWeapon() == this)
		{
			// �� ���⿡ ����� ź�� ���� ���ʿ��� �˸�
			SetHUDAmmo();
		}
	}
}

void AWeapon::Fire(const FVector& HitTarget)
{
	if (FireAnimation && WeaponMesh)
	{
		WeaponMesh->PlayAnimation(FireAnimation, false);
	}

	// ź�� ����
	if (CasingClass && WeaponMesh && GetWorld())
	{
		const USkeletalMeshSocket* AmmoEjectSocket = WeaponMesh->GetSocketByName(TEXT("AmmoEject"));
		if (AmmoEjectSocket)
		{
			// ���� ������ AmmoEject ����
			const FTransform SocketTransform = AmmoEjectSocket->GetSocketTransform(WeaponMesh);

			FActorSpawnParameters SpawnParams;
			GetWorld()->SpawnActor<ACasing>(CasingClass, SocketTransform.GetLocation(), SocketTransform.GetRotation().Rotator(), SpawnParams);
		}
	}

	// ź�� ���
	SpendRound();
}

void AWeapon::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	ABlasterCharacter* BlasterCharacther = Cast<ABlasterCharacter>(OtherActor);
	if (BlasterCharacther)
	{
		BlasterCharacther->SetOverlappingWeapon(this);
	}
}

void AWeapon::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	ABlasterCharacter* BlasterCharacther = Cast<ABlasterCharacter>(OtherActor);
	if (BlasterCharacther)
	{
		BlasterCharacther->SetOverlappingWeapon(nullptr);
	}
}

void AWeapon::SetWeaponState(EWeaponState State)
{
	WeaponState = State;
	OnWeaponStateSet();
}

void AWeapon::ShowPickupWidget(bool bShowWidget)
{
	if (PickupWidget)
	{
		PickupWidget->SetVisibility(bShowWidget);
	}
}

void AWeapon::Dropped()
{
	SetWeaponState(EWeaponState::EWS_Dropped);

	// ���⸦ ���� ������
	if (WeaponMesh)
	{
		FDetachmentTransformRules DetachRules(EDetachmentRule::KeepWorld, true);
		WeaponMesh->DetachFromComponent(DetachRules);
	}

	// ������ �ʱ�ȭ. ���������� ȣ��Ǽ� Ŭ��� OnRep_Owner()���� ó��
	SetOwner(nullptr);
	BlasterOwnerCharacter = nullptr;
	BlasterOwnerController = nullptr;
}

void AWeapon::SetHUDAmmo()
{
	CheckInit();

	if (BlasterOwnerController)
	{
		BlasterOwnerController->SetHUDWeaponAmmo(Ammo);
	}
}

bool AWeapon::IsEmpty() const
{
	return Ammo <= 0;
}

bool AWeapon::IsFull() const
{
	return Ammo == MagCapacity;
}

void AWeapon::AddAmmo(int32 AmmoToAdd)
{
	Ammo = FMath::Clamp(Ammo + AmmoToAdd, 0, MagCapacity);
	SetHUDAmmo();
}

void AWeapon::EnableCustomDepth(bool bEnable)
{
	// ����Ʈ ���μ����� ����Ʈ ���μ��� ��Ƽ������ ���� ���ٽ� ȿ���� ����� ���� �ܰ� ���� ó��
	if (WeaponMesh)
	{
		WeaponMesh->SetRenderCustomDepth(bEnable);

	}
}

void AWeapon::SpendRound()
{
	// �������� ȣ���ϴ� Fire()���� ����ϱ⿡ ���������� ���Ǵ� �Լ�
	// �Ѿ� ���. ���ø����̼� ������ Ŭ���̾�Ʈ�鿡�Ե� ����ǰ� OnRep_Ammo() �Լ� ����
	Ammo = FMath::Clamp(Ammo - 1, 0, MagCapacity);
	SetHUDAmmo();
}

void AWeapon::CheckInit()
{
	if (!BlasterOwnerCharacter)
	{
		BlasterOwnerCharacter = Cast<ABlasterCharacter>(GetOwner());
	}

	if (BlasterOwnerCharacter && !BlasterOwnerController)
	{
		BlasterOwnerController = Cast<ABlasterPlayerController>(BlasterOwnerCharacter->Controller);
	}
}

void AWeapon::OnWeaponStateSet()
{
	switch (WeaponState)
	{
	case EWeaponState::EWS_Equipped:
		OnEquipped();
		break;
	case EWeaponState::EWS_EquippedSecondary:
		OnEquippedSecondary();
		break;
	case EWeaponState::EWS_Dropped:
		OnDropped();
		break;
	default:
		break;
	}
}

void AWeapon::OnEquipped()
{
	ShowPickupWidget(false);

	// �浹 ó���� ���������� �ϹǷ� �浹 ���°� ���������� �ϸ��
	if (HasAuthority() && AreaSphere)
	{
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (WeaponMesh)
	{
		WeaponMesh->SetSimulatePhysics(false);

		// SMG���� ������ �������� �� ��鸲�� �����ؼ� �浹ó���� �߷� ���
		if (WeaponType == EWeaponType::EWT_SubmachineGun)
		{
			WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			WeaponMesh->SetEnableGravity(true);
			// �׷��ٰ� �ٸ� ��ü�� �浹�ϸ� �ȵǴϱ� ����
			WeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
		}
		else
		{
			WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			WeaponMesh->SetEnableGravity(false);
		}

		// ���� �ܰ��� ����
		EnableCustomDepth(false);
	}
}

void AWeapon::OnEquippedSecondary()
{
	ShowPickupWidget(false);

	// �浹 ó���� ���������� �ϹǷ� �浹 ���°� ���������� �ϸ��
	if (HasAuthority() && AreaSphere)
	{
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (WeaponMesh)
	{
		WeaponMesh->SetSimulatePhysics(false);

		// SMG���� ������ �������� �� ��鸲�� �����ؼ� �浹ó���� �߷� ���
		if (WeaponType == EWeaponType::EWT_SubmachineGun)
		{
			WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			WeaponMesh->SetEnableGravity(true);
			// �׷��ٰ� �ٸ� ��ü�� �浹�ϸ� �ȵǴϱ� ����
			WeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
		}
		else
		{
			WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			WeaponMesh->SetEnableGravity(false);
		}

		EnableCustomDepth(true);
		WeaponMesh->SetCustomDepthStencilValue(CUSTOM_DEPTH_TAN);
		WeaponMesh->MarkRenderStateDirty();
	}
}

void AWeapon::OnDropped()
{
	// �ֿ� �� �ִ� �浹 ó���� ���������� �ϹǷ� �浹 Ű�°� ���������� �ϸ��
	if (HasAuthority() && AreaSphere)
	{
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	}

	if (WeaponMesh)
	{
		WeaponMesh->SetSimulatePhysics(true);
		WeaponMesh->SetEnableGravity(true);
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

		// ����ҋ��� ���� �ٽ� �浹�ؾ��ϹǷ� ������� ����(SMG�� ���)
		WeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
		WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
		WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	}

	// ���� �ܰ� ���� ȿ�� Ŵ
	WeaponMesh->SetCustomDepthStencilValue(CUSTOM_DEPTH_BLUE);
	WeaponMesh->MarkRenderStateDirty();
	EnableCustomDepth(true);
}

void AWeapon::OnRep_WeaponState()
{
	OnWeaponStateSet();
}

void AWeapon::OnRep_Ammo()
{
	// Ŭ���̾�Ʈ�� ź�� ����
	SetHUDAmmo();

	// ���� ���� �� ź�� ��á�� �� �ִϸ��̼� �����ϴ°� �Ĺ�������Ʈ���� �����ϴµ� ���������� �ϰ�����
	// Ŭ�󿡼� �ϴ°� ���⼭ �ϵ��ڵ����� �۾�
	if (BlasterOwnerCharacter && BlasterOwnerCharacter->GetCombat() && IsFull())
	{
		BlasterOwnerCharacter->GetCombat()->JumpToShotgunEnd();
	}
}
