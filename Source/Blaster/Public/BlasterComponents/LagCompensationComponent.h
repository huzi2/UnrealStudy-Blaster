// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LagCompensationComponent.generated.h"

class ABlasterPlayerController;

// 프레임 정보 안의 내용
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

// 특정 시간의 프레임 정보
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

// 서버 되감기 결과
USTRUCT(BlueprintType)
struct FServerSideRewindResult
{
	GENERATED_BODY()

public:
	// 맞았는가?
	UPROPERTY()
	bool bHitConfirmed;

	// 헤드샷이었는가?
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
	// 지정된 시간동안의 프레임 정보를 저장하는 이중연결리스트
	TDoubleLinkedList<FFramePackage> FrameHistory;
};
