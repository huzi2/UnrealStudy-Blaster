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

	// 충돌 체크는 서버만
	if (HasAuthority())
	{
		// 아주 약간의 시간을 주고 함수를 바인드
		// 이렇게하는 이유는 바로 충돌처리를 바인드하면 픽업이 생성되지마자 충돌처리되면서 삭제되어서 APickupSpawnPoint에서 픽업 객체에 바인드할 시간이 없다.
		GetWorldTimerManager().SetTimer(BindOverlapTimer, this, &ThisClass::BindOverlapTimerFinished, BindOverlapTime);
	}
}

void APickup::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 아이템 계속 회전
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
	// 아이템 충돌 함수 바인드
	if (OverlapShpere)
	{
		OverlapShpere->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnSphereOverlap);
	}
}
