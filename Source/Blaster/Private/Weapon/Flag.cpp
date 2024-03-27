// Fill out your copyright notice in the Description page of Project Settings.

#include "Weapon/Flag.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"

AFlag::AFlag()
{
	FlagMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FlagMesh"));
	FlagMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	FlagMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetRootComponent(FlagMesh);

	AreaSphere->SetupAttachment(FlagMesh);
	PickupWidget->SetupAttachment(FlagMesh);
}
