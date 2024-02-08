// Fill out your copyright notice in the Description page of Project Settings.

#include "BlasterComponents/CombatComponent.h"
#include "Weapon/Weapon.h"
#include "Character/BlasterCharacter.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/CharacterMovementComponent.h"

UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// EquippedWeapon은 캐릭터가 RPC로 서버에서만 호출하기에 각각의 클라에서는 nullptr이다
	// 근데 애님인스턴스에서 해당 변수로 무기가 장착되었는지 확인하기에 각 클라에 레플리케이트해야한다.
	DOREPLIFETIME(UCombatComponent, EquippedWeapon);

	// 조준 자세를 다른 클라도 확인해야해서 레플리케이트
	DOREPLIFETIME(UCombatComponent, bAiming);
}

void UCombatComponent::ServerSetAiming_Implementation(bool bIsAiming)
{
	bAiming = bIsAiming;
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

	// 무기를 낀 후에는 정면을 조준하면서 이동하도록 함
	if (Character->GetCharacterMovement())
	{
		Character->GetCharacterMovement()->bOrientRotationToMovement = false;
		Character->bUseControllerRotationYaw = true;
	}
}

void UCombatComponent::SetAiming(bool bIsAiming)
{
	// 자신의 조준 자세는 자신이 확인 가능하나, 다른 클라에게는 보이지 않는다.
	bAiming = bIsAiming;

	// 클라이언트의 경우 다른 클라도 조준 자세를 확인하도록 서버가 변수를 처리하도록 요청
	// 해당 함수를 Server로 지정해서 클라에서 호출하면 서버에서 호출해줌
	ServerSetAiming(bIsAiming);
}

void UCombatComponent::OnRep_EquippedWeapon()
{
	// 다른 클라이언트에서도 무기들었을 때 회전하지 않도록 함
	if (EquippedWeapon && Character)
	{
		if (Character->GetCharacterMovement())
		{
			Character->GetCharacterMovement()->bOrientRotationToMovement = false;
			Character->bUseControllerRotationYaw = true;
		}
	}
}
