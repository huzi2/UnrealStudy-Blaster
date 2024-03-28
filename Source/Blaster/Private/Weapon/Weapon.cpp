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
#include "Kismet/KismetMathLibrary.h"
//#include "DrawDebugHelpers.h"

AWeapon::AWeapon()
	: DistanceToSphere(800.f)
	, SphereRadius(75.f)
	, Damage(20.f)
	, HeadShotDamage(40.f)
	, bUseServerSideRewind(false)
	, ZoomedFOV(30.f)
	, ZoomInterpSpeed(20.f)
	, bAutomatic(true)
	, FireDelay(0.15f)
	, bUseScatter(false)
	, bDestroyWeapon(false)
	, Sequence(0)
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

	if (AreaSphere)
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

	// ���� ������ Ŭ�󿡼� ���� �ǰ��⸦ ������� �������� �ϱ⿡ �ش� ������ ������. ���� Ŭ�� �˸�ȴ�.
	DOREPLIFETIME_CONDITION(AWeapon, bUseServerSideRewind, COND_OwnerOnly);
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

	SpendRound();
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
			WeaponMesh->SetEnableGravity(false);
			WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}

		// ���� �ܰ��� ����
		EnableCustomDepth(false);
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

		// ���� �ܰ� ���� ȿ�� Ŵ
		WeaponMesh->SetCustomDepthStencilValue(CUSTOM_DEPTH_BLUE);
		WeaponMesh->MarkRenderStateDirty();
		EnableCustomDepth(true);
	}

	// ���⸦ ����߸��� ����� ��������Ʈ ����
	if (BlasterOwnerController && HasAuthority() && BlasterOwnerController->HighPingDelegate.IsBound())
	{
		BlasterOwnerController->HighPingDelegate.RemoveDynamic(this, &ThisClass::OnPingTooHigh);
	}
}

void AWeapon::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	ABlasterCharacter* BlasterCharacther = Cast<ABlasterCharacter>(OtherActor);
	if (BlasterCharacther)
	{
		// ����� ���� ���� �� �� �ִ�.
		if (WeaponType == EWeaponType::EWT_Flag && BlasterCharacther->GetTeam() != Team) return;
		// ����� �� ���¿����� ���⸦ ���� �� ����.
		if (BlasterCharacther->IsHoldingTheFlag()) return;

		BlasterCharacther->SetOverlappingWeapon(this);
	}
}

void AWeapon::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	ABlasterCharacter* BlasterCharacther = Cast<ABlasterCharacter>(OtherActor);
	if (BlasterCharacther)
	{
		if (WeaponType == EWeaponType::EWT_Flag && BlasterCharacther->GetTeam() != Team) return;
		if (BlasterCharacther->IsHoldingTheFlag()) return;

		BlasterCharacther->SetOverlappingWeapon(nullptr);
	}
}

void AWeapon::ClientUpdateAmmo_Implementation(int32 ServerAmmo)
{
	if (HasAuthority()) return;

	// �������� ��û�Ѵ�� Ammo�� ����
	Ammo = ServerAmmo;
	// ���� ��û�� ó�������Ƿ� --
	--Sequence;
	// ���� ���� ��û�� �ִٸ� �̸� �����ؼ� ����
	// �� �Լ��� SpendRound()�� �Ѿ��� �Һ��ϸ� ȣ��ǹǷ� �ذ���� ���� ��û�� �Ѿ��� �Һ��ϴ� ���̹Ƿ� �׸�ŭ �Ѿ��� �Һ��ߴٰ�ģ��.
	Ammo -= Sequence;
	SetHUDAmmo();
}

void AWeapon::ClientAddAmmo_Implementation(int32 AmmoToAdd)
{
	if (HasAuthority()) return;

	Ammo = FMath::Clamp(Ammo + AmmoToAdd, 0, MagCapacity);
	if (BlasterOwnerCharacter && BlasterOwnerCharacter->GetCombat() && IsFull())
	{
		BlasterOwnerCharacter->GetCombat()->JumpToShotgunEnd();
	}

	SetHUDAmmo();
}

