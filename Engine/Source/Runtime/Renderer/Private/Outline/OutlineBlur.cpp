#include "OutlineBlur.h"
#include "ScreenPass.h"
#include "DataDrivenShaderPlatformInfo.h"
#include "GaussianBlurPassRendering.h"
#include "PixelShaderUtils.h"
#include "SceneRendering.h"

class FOutlineBlurSetupPS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FOutlineBlurSetupPS);
	SHADER_USE_PARAMETER_STRUCT(FOutlineBlurSetupPS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, OutlineTexture)
		SHADER_PARAMETER(FScreenTransform, SvPositionToTextureUV)
		RENDER_TARGET_BINDING_SLOTS()
		END_SHADER_PARAMETER_STRUCT()

		static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}
};

IMPLEMENT_GLOBAL_SHADER(FOutlineBlurSetupPS, "/Engine/Private/Outline/OutlineBlur.usf", "OutlineBlurSetupPS", SF_Pixel);

FScreenPassRenderTarget AddOutlineBlurSetupPass(FRDGBuilder& GraphBuilder, const FViewInfo& View)
{
	const FSceneTextures& SceneTextures = View.GetSceneTextures();
	const FRDGTextureRef OutlineBufferA = SceneTextures.OutlineBufferA;

	FScreenPassRenderTarget Output;

	if (!OutlineBufferA)
	{
		return Output;
	}

	FRDGTextureDesc Desc = OutlineBufferA->Desc;
	Desc.Reset();
	Desc.Extent = OutlineBufferA->Desc.Extent;
	Desc.Flags &= ~(TexCreate_UAV | TexCreate_Presentable);
	Desc.Flags |= (TexCreate_RenderTargetable | TexCreate_NoFastClear);
	Desc.ClearValue = FClearValueBinding(FLinearColor(1, 1, 1, 1));
	Desc.Format = EPixelFormat::PF_B8G8R8A8;
	Output.Texture = GraphBuilder.CreateTexture(Desc, TEXT("SetupOutline"));
	Output.LoadAction = ERenderTargetLoadAction::ENoAction;
	Output.ViewRect.Max = OutlineBufferA->Desc.Extent;

	FOutlineBlurSetupPS::FParameters* PassParameters = GraphBuilder.AllocParameters<FOutlineBlurSetupPS::FParameters>();

	PassParameters->OutlineTexture = OutlineBufferA;
	PassParameters->SvPositionToTextureUV = (
		FScreenTransform::ChangeTextureBasisFromTo(FScreenPassTextureViewport(Output), FScreenTransform::ETextureBasis::TexelPosition, FScreenTransform::ETextureBasis::ViewportUV) *
		FScreenTransform::ChangeTextureBasisFromTo(FScreenPassTextureViewport(OutlineBufferA), FScreenTransform::ETextureBasis::ViewportUV, FScreenTransform::ETextureBasis::TextureUV));

	PassParameters->RenderTargets[0] = Output.GetRenderTargetBinding();

	auto PixelShader = View.ShaderMap->GetShader<FOutlineBlurSetupPS>();

	FPixelShaderUtils::AddFullscreenPass(
		GraphBuilder,
		View.ShaderMap,
		RDG_EVENT_NAME("SetupOutline %dx%d (PS)", Output.ViewRect.Width(), Output.ViewRect.Height()),
		PixelShader,
		PassParameters,
		Output.ViewRect);

	return MoveTemp(Output);
}

class FOutlineBlurCombinePS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FOutlineBlurCombinePS);
	SHADER_USE_PARAMETER_STRUCT(FOutlineBlurCombinePS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, BlurredOutlineTexture)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, NoBlurredOutlineTexture)
		SHADER_PARAMETER(FScreenTransform, SvPositionToTextureUV)
		RENDER_TARGET_BINDING_SLOTS()
		END_SHADER_PARAMETER_STRUCT()

		static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}
};

IMPLEMENT_GLOBAL_SHADER(FOutlineBlurCombinePS, "/Engine/Private/Outline/OutlineBlur.usf", "OutlineBlurCombinePS", SF_Pixel);

