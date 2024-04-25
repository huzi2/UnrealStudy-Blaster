// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "BlasterHUD.generated.h"

class UCharacterOverlay;
class UAnnouncement;
class UElimAnnouncement;

/**
 * 십자선에 사용할 텍스쳐 정보 구조체
 */
USTRUCT(BlueprintType)
struct FHUDPackage
{
	GENERATED_BODY()

public:
	// 십자선 텍스쳐들
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
	// 십자선의 퍼짐 정도
	double CrosshairSpread;
	// 십자선 색깔
	FLinearColor CrosshairsColor;
};

/**
 * 플레이어의 정보와 십자선을 표시할 HUD 클래스
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

	// 화면에 UI 표시
	void AddCharacterOverlay();
	void AddAnnouncement();
	void AddElimAnnouncement(const FString& AttackerName, const FString& VictimName);

private:
	// 초기화
	void PollInit();

	// 십자선
	void DrawCrosshair(UTexture2D* Texture, const FVector2D& ViewportCenter, const FVector2D& Spread, const FLinearColor& CrosshairColor);

	// 알림 메시지 종료
	UFUNCTION()
	void ElimAnnouncementTimerFinished(UElimAnnouncement* MsgToRemove);

private:
	// 십자선의 최대 벌어짐 정도
	UPROPERTY(EditAnywhere)
	double CrosshairSpreadMax = 16.0;

	// 알림 메시지가 지속될 시간
	UPROPERTY(EditAnywhere, Category = "Announcements")
	float ElimAnnouncementTime = 50.f;

	// UI 클래스
	UPROPERTY(EditAnywhere, Category = "Player Stats")
	TSubclassOf<UUserWidget> CharacterOverlayClass;
	UPROPERTY(EditAnywhere, Category = "Announcements")
	TSubclassOf<UUserWidget> AnnouncementClass;
	UPROPERTY(EditAnywhere, Category = "Announcements")
	TSubclassOf<UElimAnnouncement> ElimAnnouncementClass;

	// 십자선 정보
	FHUDPackage HUDPackage;

	// 참조 변수
	UPROPERTY()
	TObjectPtr<APlayerController> OwningPlayer;
	UPROPERTY()
	TObjectPtr<UCharacterOverlay> CharacterOverlay;
	UPROPERTY()
	TObjectPtr<UAnnouncement> Announcement;
	UPROPERTY()
	TArray<UElimAnnouncement*> ElimMessages;
};
