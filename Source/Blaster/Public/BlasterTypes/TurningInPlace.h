#pragma once

/**
 * 캐릭터 회전 정보
 */
UENUM(BlueprintType)
enum class ETurningInPlace : uint8
{
	ETIP_Left		UMETA(DisplayName = "Turning Left"),
	ETIP_Right		UMETA(DisplayName = "Turning Right"),
	ETIP_NotTurning UMETA(DisplayName = "Not Turning"),

	ETIP_MAX		UMETA(DisplayName = "DefaultMAX")
};
