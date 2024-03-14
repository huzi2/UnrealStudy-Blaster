// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LagCompensationComponent.generated.h"

class ABlasterPlayerController;

// ������ ���� ���� ����
USTRUCT(BlueprintType)
struct FBoxInformation
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FVector Location;

	UPROPERTY()
	FRotator Rotation;

	UPROPERTY()
	FVector BoxExtent;
};

// Ư�� �ð��� ������ ����
USTRUCT(BlueprintType)
struct FFramePackage
{
	GENERATED_BODY()

public:
	UPROPERTY()
	double Time;

	UPROPERTY()
	TMap<FName, FBoxInformation> HitBoxInfo;
};

// ���� �ǰ��� ���
USTRUCT(BlueprintType)
struct FServerSideRewindResult
{
	GENERATED_BODY()

public:
	// �¾Ҵ°�?
	UPROPERTY()
	bool bHitConfirmed;

	// ��弦�̾��°�?
	UPROPERTY()
	bool bHeadShot;
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class BLASTER_API ULagCompensationComponent : public UActorComponent
{
	GENERATED_BODY()

private:
	ULagCompensationComponent();
	friend class ABlasterCharacter;

private:
	virtual void BeginPlay() final;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) final;

public:
	void ShowFramePackage(const FFramePackage& Package, const FColor& Color) const;
	FServerSideRewindResult ServerSideRewind(ABlasterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation, double HitTime) const;

private:
	void Init();
	void SaveFramePackage(FFramePackage& Package);
	FFramePackage InterpBetweenFrames(const FFramePackage& OlderFrame, const FFramePackage& YoungerFrame, double HitTime) const;
	FServerSideRewindResult ConfirmHit(const FFramePackage& Package, ABlasterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation) const;
	void CacheBoxPositions(ABlasterCharacter* HitCharacter, FFramePackage& OutFramePackage) const;
	void MoveBoxes(ABlasterCharacter* HitCharacter, const FFramePackage& Package) const;
	void ResetHitBoxes(ABlasterCharacter* HitCharacter, const FFramePackage& Package) const;
	void EnableCharacterMeshCollision(ABlasterCharacter* HitCharacter, ECollisionEnabled::Type CollisionEnabled) const;

private:
	UPROPERTY(EditAnywhere)
	double MaxRecordTime;

	UPROPERTY()
	TObjectPtr<ABlasterCharacter> Character;

	UPROPERTY()
	TObjectPtr<ABlasterPlayerController> Controller;

private:
	// ������ �ð������� ������ ������ �����ϴ� ���߿��Ḯ��Ʈ
	TDoubleLinkedList<FFramePackage> FrameHistory;
};
