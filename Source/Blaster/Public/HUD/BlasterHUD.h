// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "BlasterHUD.generated.h"

class UCharacterOverlay;
class UAnnouncement;
class UElimAnnouncement;

/**
 * ���ڼ��� ����� �ؽ��� ���� ����ü
 */
USTRUCT(BlueprintType)
struct FHUDPackage
{
	GENERATED_BODY()

public:
	// ���ڼ� �ؽ��ĵ�
	UPROPERTY()
	TObjectPtr<UTexture2D> CrosshairsCenter;
	UPROPERTY()
	TObjectPtr<UTexture2D> CrosshairsLeft;
	UPROPERTY()
	TObjectPtr<UTexture2D> CrosshairsRight;
	UPROPERTY()
	TObjectPtr<UTexture2D> CrosshairsTop;
	UPROPERTY()
	TObjectPtr<UTexture2D> CrosshairsBottom;
	// ���ڼ��� ���� ����
	double CrosshairSpread;
	// ���ڼ� ����
	FLinearColor CrosshairsColor;
};

/**
 * �÷��̾��� ������ ���ڼ��� ǥ���� HUD Ŭ����
 */
UCLASS()
class BLASTER_API ABlasterHUD : public AHUD
{
	GENERATED_BODY()

private:
	virtual void DrawHUD() final;

public:
	FORCEINLINE UCharacterOverlay* GetCharacterOverlay() const { return CharacterOverlay; }
	FORCEINLINE UAnnouncement* GetAnnouncement() const { return Announcement; }
	FORCEINLINE void SetHUDPackage(const FHUDPackage& Package) { HUDPackage = Package; }

	// ȭ�鿡 UI ǥ��
	void AddCharacterOverlay();
	void AddAnnouncement();
	void AddElimAnnouncement(const FString& AttackerName, const FString& VictimName);

private:
	// �ʱ�ȭ
	void PollInit();

	// ���ڼ�
	void DrawCrosshair(UTexture2D* Texture, const FVector2D& ViewportCenter, const FVector2D& Spread, const FLinearColor& CrosshairColor);

	// �˸� �޽��� ����
	UFUNCTION()
	void ElimAnnouncementTimerFinished(UElimAnnouncement* MsgToRemove);

private:
	// ���ڼ��� �ִ� ������ ����
	UPROPERTY(EditAnywhere)
	double CrosshairSpreadMax = 16.0;

	// �˸� �޽����� ���ӵ� �ð�
	UPROPERTY(EditAnywhere, Category = "Announcements")
	float ElimAnnouncementTime = 50.f;

	// UI Ŭ����
	UPROPERTY(EditAnywhere, Category = "Player Stats")
	TSubclassOf<UUserWidget> CharacterOverlayClass;
	UPROPERTY(EditAnywhere, Category = "Announcements")
	TSubclassOf<UUserWidget> AnnouncementClass;
	UPROPERTY(EditAnywhere, Category = "Announcements")
	TSubclassOf<UElimAnnouncement> ElimAnnouncementClass;

	// ���ڼ� ����
	FHUDPackage HUDPackage;

	// ���� ����
	UPROPERTY()
	TObjectPtr<APlayerController> OwningPlayer;
	UPROPERTY()
	TObjectPtr<UCharacterOverlay> CharacterOverlay;
	UPROPERTY()
	TObjectPtr<UAnnouncement> Announcement;
	UPROPERTY()
	TArray<UElimAnnouncement*> ElimMessages;
};
