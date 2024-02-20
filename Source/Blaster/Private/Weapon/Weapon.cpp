// Fill out your copyright notice in the Description page of Project Settings.

#include "Weapon/Weapon.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "Character/BlasterCharacter.h"
#include "Net/UnrealNetwork.h"
#include "Weapon/Casing.h"
#include "Engine/SkeletalMeshSocket.h"

AWeapon::AWeapon()
	: ZoomedFOV(30.f)
	, ZoomInterpSpeed(20.f)
	, bAutomatic(true)
	, FireDelay(0.15f)
{
	PrimaryActorTick.bCanEverTick = false;
	// 리플리케이션을 켜서 서버의 내용을 클라가 모두 복제하도록함
	bReplicates = true;

	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	SetRootComponent(WeaponMesh);
	WeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
	WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Block);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	AreaSphere = CreateDefaultSubobject<USphereComponent>(TEXT("AreaSphere"));
	AreaSphere->SetupAttachment(GetRootComponent());
	// 서버에서만 충돌 처리하도록 하기위해 일단 충돌 처리를 끈다.
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

	// 충돌 처리는 서버에서만 진행하도록 함
	if (HasAuthority())
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
}

void AWeapon::Fire(const FVector& HitTarget)
{
	if (FireAnimation && WeaponMesh)
	{
		WeaponMesh->PlayAnimation(FireAnimation, false);
	}

	// 탄피 배출
	if (CasingClass && WeaponMesh && GetWorld())
	{
		const USkeletalMeshSocket* AmmoEjectSocket = WeaponMesh->GetSocketByName(TEXT("AmmoEject"));
		if (AmmoEjectSocket)
		{
			// 생성 지점은 AmmoEject 소켓
			const FTransform SocketTransform = AmmoEjectSocket->GetSocketTransform(WeaponMesh);

			FActorSpawnParameters SpawnParams;
			GetWorld()->SpawnActor<ACasing>(CasingClass, SocketTransform.GetLocation(), SocketTransform.GetRotation().Rotator(), SpawnParams);
		}
	}
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

	switch (WeaponState)
	{
	case EWeaponState::EWS_Equipped:
		ShowPickupWidget(false);

		if (AreaSphere)
		{
			AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}

		if (WeaponMesh)
		{
			WeaponMesh->SetSimulatePhysics(false);
			WeaponMesh->SetEnableGravity(false);
			WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
		break;
	case EWeaponState::EWS_Dropped:
		// 주울 수 있는 충돌 처리는 서버에서만 하므로 충돌 키는건 서버에서만 하면됨
		if (HasAuthority() && AreaSphere)
		{
			AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		}

		if (WeaponMesh)
		{
			WeaponMesh->SetSimulatePhysics(true);
			WeaponMesh->SetEnableGravity(true);
			WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		}
		break;
	default:
		break;
	}
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

	if (WeaponMesh)
	{
		FDetachmentTransformRules DetachRules(EDetachmentRule::KeepWorld, true);
		WeaponMesh->DetachFromComponent(DetachRules);
		SetOwner(nullptr);
	}
}

void AWeapon::OnRep_WeaponState()
{
	switch (WeaponState)
	{
	case EWeaponState::EWS_Equipped:
		ShowPickupWidget(false);

		if (WeaponMesh)
		{
			WeaponMesh->SetSimulatePhysics(false);
			WeaponMesh->SetEnableGravity(false);
			WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
		break;
	case EWeaponState::EWS_Dropped:
		if (WeaponMesh)
		{
			WeaponMesh->SetSimulatePhysics(true);
			WeaponMesh->SetEnableGravity(true);
			WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		}
		break;
	default:
		break;
	}
}
