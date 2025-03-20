// Copyright Epic Games, Inc. All Rights Reserved.
#include "OutlinePassRendering.h"
#include "MeshEdgesRendering.h"
#include "ScenePrivate.h"
#include "MeshPassProcessor.inl"
#include "SimpleMeshDrawCommandPass.h"
#include "StaticMeshBatch.h"
#include "DeferredShadingRenderer.h"

// #pragma optimize("", off)
//--------------------------------OutlineBufferTexture------------------------
FRDGTextureDesc GetOutlineBufferTextureDesc(FIntPoint Extent, ETextureCreateFlags CreateFlags)
{
	//输入的参数：
	//Extent：贴图尺寸；PF_B8G8R8A8：贴图格式，表示RGBA各个通道均为8bit
	//FClearValueBinding::Black:清除值，表示清除贴图时将其清除为黑色
	//TexCreate_UAV：Unordered Access View，允许在着色器中进行随机读写操作
	//TexCreate_RenderTargetable：表示纹理可作为渲染目标使用
	//TexCreate_ShaderResource：表示纹理可作为着色器资源，可以在着色器中进行采样等操作
	return FRDGTextureDesc(FRDGTextureDesc::Create2D(Extent, PF_B8G8R8A8, FClearValueBinding::Black, TexCreate_UAV | TexCreate_RenderTargetable | TexCreate_ShaderResource | CreateFlags));
}

FRDGTextureRef CreateOutlineBufferTexture(FRDGBuilder& GraphBuilder, FIntPoint Extent, ETextureCreateFlags CreateFlags)
{
	return GraphBuilder.CreateTexture(GetOutlineBufferTextureDesc(Extent, CreateFlags), TEXT("OutlineBufferA"));
}
//--------------------------------OutlineBufferTexture------------------------

// IMPLEMENT_MATERIAL_SHADER_TYPE接受的参数：
// FOutlinePassPS:我们在OutlinePassRendering.h中定义的shader类
// TEXT("/Engine/Private/Outline/OutlinePassShader.usf"):我们使用的shader路径
// TEXT("MainPS"):shader的入口函数名
// SF_Pixel:shader的类型，Vertex shader、Pixel shader或者compute shader
IMPLEMENT_MATERIAL_SHADER_TYPE(, FOutlinePassVS, TEXT("/Engine/Private/Outline/OutlinePassShader.usf"), TEXT("MainVS"), SF_Vertex);
IMPLEMENT_MATERIAL_SHADER_TYPE(, FOutlinePassPS, TEXT("/Engine/Private/Outline/OutlinePassShader.usf"), TEXT("MainPS"), SF_Pixel);

FOutlinePassMeshProcessor::FOutlinePassMeshProcessor(
	EMeshPass::Type InMeshPassType,
	const FScene* Scene,
	ERHIFeatureLevel::Type InFeatureLevel,
	const FSceneView* InViewIfDynamicMeshCommand,
	const FMeshPassProcessorRenderState& InPassDrawRenderState,
	FMeshPassDrawListContext* InDrawListContext)
	:FMeshPassProcessor(InMeshPassType, Scene, Scene->GetFeatureLevel(), InViewIfDynamicMeshCommand, InDrawListContext),
	PassDrawRenderState(InPassDrawRenderState)
{
	// 设置默认的BlendState和DepthStencilState
	// BlendState控制颜色混合方式
	// DepthStencilState控制深度写入，深度测试等行为
	if (PassDrawRenderState.GetDepthStencilState() == nullptr)
	{
		PassDrawRenderState.SetDepthStencilState(TStaticDepthStencilState<false, CF_DepthNearOrEqual>().GetRHI());
	}
	if (PassDrawRenderState.GetBlendState() == nullptr)
	{
		PassDrawRenderState.SetBlendState(TStaticBlendState<>().GetRHI());
	}
}

