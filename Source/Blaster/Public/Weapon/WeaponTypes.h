#pragma once

// ������ ���� Ʈ���̽� �ִ� ��Ÿ�
constexpr double TRACE_LENGTH = 80000.0;

// ������ �ܰ� ���������� ����� ���ٽ� ���� ��
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
