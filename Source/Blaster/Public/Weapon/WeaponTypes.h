#pragma once

// 무기의 라인 트레이스 최대 사거리
constexpr double TRACE_LENGTH = 80000.0;

// 무기의 외곽 강조용으로 사용할 스텐실 버퍼 값
constexpr int32 CUSTOM_DEPTH_PURPLE = 250;
constexpr int32 CUSTOM_DEPTH_BLUE = 251;
constexpr int32 CUSTOM_DEPTH_TAN = 252;

UENUM(BlueprintType)
enum class EWeaponType : uint8
{
	EWT_AssaultRifle		UMETA(DisplayName = "Assault Rifle"),
	EWT_RocketLauncher		UMETA(DisplayName = "Rocket Launcher"),
	EWT_Pistol				UMETA(DisplayName = "Pistol"),
	EWT_SubmachineGun		UMETA(DisplayName = "Submachine Gun"),
	EWT_ShotGun				UMETA(DisplayName = "ShotGun"),
	EWT_SniperRifle			UMETA(DisplayName = "Sniper Rifle"),
	EWT_GrenaderLauncher	UMETA(DisplayName = "Grenader Launcher"),
	EWT_Flag				UMETA(DisplayName = "Flag"),

	EWT_MAX					UMETA(DisplayName = "DefaultMAX")
};
