// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon/Weapon.h"
#include "Flag.generated.h"

/**
 * 팀 깃발 게임에서 사용할 깃발 클래스
 */
UCLASS()
class BLASTER_API AFlag : public AWeapon
{
	GENERATED_BODY()

private:
	AFlag();

private:
	virtual void BeginPlay() final;

	// AWeapon에서 상속
	virtual void Dropped() final;
	virtual void OnEquipped() final;
	virtual void OnDropped() final;

public:
	// 깃발을 목표 지점에 넣은 뒤 깃발 초기화
	void ResetFlag();

private:
	// 깃발 메쉬
	UPROPERTY(VisibleAnywhere, Category = "Flag")
	TObjectPtr<UStaticMeshComponent> FlagMesh;

	// 깃발 시작 위치
	FTransform InitialTransform;
};
