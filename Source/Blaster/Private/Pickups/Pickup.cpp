// Fill out your copyright notice in the Description page of Project Settings.

#include "Pickups/Pickup.h"
#include "Components/SphereComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"
#include "Weapon/WeaponTypes.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"

APickup::APickup()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	OverlapShpere = CreateDefaultSubobject<USphereComponent>(TEXT("OverlapShpere"));
	OverlapShpere->SetupAttachment(GetRootComponent());
	OverlapShpere->SetSphereRadius(150.f);
	OverlapShpere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	OverlapShpere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	OverlapShpere->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
	OverlapShpere->AddLocalOffset(FVector(0.f, 0.f, 85.f));

	PickupMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PickupMesh"));
	PickupMesh->SetupAttachment(OverlapShpere);
	PickupMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PickupMesh->SetRelativeScale3D(FVector(5.f, 5.f, 5.f));
	PickupMesh->SetRenderCustomDepth(true);
	PickupMesh->SetCustomDepthStencilValue(CUSTOM_DEPTH_PURPLE);

	PickupEffectComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("PickupEffectComponent"));
	PickupEffectComponent->SetupAttachment(GetRootComponent());
}

void APickup::BeginPlay()
{
	Super::BeginPlay();

	// �浹 üũ�� ������
	if (HasAuthority())
	{
		// ���� �ణ�� �ð��� �ְ� �Լ��� ���ε�
		// �̷����ϴ� ������ �ٷ� �浹ó���� ���ε��ϸ� �Ⱦ��� ������������ �浹ó���Ǹ鼭 �����Ǿ APickupSpawnPoint���� �Ⱦ� ��ü�� ���ε��� �ð��� ����.
		GetWorldTimerManager().SetTimer(BindOverlapTimer, this, &ThisClass::BindOverlapTimerFinished, BindOverlapTime);
	}
}

void APickup::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// ������ ��� ȸ��
	if (PickupMesh)
	{
		PickupMesh->AddWorldRotation(FRotator(0.f, BaseTurnRate * DeltaTime, 0.f));
	}
}

void APickup::Destroyed()
{
	if (PickupSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, PickupSound, GetActorLocation());
	}

	if (PickupEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, PickupEffect, GetActorLocation(), GetActorRotation());
	}

	Super::Destroyed();
}

void APickup::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
}

void APickup::BindOverlapTimerFinished()
{
	// ������ �浹 �Լ� ���ε�
	if (OverlapShpere)
	{
		OverlapShpere->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnSphereOverlap);
	}
}
