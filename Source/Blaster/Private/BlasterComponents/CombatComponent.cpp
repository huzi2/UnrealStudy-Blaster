// Fill out your copyright notice in the Description page of Project Settings.

#include "BlasterComponents/CombatComponent.h"
#include "Weapon/Weapon.h"
#include "Character/BlasterCharacter.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"

UCombatComponent::UCombatComponent()
	: BaseWalkSpeed(600.f)
	, AimWalkSpeed(450.f)
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	if (Character && Character->GetCharacterMovement())
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;
	}
}

void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
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

void UCombatComponent::ServerFire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{
	// ��Ƽĳ��Ʈ�� ������ ȣ���� �� ����. �׷��� �������� ó��
	// ��Ƽĳ��Ʈ�� ���� ���������� ȣ��ǹǷ� ���⼭ ���� �� �߻縦 �� �ʿ�� ����.
	MulticastFire(TraceHitTarget);
}

void UCombatComponent::MulticastFire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{
	if (!Character) return;
	if (!EquippedWeapon) return;

	// �߻� ��ǰ� �߻� ����Ʈ�� ��� Ŭ���̾�Ʈ���� ���̵��� ��Ƽĳ��Ʈ
	// ��Ƽĳ��Ʈ�� ����, Ŭ�� ��ο��� �߻� ����
	Character->PlayFireMontage(bAiming);
	EquippedWeapon->Fire(TraceHitTarget);
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

	// �������� �߻縦 ó��. �׳� ���⿡�� ó���ϸ� �ش� Ŭ���̾�Ʈ������ ����
	if (bFireButtonPressed)
	{
		FHitResult HitResult;
		TraceUnderCrosshairs(HitResult);

		ServerFire(HitResult.ImpactPoint);
	}
}

void UCombatComponent::TraceUnderCrosshairs(FHitResult& TraceHitResult)
{
	if (!GEngine) return;
	if (!GEngine->GameViewport) return;
	if (!GetWorld()) return;

	FVector2D ViewportSize;
	GEngine->GameViewport->GetViewportSize(ViewportSize);

	const FVector2D CrosshairLocation(ViewportSize.X / 2.0, ViewportSize.Y / 2.0);
	FVector CrosshairWorldPosition;
	FVector CrosshairWorldDirection;

	// ����Ʈ ��ǥ�� ���� ��ǥ�� ��ȯ
	bool bScreenToWorld = UGameplayStatics::DeprojectScreenToWorld(UGameplayStatics::GetPlayerController(this, 0), CrosshairLocation, CrosshairWorldPosition, CrosshairWorldDirection);
	if (bScreenToWorld)
	{
		const FVector& Start = CrosshairWorldPosition;
		const FVector End = Start + CrosshairWorldDirection * TRACE_LENGTH;

		GetWorld()->LineTraceSingleByChannel(TraceHitResult, Start, End, ECollisionChannel::ECC_Visibility);
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
