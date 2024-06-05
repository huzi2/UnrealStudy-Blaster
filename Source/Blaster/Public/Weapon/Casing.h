// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Casing.generated.h"

class USoundCue;

/**
 * ÅºÇÇ Å¬·¡½º
 */
UCLASS()
class BLASTER_API ACasing : public AActor
{
	GENERATED_BODY()
	
private:
	ACasing();

private:
	virtual void BeginPlay() final;

private:
	UFUNCTION()
	void OnHit(UPrimitiveComponent* HItComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

private:
	// ÅºÇÇ ¸Þ½¬
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> CasingMesh;

	// ÅºÇÇ ¼Ò¸®
	UPROPERTY(EditAnywhere)
	TObjectPtr<USoundCue> ShellSound;

	// ÅºÇÇ¿¡ °¡ÇÒ Èû
	UPROPERTY(EditAnywhere)
	float ShellEjectionImpulse = 10.f;
};
