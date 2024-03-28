// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon/Weapon.h"
#include "Flag.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API AFlag : public AWeapon
{
	GENERATED_BODY()

private:
	AFlag();

private:
	virtual void Dropped() final;
	virtual void OnEquipped() final;
	virtual void OnDropped() final;

private:
	UPROPERTY(VisibleAnywhere, Category = "Flag")
	TObjectPtr<UStaticMeshComponent> FlagMesh;
};