void AWeapon::OnPingTooHigh(bool bPingTooHigh)
{
	// ���� �ʹ� ������ ���� �ǰ��⸦ ������� �ʴ´�.
	// �� ������ �ǰ��ڰ� ���� ���� �����µ� ���� �ǰ���� �¾ƹ����� ���ո����� ��Ÿ�� �� �ֱ� ����
	bUseServerSideRewind = !bPingTooHigh;
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

	ClientAddAmmo(Ammo);
}

void AWeapon::EnableCustomDepth(bool bEnable)
{
	// ����Ʈ ���μ����� ����Ʈ ���μ��� ��Ƽ������ ���� ���ٽ� ȿ���� ����� ���� �ܰ� ���� ó��
	if (WeaponMesh)
	{
		WeaponMesh->SetRenderCustomDepth(bEnable);
	}
}

FVector AWeapon::TraceEndWithScatter(const FVector& HitTarget) const
{
	if (!GetWeaponMesh()) return FVector();

	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName(TEXT("MuzzleFlash"));
	if (!MuzzleFlashSocket) return FVector();

	const FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
	const FVector TraceStart = SocketTransform.GetLocation();

	// ����ó�� źȯ�� ����Ű�� ���� Ÿ�������� ���� ��ü�� ����
	const FVector ToTargetNormalized = (HitTarget - TraceStart).GetSafeNormal();
	const FVector SphereCenter = TraceStart + ToTargetNormalized * DistanceToSphere;
	// ��ü �������� ������ ���� �ϳ��� ��
	const FVector RandVec = UKismetMathLibrary::RandomUnitVector() * FMath::FRandRange(0.f, SphereRadius);
	const FVector EndLoc = SphereCenter + RandVec;
	// ������ ���ͱ����� �Ÿ�
	const FVector ToEndLoc = EndLoc - TraceStart;

	/*DrawDebugSphere(GetWorld(), SphereCenter, SphereRadius, 12, FColor::Red, true);
	DrawDebugSphere(GetWorld(), EndLoc, 4.f, 12, FColor::Orange, true);
	DrawDebugLine(GetWorld(), TraceStart, TraceStart + ToEndLoc * TRACE_LENGTH / ToEndLoc.Size(), FColor::Cyan, true);*/

	// ������������ ������ġ���� Trace �Ÿ�/�����ŭ�� ��������Ʈ
	// �Ÿ�/������� �׳� ���� �Ÿ��� �ʹ� �־ ���� ������ �����Ÿ���
	return TraceStart + ToEndLoc * TRACE_LENGTH / ToEndLoc.Size();
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
		if (bUseServerSideRewind && BlasterOwnerController && HasAuthority() && !BlasterOwnerController->HighPingDelegate.IsBound())
		{
			BlasterOwnerController->HighPingDelegate.AddDynamic(this, &ThisClass::OnPingTooHigh);
		}
	}
}

void AWeapon::SpendRound()
{
	Ammo = FMath::Clamp(Ammo - 1, 0, MagCapacity);
	SetHUDAmmo();

	// �������� ȣ���ߴٸ� �ش� Ŭ���̾�Ʈ���� ź���� ������Ʈ�϶�� ��û
	if (HasAuthority())
	{
		ClientUpdateAmmo(Ammo);
	}
	else
	{
		// Ŭ���̾�Ʈ�� ��� ���� ó������ ���� ������ ������� ++�Ѵ�.
		++Sequence;
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

		WeaponMesh->SetCustomDepthStencilValue(CUSTOM_DEPTH_TAN);
		WeaponMesh->MarkRenderStateDirty();
	}

	// ���⸦ ��ü�ϸ� ����� ��������Ʈ ����
	if (BlasterOwnerController && HasAuthority() && BlasterOwnerController->HighPingDelegate.IsBound())
	{
		BlasterOwnerController->HighPingDelegate.RemoveDynamic(this, &ThisClass::OnPingTooHigh);
	}
}

void AWeapon::OnRep_WeaponState()
{
	OnWeaponStateSet();
}
