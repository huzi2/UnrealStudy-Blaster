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

	// 깃발을 다른 팀 깃발존에 가져왔는지 확인
	if (AFlag* OverlappingFlag = Cast<AFlag>(OtherActor))
	{
		if (OverlappingFlag->GetTeam() != Team)
		{
			// 게임모드에게 점수 처리 요청
			if (ACaptureTheFlagGameMode* GameMode = GetWorld()->GetAuthGameMode<ACaptureTheFlagGameMode>())
			{
				GameMode->FlagCapture(OverlappingFlag, this);
			}

			// 깃발의 위치를 원래 위치로 되돌린다.
			OverlappingFlag->ResetFlag();
		}
	}
}
