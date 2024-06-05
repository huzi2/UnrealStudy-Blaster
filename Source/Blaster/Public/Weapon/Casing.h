// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Casing.generated.h"

class USoundCue;

/**
 * ź�� Ŭ����
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
	// ź�� �޽�
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> CasingMesh;

	// ź�� �Ҹ�
	UPROPERTY(EditAnywhere)
	TObjectPtr<USoundCue> ShellSound;

	// ź�ǿ� ���� ��
	UPROPERTY(EditAnywhere)
	float ShellEjectionImpulse = 10.f;
};
