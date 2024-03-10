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

	// 충돌 처리는 서버에서만 진행하도록 함
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

	// 탄약 사용
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
	// 포스트 프로세스와 포스트 프로세스 머티리얼을 통한 스텐실 효과를 사용한 무기 외곽 강조 처리
	if (WeaponMesh)
	{
		WeaponMesh->SetRenderCustomDepth(bEnable);

	}
}

void AWeapon::SpendRound()
{
	// 서버에서 호출하는 Fire()에서 사용하기에 서버에서만 사용되는 함수
	// 총알 사용. 레플리케이션 변수라서 클라이언트들에게도 적용되고 OnRep_Ammo() 함수 수행
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

		// 무기 외곽선 제거
		EnableCustomDepth(false);
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

		EnableCustomDepth(true);
		WeaponMesh->SetCustomDepthStencilValue(CUSTOM_DEPTH_TAN);
		WeaponMesh->MarkRenderStateDirty();
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
	}

	// 무기 외곽 강조 효과 킴
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
	// 클라이언트에 탄약 적용
	SetHUDAmmo();

	// 샷건 장전 중 탄약 꽉찼을 때 애니메이션 변경하는걸 컴뱃컴포넌트에서 수행하는데 서버에서만 하고있음
	// 클라에서 하는건 여기서 하드코딩으로 작업
	if (BlasterOwnerCharacter && BlasterOwnerCharacter->GetCombat() && IsFull())
	{
		BlasterOwnerCharacter->GetCombat()->JumpToShotgunEnd();
	}
}
