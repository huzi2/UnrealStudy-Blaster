// Fill out your copyright notice in the Description page of Project Settings.

#include "Pickups/SpeedPickup.h"
#include "Character/BlasterCharacter.h"
#include "BlasterComponents/BuffComponent.h"

ASpeedPickup::ASpeedPickup()
	: BaseSpeedBuff(1600.f)
	, CrouchSpeedBuff(850.f)
	, SpeedBuffTime(30.f)
{
}

void ASpeedPickup::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnSphereOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

	if (ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(OtherActor))
	{
		if (UBuffComponent* Buff = BlasterCharacter->GetBuff())
		{
			Buff->BuffSpeed(BaseSpeedBuff, CrouchSpeedBuff, SpeedBuffTime);
		}
	}

	Destroy();
}
