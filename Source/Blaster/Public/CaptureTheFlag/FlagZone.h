// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BlasterTypes/Team.h"
#include "FlagZone.generated.h"

class USphereComponent;

/**
 * ±ê¹ß °´Ã¼ Å¬·¡½º
 */
UCLASS()
class BLASTER_API AFlagZone : public AActor
{
	GENERATED_BODY()
	
private:
	AFlagZone();

private:
	virtual void BeginPlay() final;

public:
	FORCEINLINE ETeam GetTeam() const { return Team; }

private:
	UFUNCTION()
	void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

private:
	// ±ê¹ßÀ» ÁÖ¿ï ¼ö ÀÖ´Â ¹üÀ§
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USphereComponent> ZoneSphere;
	// ±ê¹ßÀÇ ÆÀ
	UPROPERTY(EditAnywhere)
	ETeam Team;
};
