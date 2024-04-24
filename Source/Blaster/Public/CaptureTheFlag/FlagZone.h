// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BlasterTypes/Team.h"
#include "FlagZone.generated.h"

class USphereComponent;

/**
 * ��� ��ü Ŭ����
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
	// ����� �ֿ� �� �ִ� ����
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USphereComponent> ZoneSphere;
	// ����� ��
	UPROPERTY(EditAnywhere)
	ETeam Team;
};
