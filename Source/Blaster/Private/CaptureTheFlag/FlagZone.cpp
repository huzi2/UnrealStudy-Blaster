// Fill out your copyright notice in the Description page of Project Settings.

#include "CaptureTheFlag/FlagZone.h"
#include "Components/SphereComponent.h"
#include "Weapon/Flag.h"
#include "GameMode/CaptureTheFlagGameMode.h"

AFlagZone::AFlagZone()
{
	PrimaryActorTick.bCanEverTick = false;

	ZoneSphere = CreateDefaultSubobject<USphereComponent>(TEXT("ZoneSphere"));
	SetRootComponent(ZoneSphere);
}

void AFlagZone::BeginPlay()
{
	Super::BeginPlay();

	if (ZoneSphere)
	{
		ZoneSphere->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnSphereOverlap);
	}
}

void AFlagZone::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!GetWorld()) return;

	// ����� �ٸ� �� ������� �����Դ��� Ȯ��
	if (AFlag* OverlappingFlag = Cast<AFlag>(OtherActor))
	{
		if (OverlappingFlag->GetTeam() != Team)
		{
			// ���Ӹ�忡�� ���� ó�� ��û
			if (ACaptureTheFlagGameMode* GameMode = GetWorld()->GetAuthGameMode<ACaptureTheFlagGameMode>())
			{
				GameMode->FlagCapture(OverlappingFlag, this);
			}

			// ����� ��ġ�� ���� ��ġ�� �ǵ�����.
			OverlappingFlag->ResetFlag();
		}
	}
}
