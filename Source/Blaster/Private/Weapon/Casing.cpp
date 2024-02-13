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
	// ��ü �浹�� Ȱ��ȭ�ؼ� �浹 �̺�Ʈ�� �߻��ϵ��� ��
	CasingMesh->SetNotifyRigidBodyCollision(true);
}

void ACasing::BeginPlay()
{
	Super::BeginPlay();

	if (CasingMesh)
	{
		// �߿����� ���� ������Ʈ�̹Ƿ� �浹ó���� �� Ŭ�󿡼� �����ص���
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