void AddOutlineBlurCombinePass(FRDGBuilder& GraphBuilder, const FViewInfo& View, FRDGTextureRef BlurredOutline, FRDGTextureRef NoBlurredOutline)
{
	const FSceneTextures& SceneTextures = View.GetSceneTextures();
	FRDGTextureRef OutlineBufferA = SceneTextures.OutlineBufferA;

	if (!OutlineBufferA || !BlurredOutline || OutlineBufferA->Desc.Extent != BlurredOutline->Desc.Extent)
	{
		return;
	}

	const FScreenPassTextureViewport OutputViewport = FScreenPassTextureViewport(OutlineBufferA);
	FOutlineBlurCombinePS::FParameters* PassParameters = GraphBuilder.AllocParameters<FOutlineBlurCombinePS::FParameters>();
	PassParameters->BlurredOutlineTexture = BlurredOutline;
	PassParameters->NoBlurredOutlineTexture = NoBlurredOutline;
	PassParameters->SvPositionToTextureUV = (
		FScreenTransform::ChangeTextureBasisFromTo(FScreenPassTextureViewport(OutlineBufferA), FScreenTransform::ETextureBasis::TexelPosition, FScreenTransform::ETextureBasis::ViewportUV) *
		FScreenTransform::ChangeTextureBasisFromTo(FScreenPassTextureViewport(BlurredOutline), FScreenTransform::ETextureBasis::ViewportUV, FScreenTransform::ETextureBasis::TextureUV));

	PassParameters->RenderTargets[0] = FRenderTargetBinding(OutlineBufferA, ERenderTargetLoadAction::EClear);

	FRHIBlendState* BlendState = TStaticBlendState< CW_RGBA, BO_Add, BF_One, BF_One, BO_Add, BF_One, BF_One>::GetRHI();

	auto PixelShader = View.ShaderMap->GetShader<FOutlineBlurCombinePS>();

	FPixelShaderUtils::AddFullscreenPass(
		GraphBuilder,
		View.ShaderMap,
		RDG_EVENT_NAME("BlurOutlineCombine %dx%d (PS)", OutputViewport.Rect.Width(), OutputViewport.Rect.Height()),
		PixelShader,
		PassParameters,
		OutputViewport.Rect,
		BlendState);
}


FBlurPassesSetting GetBlurSetting(const FViewInfo& View)
{
	const FFinalPostProcessSettings& PostSettings = View.FinalPostProcessSettings;
	FBlurPassesSetting Setting;
	Setting.BlurSizeScale = PostSettings.ShadowBlurSizeScale;
	Setting.CrossBlur = PostSettings.ShadowCrossBlur;

	Setting.BlurSizes[0] = PostSettings.ShadowBlur6Size;
	Setting.BlurSizes[1] = PostSettings.ShadowBlur5Size;
	Setting.BlurSizes[2] = PostSettings.ShadowBlur4Size;
	Setting.BlurSizes[3] = PostSettings.ShadowBlur3Size;
	Setting.BlurSizes[4] = PostSettings.ShadowBlur2Size;
	Setting.BlurSizes[5] = PostSettings.ShadowBlur1Size;

	Setting.BlurTints[0] = PostSettings.ShadowBlur6Tint;
	Setting.BlurTints[1] = PostSettings.ShadowBlur5Tint;
	Setting.BlurTints[2] = PostSettings.ShadowBlur4Tint;
	Setting.BlurTints[3] = PostSettings.ShadowBlur3Tint;
	Setting.BlurTints[4] = PostSettings.ShadowBlur2Tint;
	Setting.BlurTints[5] = PostSettings.ShadowBlur1Tint;

	return Setting;
}

void AddOutlineBlurPass(FRDGBuilder& GraphBuilder, const FViewInfo& View)
{
	FBlurPassesSetting Setting = GetBlurSetting(View);
	FScreenPassRenderTarget SetupOutlineTextureA = AddOutlineBlurSetupPass(GraphBuilder, View);
	FScreenPassRenderTarget SetupOutlineTextureB = AddOutlineBlurSetupPass(GraphBuilder, View);
	FScreenPassTexture BlurredOutline = AddTextureBlurPass(GraphBuilder, View, SetupOutlineTextureA.Texture, Setting);
	AddOutlineBlurCombinePass(GraphBuilder, View, BlurredOutline.Texture, SetupOutlineTextureB.Texture);
}