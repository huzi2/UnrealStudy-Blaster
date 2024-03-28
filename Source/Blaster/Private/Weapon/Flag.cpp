// Fill out your copyright notice in the Description page of Project Settings.

#include "Weapon/Flag.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"

AFlag::AFlag()
{
	FlagMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FlagMesh"));
	FlagMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	FlagMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetRootComponent(FlagMesh);

	AreaSphere->SetupAttachment(FlagMesh);
	PickupWidget->SetupAttachment(FlagMesh);
}

void AFlag::Dropped()
{
	SetWeaponState(EWeaponState::EWS_Dropped);

	// WeaponMesh가 아니라 FlagMesh를 써야해서 가상함수로 재정의한다.
	if (FlagMesh)
	{
		FDetachmentTransformRules DetachRules(EDetachmentRule::KeepWorld, true);
		FlagMesh->DetachFromComponent(DetachRules);
	}

	// 소유자 초기화. 서버에서만 호출되서 클라는 OnRep_Owner()에서 처리
	SetOwner(nullptr);
	BlasterOwnerCharacter = nullptr;
	BlasterOwnerController = nullptr;
}

void AFlag::OnEquipped()
{
	ShowPickupWidget(false);

	// 충돌 처리는 서버에서만 하므로 충돌 끄는건 서버에서만 하면됨
	if (HasAuthority() && AreaSphere)
	{
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (FlagMesh)
	{
		FlagMesh->SetSimulatePhysics(false);
		FlagMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		FlagMesh->SetEnableGravity(false);

		// 무기 외곽선 제거
		EnableCustomDepth(false);
	}
}

void AFlag::OnDropped()
{
	// 주울 수 있는 충돌 처리는 서버에서만 하므로 충돌 키는건 서버에서만 하면됨
	if (HasAuthority() && AreaSphere)
	{
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	}

	if (FlagMesh)
	{
		FlagMesh->SetSimulatePhysics(true);
		FlagMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		FlagMesh->SetEnableGravity(true);

		// 드롭할떄는 땅과 다시 충돌해야하므로 블록으로 설정
		FlagMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
		FlagMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
		FlagMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);

		// 무기 외곽 강조 효과 킴
		FlagMesh->SetCustomDepthStencilValue(CUSTOM_DEPTH_BLUE);
		FlagMesh->MarkRenderStateDirty();
		EnableCustomDepth(true);
	}
}
