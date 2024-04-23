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
* ������ ���� ���� ����
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
* Ư�� �ð��� ������ ����
*/
USTRUCT(BlueprintType)
struct FFramePackage
{
	GENERATED_BODY()

public:
	UPROPERTY()
	double Time;

	// ���� ��� ��Ʈ�ڽ��� ����
	UPROPERTY()
	TMap<FName, FBoxInformation> HitBoxInfo;

	// ���ǿ��� Ȯ���ϱ� ���� ����
	UPROPERTY()
	TObjectPtr<ABlasterCharacter> Character;
};

/**
* ���� �ǰ��� ���
*/
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

/**
* ���ǿ� ���� �ǰ��� ���
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
* ���� �� �� ���� �ǰ��⸦ ���ؼ� ����ȭ�ϴ� ������Ʈ Ŭ����
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
	// ���� �� ���� �ǰ��� ��û
	UFUNCTION(Server, Reliable)
	void ServerScoreRequest(ABlasterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation, double HitTime, AWeapon* DamageCauser) const;

	// �߻�ü ���� �� ���� �ǰ��� ��û
	UFUNCTION(Server, Reliable)
	void ServerProjectileScoreRequest(ABlasterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize100& InitialVelocity, double HitTime, AProjectile* DamageCauser) const;

	// ���� ���� �� ���� �ǰ��� ��û
	UFUNCTION(Server, Reliable)
	void ServerShotgunScoreRequest(const TArray<ABlasterCharacter*>& HitCharacters, const FVector_NetQuantize& TraceStart, const TArray<FVector_NetQuantize>& HitLocations, double HitTime, AWeapon* DamageCauser) const;
	
private:
	void Init();

	// �Ϲ� ���� ���� �ǰ���
	FServerSideRewindResult ServerSideRewind(ABlasterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation, double HitTime) const;
	FServerSideRewindResult ConfirmHit(const FFramePackage& Package, ABlasterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation) const;

	// �߻�ü ���� ���� �ǰ���
	FServerSideRewindResult ProjectileServerSideRewind(ABlasterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize100& InitialVelocity, double HitTime) const;
	FServerSideRewindResult ProjectileConfirmHit(const FFramePackage& Package, ABlasterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize100& InitialVelocity) const;

	// ���� ���� ���� �ǰ���
	FShotgunServerSideRewindResult ShotgunServerSideRewind(const TArray<ABlasterCharacter*>& HitCharacters, const FVector_NetQuantize& TraceStart, const TArray<FVector_NetQuantize>& HitLocations, double HitTime) const;
	FShotgunServerSideRewindResult ShotgunConfirmHit(const TArray<FFramePackage>& Packages, const FVector_NetQuantize& TraceStart, const TArray<FVector_NetQuantize>& HitLocations) const;

	// ���� �ǰ��� ����
	void SaveFramePackage();
	void SaveFramePackage(FFramePackage& Package);
	FFramePackage GetFrameToCheck(ABlasterCharacter* HitCharacter, double HitTime) const;
	FFramePackage InterpBetweenFrames(const FFramePackage& OlderFrame, const FFramePackage& YoungerFrame, double HitTime) const;
	void CacheBoxPositions(ABlasterCharacter* HitCharacter, FFramePackage& OutFramePackage) const;
	void MoveBoxes(ABlasterCharacter* HitCharacter, const FFramePackage& Package) const;
	void ResetHitBoxes(ABlasterCharacter* HitCharacter, const FFramePackage& Package) const;
	void EnableCharacterMeshCollision(ABlasterCharacter* HitCharacter, ECollisionEnabled::Type CollisionEnabled) const;

	// ������
	void ShowFramePackage(const FFramePackage& Package, const FColor& Color) const;
	void DrawBox(const FHitResult& ConfirmHitResult, const FColor& Color) const;

private:
	// �ִ� ��ȭ �ð�
	UPROPERTY(EditAnywhere)
	double MaxRecordTime = 4.0;

	// ����� ǥ�� ����
	UPROPERTY(EditAnywhere)
	bool bDrawDebug = false;

	// ������ �ð������� ������ ������ �����ϴ� ���߿��Ḯ��Ʈ
	TDoubleLinkedList<FFramePackage> FrameHistory;

	// ���� ������
	UPROPERTY()
	TObjectPtr<ABlasterCharacter> Character;
	UPROPERTY()
	TObjectPtr<ABlasterPlayerController> Controller;
};
