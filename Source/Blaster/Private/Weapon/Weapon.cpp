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
	// 리플리케이션을 켜서 서버의 내용을 클라가 모두 복제하도록함
	bReplicates = true;
	// 무브먼트 레플리케이트도 true로 하여 중력 등의 물리 시뮬레이션으로 이동했을 때도 동기화가 된다.
	SetReplicateMovement(true);

	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	SetRootComponent(WeaponMesh);
	WeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
	WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
	WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	// 무기 외곽 강조 처리를 위한 스텐실 버퍼 값 적용
	WeaponMesh->SetCustomDepthStencilValue(CUSTOM_DEPTH_BLUE);
	WeaponMesh->MarkRenderStateDirty();
	EnableCustomDepth(true);

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

	// 핑이 높으면 클라에서 서버 되감기를 사용하지 않으려고 하기에 해당 변수를 복제함. 로컬 클라만 알면된다.
	DOREPLIFETIME_CONDITION(AWeapon, bUseServerSideRewind, COND_OwnerOnly);
}

void AWeapon::OnRep_Owner()
{
	// Owner 변수가 레플리케이션 될 때 호출. 여기선 클라이언트에만 적용됨
	// 서버의 경우에는 무기 장착하면서 SetOwner하면서 같이하고있음
	Super::OnRep_Owner();

	if (!Owner)
	{
		BlasterOwnerCharacter = nullptr;
		BlasterOwnerController = nullptr;
	}
	else
	{
		CheckInit();

		// 보조 무기를 들 때는 HUD 업데이트를 하면 안됨
		if (BlasterOwnerCharacter && BlasterOwnerCharacter->GetEquippedWeapon() && BlasterOwnerCharacter->GetEquippedWeapon() == this)
		{
			// 이 무기에 저장된 탄약 수를 오너에게 알림
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

	SpendRound();
}

void AWeapon::Dropped()
{
	SetWeaponState(EWeaponState::EWS_Dropped);

	// 무기를 땅에 떨구고
	if (WeaponMesh)
	{
		FDetachmentTransformRules DetachRules(EDetachmentRule::KeepWorld, true);
		WeaponMesh->DetachFromComponent(DetachRules);
	}

	// 소유자 초기화. 서버에서만 호출되서 클라는 OnRep_Owner()에서 처리
	SetOwner(nullptr);
	BlasterOwnerCharacter = nullptr;
	BlasterOwnerController = nullptr;
}

void AWeapon::OnEquipped()
{
	ShowPickupWidget(false);

	// 충돌 처리는 서버에서만 하므로 충돌 끄는건 서버에서만 하면됨
	if (HasAuthority() && AreaSphere)
	{
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (WeaponMesh)
	{
		WeaponMesh->SetSimulatePhysics(false);

		// SMG에는 피직스 에셋으로 줄 흔들림을 구현해서 충돌처리와 중력 사용
		if (WeaponType == EWeaponType::EWT_SubmachineGun)
		{
			WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			WeaponMesh->SetEnableGravity(true);
			// 그렇다고 다른 물체와 충돌하면 안되니까 무시
			WeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
		}
		else
		{
			WeaponMesh->SetEnableGravity(false);
			WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}

		// 무기 외곽선 제거
		EnableCustomDepth(false);
	}
}

void AWeapon::OnDropped()
{
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

		// 드롭할떄는 땅과 다시 충돌해야하므로 블록으로 설정(SMG의 경우)
		WeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
		WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
		WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);

		// 무기 외곽 강조 효과 킴
		WeaponMesh->SetCustomDepthStencilValue(CUSTOM_DEPTH_BLUE);
		WeaponMesh->MarkRenderStateDirty();
		EnableCustomDepth(true);
	}

	// 무기를 떨어뜨리면 연결된 델리게이트 해제
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
		// 깃발은 같은 팀만 들 수 있다.
		if (WeaponType == EWeaponType::EWT_Flag && BlasterCharacther->GetTeam() != Team) return;
		// 깃발을 든 상태에서는 무기를 집을 수 없다.
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

	// 서버에서 요청한대로 Ammo를 갱신
	Ammo = ServerAmmo;
	// 서버 요청이 처리됬으므로 --
	--Sequence;
	// 아직 서버 요청이 있다면 미리 예측해서 갱신
	// 이 함수는 SpendRound()로 총알을 소비하면 호출되므로 해결되지 않은 요청은 총알을 소비하는 것이므로 그만큼 총알을 소비했다고친다.
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
	// 핑이 너무 높으면 서버 되감기를 사용하지 않는다.
	// 그 이유는 피격자가 총을 피해 숨었는데 서버 되감기로 맞아버리는 불합리함이 나타날 수 있기 때문
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
	// 포스트 프로세스와 포스트 프로세스 머티리얼을 통한 스텐실 효과를 사용한 무기 외곽 강조 처리
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

	// 샷건처럼 탄환을 방사시키기 위해 타겟지점에 원형 구체를 생성
	const FVector ToTargetNormalized = (HitTarget - TraceStart).GetSafeNormal();
	const FVector SphereCenter = TraceStart + ToTargetNormalized * DistanceToSphere;
	// 구체 기준으로 랜덤한 벡터 하나를 고름
	const FVector RandVec = UKismetMathLibrary::RandomUnitVector() * FMath::FRandRange(0.f, SphereRadius);
	const FVector EndLoc = SphereCenter + RandVec;
	// 랜덤한 벡터까지의 거리
	const FVector ToEndLoc = EndLoc - TraceStart;

	/*DrawDebugSphere(GetWorld(), SphereCenter, SphereRadius, 12, FColor::Red, true);
	DrawDebugSphere(GetWorld(), EndLoc, 4.f, 12, FColor::Orange, true);
	DrawDebugLine(GetWorld(), TraceStart, TraceStart + ToEndLoc * TRACE_LENGTH / ToEndLoc.Size(), FColor::Cyan, true);*/

	// 시작지점에서 랜덤위치까지 Trace 거리/사이즈만큼이 엔드포인트
	// 거리/사이즈는 그냥 기존 거리가 너무 멀어서 정한 임의의 사정거리임
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

	// 서버에서 호출했다면 해당 클라이언트에게 탄약을 업데이트하라고 요청
	if (HasAuthority())
	{
		ClientUpdateAmmo(Ammo);
	}
	else
	{
		// 클라이언트의 경우 아직 처리되지 않은 서버의 명령으로 ++한다.
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

	// 충돌 처리는 서버에서만 하므로 충돌 끄는건 서버에서만 하면됨
	if (HasAuthority() && AreaSphere)
	{
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (WeaponMesh)
	{
		WeaponMesh->SetSimulatePhysics(false);

		// SMG에는 피직스 에셋으로 줄 흔들림을 구현해서 충돌처리와 중력 사용
		if (WeaponType == EWeaponType::EWT_SubmachineGun)
		{
			WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			WeaponMesh->SetEnableGravity(true);
			// 그렇다고 다른 물체와 충돌하면 안되니까 무시
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

	// 무기를 교체하면 연결된 델리게이트 해제
	if (BlasterOwnerController && HasAuthority() && BlasterOwnerController->HighPingDelegate.IsBound())
	{
		BlasterOwnerController->HighPingDelegate.RemoveDynamic(this, &ThisClass::OnPingTooHigh);
	}
}

void AWeapon::OnRep_WeaponState()
{
	OnWeaponStateSet();
}
