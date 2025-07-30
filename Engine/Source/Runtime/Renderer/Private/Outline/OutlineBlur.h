// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "RenderGraphBuilder.h"

void AddOutlineBlurCombinePass(FRDGBuilder& GraphBuilder, const FViewInfo& View, FRDGTextureRef BlurredOutline, FRDGTextureRef NoBlurredOutline);

void AddOutlineBlurPass(FRDGBuilder& GraphBuilder, const FViewInfo& View);