void FOutlinePassMeshProcessor::AddMeshBatch(
	const FMeshBatch& MeshBatch,
	uint64 BatchElementMask,
	const FPrimitiveSceneProxy* PrimitiveSceneProxy,
	int32 StaticMeshId)
{
	if (PrimitiveSceneProxy && PrimitiveSceneProxy->IsEnabledOutline_RenderThread())
	{
		const FMaterialRenderProxy* MaterialRenderProxy = MeshBatch.MaterialRenderProxy;

		const FMaterial* Material = MaterialRenderProxy->GetMaterialNoFallback(FeatureLevel);

		if (Material != nullptr && Material->GetRenderingThreadShaderMap())
		{
			Process(
				MeshBatch,
				BatchElementMask,
				StaticMeshId,
				PrimitiveSceneProxy,
				*MaterialRenderProxy,
				*Material,
				FM_Solid,
				CM_CW); //背面剔除
		}
	}
}

bool FOutlinePassMeshProcessor::Process(
	const FMeshBatch& MeshBatch,
	uint64 BatchElementMask,
	int32 StaticMeshId,
	const FPrimitiveSceneProxy* PrimitiveSceneProxy,
	const FMaterialRenderProxy& MaterialRenderProxy,
	const FMaterial& RESTRICT MaterialResource,
	ERasterizerFillMode MeshFillMode,
	ERasterizerCullMode MeshCullMode)
{
	const FVertexFactory* VertexFactory = MeshBatch.VertexFactory;

	TMeshProcessorShaders<FOutlinePassVS, FOutlinePassPS> OutlinePassShader;
	{
		FMaterialShaderTypes ShaderTypes;
		// 指定使用的shader
		ShaderTypes.AddShaderType<FOutlinePassVS>();
		ShaderTypes.AddShaderType<FOutlinePassPS>();

		const FVertexFactoryType* VertexFactoryType = VertexFactory->GetType();

		FMaterialShaders Shaders;
		if (!MaterialResource.TryGetShaders(ShaderTypes, VertexFactoryType, Shaders))
		{
			//UE_LOG(LogShaders, Warning, TEXT("Shader Not Found!"));
			return false;
		}

		Shaders.TryGetVertexShader(OutlinePassShader.VertexShader);
		Shaders.TryGetPixelShader(OutlinePassShader.PixelShader);
	}


	FMeshMaterialShaderElementData ShaderElementData;
	ShaderElementData.InitializeMeshMaterialData(ViewIfDynamicMeshCommand, PrimitiveSceneProxy, MeshBatch, StaticMeshId, false);

	const FMeshDrawCommandSortKey SortKey = CalculateMeshStaticSortKey(OutlinePassShader.VertexShader, OutlinePassShader.PixelShader);
	PassDrawRenderState.SetDepthStencilState(TStaticDepthStencilState<false, CF_DepthNearOrEqual>().GetRHI());

	FMeshPassProcessorRenderState DrawRenderState(PassDrawRenderState);

	BuildMeshDrawCommands(
		MeshBatch,
		BatchElementMask,
		PrimitiveSceneProxy,
		MaterialRenderProxy,
		MaterialResource,
		PassDrawRenderState,
		OutlinePassShader,
		MeshFillMode,
		MeshCullMode,
		SortKey,
		EMeshPassFeatures::Default,
		ShaderElementData
	);

	return true;
}

void SetupOutlinePassState(FMeshPassProcessorRenderState& DrawRenderState)
{
	DrawRenderState.SetDepthStencilState(TStaticDepthStencilState<false, CF_DepthNearOrEqual>::GetRHI());
}

FMeshPassProcessor* CreateOutlinePassProcessor(ERHIFeatureLevel::Type FeatureLevel, const FScene* Scene, const FSceneView* InViewIfDynamicMeshCommand, FMeshPassDrawListContext* InDrawListContext)
{
	FMeshPassProcessorRenderState OutlinePassState;
	SetupOutlinePassState(OutlinePassState);
	return new FOutlinePassMeshProcessor(EMeshPass::OutlinePass, Scene, FeatureLevel, InViewIfDynamicMeshCommand, OutlinePassState, InDrawListContext);
}

