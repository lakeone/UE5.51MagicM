// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "ScreenPass.h"

class FSceneDownsampleChain;

enum class EBlurQuality : uint32
{
	Disabled,
	Q1,
	Q2,
	Q3,
	Q4,
	Q5,
	MAX
};

struct FBlurPassesSetting
{
	EBlurQuality BlurQuality = EBlurQuality::Q5;
	float BlurSizeScale = 4.0;
	// 应该是让靠近中间的采样点的权重更高，默认0为关闭
	// 详见：CVarBloomCross
	float CrossBlur = 0.0;
	float BlurSizes[6] =
	{
		64,
		30,
		10,
		2,
		1,
		0.3
	};
	FLinearColor BlurTints[6]
	{
		FLinearColor(0.061, 0.061, 0.061, 0.061),
		FLinearColor(0.066, 0.066, 0.066, 0.066),
		FLinearColor(0.066, 0.066, 0.066, 0.066),
		FLinearColor(0.117, 0.117, 0.117, 0.117),
		FLinearColor(0.138, 0.138, 0.138, 0.138),
		FLinearColor(0.346, 0.346, 0.346, 0.346),
	};
};

FScreenPassTexture AddBlurPasses(FRDGBuilder& GraphBuilder, const FViewInfo& View, const FSceneDownsampleChain* SceneDownsampleChain, const FBlurPassesSetting& Setting);

FScreenPassTexture AddTextureBlurPass(FRDGBuilder& GraphBuilder, const FViewInfo& View, FRDGTexture* InTexture, const FBlurPassesSetting& Setting);