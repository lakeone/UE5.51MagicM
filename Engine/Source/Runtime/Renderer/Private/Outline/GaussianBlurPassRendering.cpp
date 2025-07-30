#include "GaussianBlurPassRendering.h"
#include "SceneRendering.h"
#include "PostProcess/PostProcessDownsample.h"
#include "PostProcess/PostProcessEyeAdaptation.h"
#include "PostProcess/PostProcessWeightedSampleSum.h"
#include "ScreenPass.h"
 
 FScreenPassTexture AddBlurPasses(FRDGBuilder& GraphBuilder, const FViewInfo& View, const FSceneDownsampleChain* SceneDownsampleChain, const FBlurPassesSetting& Setting)
 {
 	check(SceneDownsampleChain);
 
 	const EBlurQuality BlurQuality = Setting.BlurQuality;
 
 	FScreenPassTexture PassOutputs;
 	if (BlurQuality != EBlurQuality::Disabled)
 	{
 		RDG_EVENT_SCOPE(GraphBuilder, "Blur");
 
 		const FVector2D CrossCenterWeight(FMath::Max(Setting.CrossBlur, 0.0f), FMath::Abs(Setting.CrossBlur));
 
 		const uint32 BlurQualityIndex = static_cast<uint32>(BlurQuality);
 		const uint32 BlurQualityCountMax = static_cast<uint32>(EBlurQuality::MAX);
 
 		const uint32 BlurQualityToSceneDownsampleStage[] =
 		{
 			static_cast<uint32>(-1), // Disabled (sentinel entry to preserve indices)
 			3, // Q1
 			3, // Q2
 			4, // Q3
 			5, // Q4
 			6  // Q5
 		};
 		
 		static_assert(UE_ARRAY_COUNT(Setting.BlurSizes) == BlurQualityCountMax, "Array must be one less than the number of blur quality entries.");
 		static_assert(UE_ARRAY_COUNT(Setting.BlurTints) == BlurQualityCountMax, "Array must be one less than the number of blur quality entries.");
 		static_assert(UE_ARRAY_COUNT(BlurQualityToSceneDownsampleStage) == BlurQualityCountMax, "Array must be one less than the number of blur quality entries.");
 
 		check(BlurQualityIndex < BlurQualityCountMax);
 
 		// Use blur quality to select the number of downsample stages to use for blur.
 		const uint32 BlurStageCount = BlurQualityToSceneDownsampleStage[BlurQualityIndex];
 
 		FLinearColor TotalBlurTint = FLinearColor::Transparent;
 		for (uint32 i = 0; i < BlurStageCount; ++i)
 		{
 			TotalBlurTint += Setting.BlurTints[i];
 		}
 		const FLinearColor TintScale = (FLinearColor::White / TotalBlurTint);
 
 		for (uint32 StageIndex = 0, SourceIndex = BlurQualityCountMax - 1; StageIndex < BlurStageCount; ++StageIndex, --SourceIndex)
 		{
 			float BlurSize = Setting.BlurSizes[StageIndex];
 			FLinearColor BlurTint = Setting.BlurTints[StageIndex];
 			if (BlurSize > SMALL_NUMBER)
 			{
 				FGaussianBlurInputs PassInputs;
 				PassInputs.NameX = TEXT("BlurX");
 				PassInputs.NameY = TEXT("BlurY");
 				PassInputs.Filter = SceneDownsampleChain->GetTexture(SourceIndex);
 				PassInputs.Additive = PassOutputs;
 				PassInputs.CrossCenterWeight = FVector2f(CrossCenterWeight);	// LWC_TODO: Precision loss
 				PassInputs.KernelSizePercent = BlurSize * Setting.BlurSizeScale;
 				PassInputs.TintColor = BlurTint * TintScale;
 
 				PassOutputs = AddGaussianBlurPass(GraphBuilder, View, PassInputs);
 			}
 		}
 	}
 
 	return PassOutputs;
 }
 
 FScreenPassTexture AddTextureBlurPass(FRDGBuilder& GraphBuilder, const FViewInfo& View, FRDGTexture* InTexture, const FBlurPassesSetting& Setting)
 {
 	FSceneDownsampleChain SceneDownsampleChain;
 	const FEyeAdaptationParameters EyeAdaptationParameters = GetEyeAdaptationParameters(View);
	FScreenPassTextureSlice Texture = FScreenPassTextureSlice::CreateFromScreenPassTexture(GraphBuilder, FScreenPassTexture(InTexture));
 	if (Texture.IsValid())
 	{
 		SceneDownsampleChain.Init(
 			GraphBuilder, View,
 			EyeAdaptationParameters,
 			Texture,
 			EDownsampleQuality::High,
 			false);
 		return AddBlurPasses(GraphBuilder, View, &SceneDownsampleChain, Setting);
 	}
 	return FScreenPassTexture();
 }