// RegisterOutlinePass会将CreateOutlinePassProcessor函数的地址写入FPassProcessorManager的一个Table里，Table的下标是EShadingPath和EMeshPass
// 这个Table包括了所以Pass的CreatePassProcessor函数，之后引擎就可以根据EShadingPath和EMeshPass找到对应pass的CreatePassProcessor函数
FRegisterPassProcessorCreateFunction RegisterOutlinePass(&CreateOutlinePassProcessor, EShadingPath::Deferred, EMeshPass::OutlinePass, EMeshPassFlags::CachedMeshCommands | EMeshPassFlags::MainView);

DECLARE_CYCLE_STAT(TEXT("OutlinePass"), STAT_OutlinePass, STATGROUP_SceneRendering);

BEGIN_SHADER_PARAMETER_STRUCT(FOutlineMeshPassParameters, )
SHADER_PARAMETER_STRUCT_REF(FViewUniformShaderParameters, View)
SHADER_PARAMETER_STRUCT_INCLUDE(FInstanceCullingDrawParams, InstanceCullingDrawParams)
RENDER_TARGET_BINDING_SLOTS()
END_SHADER_PARAMETER_STRUCT()

FOutlineMeshPassParameters* GetOutlinePassParameters(FRDGBuilder& GraphBuilder, const FViewInfo& View, FSceneTextures& SceneTextures)
{
	FOutlineMeshPassParameters* PassParameters = GraphBuilder.AllocParameters<FOutlineMeshPassParameters>();
	PassParameters->View = View.ViewUniformBuffer;

	if (!HasBeenProduced(SceneTextures.OutlineBufferA))
	{
		const FSceneTexturesConfig& Config = View.GetSceneTexturesConfig();
		SceneTextures.OutlineBufferA = CreateOutlineBufferTexture(GraphBuilder, Config.Extent, GFastVRamConfig.OutlineBufferA);
	}

	// 设置RenderTarget
	PassParameters->RenderTargets[0] = FRenderTargetBinding(SceneTextures.OutlineBufferA, ERenderTargetLoadAction::EClear);

	return PassParameters;
}

// 在DeferredShadingSceneRenderer调用这个函数来渲染OutlinePass
void FDeferredShadingSceneRenderer::RenderOutlinePass(FRDGBuilder& GraphBuilder, FSceneTextures& SceneTextures)
{
	RDG_EVENT_SCOPE(GraphBuilder, "OutlinePass");
	RDG_CSV_STAT_EXCLUSIVE_SCOPE(GraphBuilder, RenderOutlinePass);

	SCOPED_NAMED_EVENT(FDeferredShadingSceneRenderer_RenderOutlinePass, FColor::Emerald);

	for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ++ViewIndex)
	{
		FViewInfo& View = Views[ViewIndex];
		RDG_GPU_MASK_SCOPE(GraphBuilder, View.GPUMask);
		RDG_EVENT_SCOPE_CONDITIONAL(GraphBuilder, Views.Num() > 1, "View%d", ViewIndex);

		const bool bShouldRenderView = View.ShouldRenderView();
		if (bShouldRenderView)
		{
			FOutlineMeshPassParameters* PassParameters = GetOutlinePassParameters(GraphBuilder, View, SceneTextures);

			View.ParallelMeshDrawCommandPasses[EMeshPass::OutlinePass].BuildRenderingCommands(GraphBuilder, Scene->GPUScene, PassParameters->InstanceCullingDrawParams);

			GraphBuilder.AddDispatchPass(
				RDG_EVENT_NAME("OutlinePass"),
				PassParameters,
				ERDGPassFlags::Raster | ERDGPassFlags::SkipRenderPass,
				[this, &View, PassParameters](FRDGDispatchPassBuilder& DispatchPassBuilder)
				{
					View.ParallelMeshDrawCommandPasses[EMeshPass::OutlinePass].Dispatch(DispatchPassBuilder, &PassParameters->InstanceCullingDrawParams);
				});
		}
	}
}
// #pragma optimize("", on)