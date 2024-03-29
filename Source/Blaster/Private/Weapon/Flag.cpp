// Fill out your copyright notice in the Description page of Project Settings.

#include "Weapon/Flag.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "Character/BlasterCharacter.h"

AFlag::AFlag()
{
	FlagMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FlagMesh"));
	FlagMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	FlagMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetRootComponent(FlagMesh);

	AreaSphere->SetupAttachment(FlagMesh);
	PickupWidget->SetupAttachment(FlagMesh);
}

void AFlag::BeginPlay()
{
	Super::BeginPlay();

	// ����� ������� �־��� �� �ٽ� ���� ��� ��ġ�� ���ư��� ���ؼ� ����
	InitialTransform = GetActorTransform();
}

void AFlag::Dropped()
{
	SetWeaponState(EWeaponState::EWS_Dropped);

	// WeaponMesh�� �ƴ϶� FlagMesh�� ����ؼ� �����Լ��� �������Ѵ�.
	if (FlagMesh)
	{
		FDetachmentTransformRules DetachRules(EDetachmentRule::KeepWorld, true);
		FlagMesh->DetachFromComponent(DetachRules);
	}

	// ������ �ʱ�ȭ. ���������� ȣ��Ǽ� Ŭ��� OnRep_Owner()���� ó��
	SetOwner(nullptr);
	BlasterOwnerCharacter = nullptr;
	BlasterOwnerController = nullptr;
}

void AFlag::OnEquipped()
{
	ShowPickupWidget(false);

	// �浹 ó���� ���������� �ϹǷ� �浹 ���°� ���������� �ϸ��
	if (HasAuthority() && AreaSphere)
	{
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (FlagMesh)
	{
		FlagMesh->SetSimulatePhysics(false);
		FlagMesh->SetEnableGravity(false);

		// ������� �浹 Ȯ���ؾ��ϹǷ� ���� �¸�
		FlagMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		FlagMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECollisionResponse::ECR_Overlap);;

		// ���� �ܰ��� ����
		EnableCustomDepth(false);
	}
}

void AFlag::OnDropped()
{
	// �ֿ� �� �ִ� �浹 ó���� ���������� �ϹǷ� �浹 Ű�°� ���������� �ϸ��
	if (HasAuthority() && AreaSphere)
	{
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	}

	if (FlagMesh)
	{
		FlagMesh->SetSimulatePhysics(true);
		FlagMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		FlagMesh->SetEnableGravity(true);

		// ����ҋ��� ���� �ٽ� �浹�ؾ��ϹǷ� ������� ����
		FlagMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
		FlagMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
		FlagMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);

		// ���� �ܰ� ���� ȿ�� Ŵ
		FlagMesh->SetCustomDepthStencilValue(CUSTOM_DEPTH_BLUE);
		FlagMesh->MarkRenderStateDirty();
		EnableCustomDepth(true);
	}
}

void AFlag::ResetFlag()
{
	// ����� ��� �ִ� �÷��̾ ����� ���´�.
	if (ABlasterCharacter* FlagBearer = Cast<ABlasterCharacter>(GetOwner()))
	{
		FlagBearer->SetHoldingTheFlag(false);
		FlagBearer->SetOverlappingWeapon(nullptr);
		FlagBearer->UnCrouch();
	}

	if (!HasAuthority()) return;

	if (FlagMesh)
	{
		FDetachmentTransformRules DetachRules(EDetachmentRule::KeepWorld, true);
		FlagMesh->DetachFromComponent(DetachRules);
	}

	if (AreaSphere)
	{
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		AreaSphere->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
	}

	SetWeaponState(EWeaponState::EWS_Initial);

	SetOwner(nullptr);
	BlasterOwnerCharacter = nullptr;
	BlasterOwnerController = nullptr;

	SetActorTransform(InitialTransform);
}
