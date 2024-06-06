// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon/Projectile.h"
#include "ProjectileBullet.generated.h"

/**
 * ���� �⺻���� �Ѿ� �߻�ü Ŭ����
 */
UCLASS()
class BLASTER_API AProjectileBullet : public AProjectile
{
	GENERATED_BODY()
	
private:
	AProjectileBullet();

private:

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& Event) final;
#endif

	virtual void BeginPlay() final;
	virtual void OnHit(UPrimitiveComponent* HItComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit) final;
};
