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

	// EquippedWeapon은 캐릭터가 RPC로 서버에서만 호출하기에 각각의 클라에서는 nullptr이다
	// 근데 애님인스턴스에서 해당 변수로 무기가 장착되었는지 확인하기에 각 클라에 레플리케이트해야한다.
	DOREPLIFETIME(UCombatComponent, EquippedWeapon);

	// 조준 자세를 다른 클라도 확인해야해서 레플리케이트
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
	// 멀티캐스트는 서버만 호출할 수 있음. 그래서 서버에서 처리
	// 멀티캐스트를 통해 서버에서도 호출되므로 여기서 따로 또 발사를 할 필요는 없다.
	MulticastFire(TraceHitTarget);
}

void UCombatComponent::MulticastFire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{
	if (!Character) return;
	if (!EquippedWeapon) return;

	// 발사 모션과 발사 이펙트가 모든 클라이언트에서 보이도록 멀티캐스트
	// 멀티캐스트로 서버, 클라 모두에서 발사 실행
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

	if (Character && Character->GetCharacterMovement())
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
	}
}

void UCombatComponent::FireButtonPressed(bool bPressed)
{
	bFireButtonPressed = bPressed;

	// 서버에서 발사를 처리. 그냥 여기에서 처리하면 해당 클라이언트에서만 보임
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

	// 뷰포트 좌표를 월드 좌표로 변환
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
