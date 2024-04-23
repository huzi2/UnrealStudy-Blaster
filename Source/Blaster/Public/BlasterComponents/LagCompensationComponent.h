// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LagCompensationComponent.generated.h"

class ABlasterCharacter;
class ABlasterPlayerController;
class AWeapon;
class AProjectile;

/**
* 프레임 정보 안의 내용
*/
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

/**
* 특정 시간의 프레임 정보
*/
USTRUCT(BlueprintType)
struct FFramePackage
{
	GENERATED_BODY()

public:
	UPROPERTY()
	double Time;

	// 맞은 모든 히트박스를 저장
	UPROPERTY()
	TMap<FName, FBoxInformation> HitBoxInfo;

	// 샷건에서 확인하기 위한 변수
	UPROPERTY()
	TObjectPtr<ABlasterCharacter> Character;
};

/**
* 서버 되감기 결과
*/
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

/**
* 샷건용 서버 되감기 결과
*/
USTRUCT(BlueprintType)
struct FShotgunServerSideRewindResult
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TMap<ABlasterCharacter*, uint32> HeadShots;

	UPROPERTY()
	TMap<ABlasterCharacter*, uint32> BodyShots;
};

/**
* 총을 쏠 때 서버 되감기를 통해서 동기화하는 컴포넌트 클래스
*/
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class BLASTER_API ULagCompensationComponent : public UActorComponent
{
	GENERATED_BODY()

private:
	ULagCompensationComponent();
	friend ABlasterCharacter;

private:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) final;

public:
	// 공격 시 서버 되감기 요청
	UFUNCTION(Server, Reliable)
	void ServerScoreRequest(ABlasterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation, double HitTime, AWeapon* DamageCauser) const;

	// 발사체 공격 시 서버 되감기 요청
	UFUNCTION(Server, Reliable)
	void ServerProjectileScoreRequest(ABlasterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize100& InitialVelocity, double HitTime, AProjectile* DamageCauser) const;

	// 샷건 공격 시 서버 되감기 요청
	UFUNCTION(Server, Reliable)
	void ServerShotgunScoreRequest(const TArray<ABlasterCharacter*>& HitCharacters, const FVector_NetQuantize& TraceStart, const TArray<FVector_NetQuantize>& HitLocations, double HitTime, AWeapon* DamageCauser) const;
	
private:
	void Init();

	// 일반 공격 서버 되감기
	FServerSideRewindResult ServerSideRewind(ABlasterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation, double HitTime) const;
	FServerSideRewindResult ConfirmHit(const FFramePackage& Package, ABlasterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation) const;

	// 발사체 공격 서버 되감기
	FServerSideRewindResult ProjectileServerSideRewind(ABlasterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize100& InitialVelocity, double HitTime) const;
	FServerSideRewindResult ProjectileConfirmHit(const FFramePackage& Package, ABlasterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize100& InitialVelocity) const;

	// 샷건 공격 서버 되감기
	FShotgunServerSideRewindResult ShotgunServerSideRewind(const TArray<ABlasterCharacter*>& HitCharacters, const FVector_NetQuantize& TraceStart, const TArray<FVector_NetQuantize>& HitLocations, double HitTime) const;
	FShotgunServerSideRewindResult ShotgunConfirmHit(const TArray<FFramePackage>& Packages, const FVector_NetQuantize& TraceStart, const TArray<FVector_NetQuantize>& HitLocations) const;

	// 서버 되감기 구현
	void SaveFramePackage();
	void SaveFramePackage(FFramePackage& Package);
	FFramePackage GetFrameToCheck(ABlasterCharacter* HitCharacter, double HitTime) const;
	FFramePackage InterpBetweenFrames(const FFramePackage& OlderFrame, const FFramePackage& YoungerFrame, double HitTime) const;
	void CacheBoxPositions(ABlasterCharacter* HitCharacter, FFramePackage& OutFramePackage) const;
	void MoveBoxes(ABlasterCharacter* HitCharacter, const FFramePackage& Package) const;
	void ResetHitBoxes(ABlasterCharacter* HitCharacter, const FFramePackage& Package) const;
	void EnableCharacterMeshCollision(ABlasterCharacter* HitCharacter, ECollisionEnabled::Type CollisionEnabled) const;

	// 디버깅용
	void ShowFramePackage(const FFramePackage& Package, const FColor& Color) const;
	void DrawBox(const FHitResult& ConfirmHitResult, const FColor& Color) const;

private:
	// 최대 녹화 시간
	UPROPERTY(EditAnywhere)
	double MaxRecordTime = 4.0;

	// 디버깅 표시 여부
	UPROPERTY(EditAnywhere)
	bool bDrawDebug = false;

	// 지정된 시간동안의 프레임 정보를 저장하는 이중연결리스트
	TDoubleLinkedList<FFramePackage> FrameHistory;

	// 참조 변수들
	UPROPERTY()
	TObjectPtr<ABlasterCharacter> Character;
	UPROPERTY()
	TObjectPtr<ABlasterPlayerController> Controller;
};
