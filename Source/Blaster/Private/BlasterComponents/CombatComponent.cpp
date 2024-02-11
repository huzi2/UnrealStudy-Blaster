// Fill out your copyright notice in the Description page of Project Settings.

#include "BlasterComponents/CombatComponent.h"
#include "Weapon/Weapon.h"
#include "Character/BlasterCharacter.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/CharacterMovementComponent.h"

UCombatComponent::UCombatComponent()
	: BaseWalkSpeed(600.f)
	, AimWalkSpeed(450.f)
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	if (Character && Character->GetCharacterMovement())
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;
	}
}

void UCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// EquippedWeapon�� ĳ���Ͱ� RPC�� ���������� ȣ���ϱ⿡ ������ Ŭ�󿡼��� nullptr�̴�
	// �ٵ� �ִ��ν��Ͻ����� �ش� ������ ���Ⱑ �����Ǿ����� Ȯ���ϱ⿡ �� Ŭ�� ���ø�����Ʈ�ؾ��Ѵ�.
	DOREPLIFETIME(UCombatComponent, EquippedWeapon);

	// ���� �ڼ��� �ٸ� Ŭ�� Ȯ���ؾ��ؼ� ���ø�����Ʈ
	DOREPLIFETIME(UCombatComponent, bAiming);
}

void UCombatComponent::ServerSetAiming_Implementation(bool bIsAiming)
{
	bAiming = bIsAiming;

	if (Character && Character->GetCharacterMovement())
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
	}
}

void UCombatComponent::EquipWeapon(AWeapon* WeaponToEquip)
{
	if (!Character || !WeaponToEquip) return;

	EquippedWeapon = WeaponToEquip;
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);

	const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(TEXT("RightHandSocket"));
	if (HandSocket)
	{
		HandSocket->AttachActor(EquippedWeapon, Character->GetMesh());
	}

	EquippedWeapon->SetOwner(Character);

	// ���⸦ �� �Ŀ��� ������ �����ϸ鼭 �̵��ϵ��� ��
	if (Character->GetCharacterMovement())
	{
		Character->GetCharacterMovement()->bOrientRotationToMovement = false;
		Character->bUseControllerRotationYaw = true;
	}
}

void UCombatComponent::SetAiming(bool bIsAiming)
{
	// �ڽ��� ���� �ڼ��� �ڽ��� Ȯ�� �����ϳ�, �ٸ� Ŭ�󿡰Դ� ������ �ʴ´�.
	bAiming = bIsAiming;

	// Ŭ���̾�Ʈ�� ��� �ٸ� Ŭ�� ���� �ڼ��� Ȯ���ϵ��� ������ ������ ó���ϵ��� ��û
	// �ش� �Լ��� Server�� �����ؼ� Ŭ�󿡼� ȣ���ϸ� �������� ȣ������
	ServerSetAiming(bIsAiming);

	if (Character && Character->GetCharacterMovement())
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
	}
}

void UCombatComponent::FireButtonPressed(bool bPressed)
{
	bFireButtonPressed = bPressed;

	if (Character && bFireButtonPressed)
	{
		Character->PlayFireMontage(bAiming);
	}
}

void UCombatComponent::OnRep_EquippedWeapon()
{
	// �ٸ� Ŭ���̾�Ʈ������ �������� �� ȸ������ �ʵ��� ��
	if (EquippedWeapon && Character)
	{
		if (Character->GetCharacterMovement())
		{
			Character->GetCharacterMovement()->bOrientRotationToMovement = false;
			Character->bUseControllerRotationYaw = true;
		}
	}
}
