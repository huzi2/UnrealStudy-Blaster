// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

// 헤드샷 등을 구현하기위해 커스텀 오브젝트 채널을 만들었음. 설명이 명확하기 위해 디파인
#define ECC_SkeletalMesh ECollisionChannel::ECC_GameTraceChannel1
#define ECC_HitBox ECollisionChannel::ECC_GameTraceChannel2
