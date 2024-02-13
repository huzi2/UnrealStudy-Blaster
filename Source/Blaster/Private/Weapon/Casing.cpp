// Fill out your copyright notice in the Description page of Project Settings.

#include "Weapon/Casing.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"

ACasing::ACasing()
	: ShellEjectionImpulse(10.f)
{
	PrimaryActorTick.bCanEverTick = false;

	CasingMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CasingMesh"));
	SetRootComponent(CasingMesh);
	CasingMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	CasingMesh->SetSimulatePhysics(true);
	CasingMesh->SetEnableGravity(true);
	// 강체 충돌을 활성화해서 충돌 이벤트가 발생하도록 함
	CasingMesh->SetNotifyRigidBodyCollision(true);
}

void ACasing::BeginPlay()
{
	Super::BeginPlay();

	if (CasingMesh)
	{
		// 중요하지 않은 오브젝트이므로 충돌처리를 각 클라에서 수행해도됨
		CasingMesh->OnComponentHit.AddDynamic(this, &ThisClass::OnHit);
		CasingMesh->AddImpulse(GetActorForwardVector() * ShellEjectionImpulse);
	}
}

void ACasing::OnHit(UPrimitiveComponent* HItComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (ShellSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ShellSound, GetActorLocation());
	}

	Destroy();
}
