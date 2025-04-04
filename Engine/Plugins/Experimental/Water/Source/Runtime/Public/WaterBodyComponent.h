// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/BlendableInterface.h"
#include "Engine/Scene.h"
#include "WaterBodyWeightmapSettings.h"
#include "WaterBodyHeightmapSettings.h"
#include "WaterCurveSettings.h"
#include "WaterBodyStaticMeshSettings.h"
#include "WaterSplineMetadata.h"
#include "BakedShallowWaterSimulationComponent.h"
#include "WaterZoneActor.h"
#include "WaterBodyTypes.h"

class AWaterBody;
class UStaticMesh;
class UWaterWavesBase;
enum ETextureRenderTargetFormat : int;
struct FPostProcessVolumeProperties;

#if UE_ENABLE_INCLUDE_ORDER_DEPRECATED_IN_5_2
#include "CoreMinimal.h"
#include "Interfaces/Interface_PostProcessVolume.h"
#include "NavAreas/NavArea.h"
#include "TerrainCarvingSettings.h"
#include "WaterBrushActorInterface.h"
#include "DynamicMeshBuilder.h"
#if WITH_EDITOR
#include "MeshDescription.h"
#endif
#endif

#include "WaterBodyComponent.generated.h"

class UWaterSplineComponent;
struct FOnWaterSplineDataChangedParams;
class UWaterBodyStaticMeshComponent;
class UWaterBodyInfoMeshComponent;
class AWaterBodyIsland;
class AWaterBodyExclusionVolume;
class AWaterZone;
class ALandscapeProxy;
class UMaterialInstanceDynamic;
class FTokenizedMessage;
class UNavAreaBase;
namespace UE::Geometry { class FDynamicMesh3; }
struct FMeshDescription;

// ----------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct FUnderwaterPostProcessSettings
{
	GENERATED_BODY()

	FUnderwaterPostProcessSettings()
		: bEnabled(true)
		, Priority(0)
		, BlendRadius(100.f)
		, BlendWeight(1.0f)
		, UnderwaterPostProcessMaterial_DEPRECATED(nullptr)
	{}
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Rendering)
	bool bEnabled;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Rendering)
	float Priority;

	/** World space radius around the volume that is used for blending (only if not unbound).			*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Rendering, meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "6000.0"))
	float BlendRadius;

	/** 0:no effect, 1:full effect */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Rendering, meta = (UIMin = "0.0", UIMax = "1.0"))
	float BlendWeight;

	/** List of all post-process settings to use when underwater : note : use UnderwaterPostProcessMaterial for setting the actual post process material. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Rendering)
	FPostProcessSettings PostProcessSettings;

	/** This is the parent post process material for the PostProcessSettings */
	UPROPERTY()
	TObjectPtr<UMaterialInterface> UnderwaterPostProcessMaterial_DEPRECATED;
};

// ----------------------------------------------------------------------------------

enum class EWaterBodyStatus : uint8
{
	Valid,
	MissingWaterZone,
	MissingLandscape,
	InvalidWaveData,
};


// ----------------------------------------------------------------------------------

struct FOnWaterBodyChangedParams
{
	FOnWaterBodyChangedParams(const FPropertyChangedEvent& InPropertyChangedEvent = FPropertyChangedEvent(/*InProperty = */nullptr))
		: PropertyChangedEvent(InPropertyChangedEvent)
	{}

	/** Provides some additional context about how the water body data has changed (property, type of change...) */
	FPropertyChangedEvent PropertyChangedEvent;

	/** Indicates that property related to the water body's visual shape has changed */
	bool bShapeOrPositionChanged = false;

	/** Indicates that a property affecting the terrain weightmaps has changed */
	bool bWeightmapSettingsChanged = false;

	/** Indicates user initiated Parameter change */
	bool bUserTriggered = false;
};


// ----------------------------------------------------------------------------------

UCLASS(Abstract, HideCategories = (Tags, Activation, Cooking, Physics, Replication, Input, AssetUserData, Mesh))
class WATER_API UWaterBodyComponent : public UPrimitiveComponent
{
	GENERATED_UCLASS_BODY()

	friend class AWaterBody;
	friend class FWaterBodySceneProxy;
	friend class FWaterBodyMeshBuilder;
public:
	virtual bool AffectsLandscape() const;
	virtual bool AffectsWaterMesh() const;
	virtual bool AffectsWaterInfo() const;
	virtual bool CanEverAffectWaterMesh() const { return true; }

	void UpdateAll(const FOnWaterBodyChangedParams& InParams);

#if WITH_EDITOR
	const FWaterCurveSettings& GetWaterCurveSettings() const { return CurveSettings; }
	const FWaterBodyHeightmapSettings& GetWaterHeightmapSettings() const { return WaterHeightmapSettings; }
	const TMap<FName, FWaterBodyWeightmapSettings>& GetLayerWeightmapSettings() const { return LayerWeightmapSettings; }
	virtual ETextureRenderTargetFormat GetBrushRenderTargetFormat() const;
	virtual void GetBrushRenderDependencies(TSet<UObject*>& OutDependencies) const;
	virtual TArray<UPrimitiveComponent*> GetBrushRenderableComponents() const { return TArray<UPrimitiveComponent*>(); }
	
	virtual void UpdateWaterSpriteComponent();

	virtual FMeshDescription GetHLODMeshDescription() const;
	virtual UMaterialInterface* GetHLODMaterial() const;
	virtual void SetHLODMaterial(UMaterialInterface* InMaterial);

	void UpdateWaterBodyRenderData();

	UFUNCTION(BlueprintCallable, Category = Water)
	void SetWaterBodyStaticMeshEnabled(bool bEnabled);
#endif //WITH_EDITOR

	/** Returns whether the body supports waves */
	virtual bool IsWaveSupported() const;

	/** Returns true if there are valid water waves */
	bool HasWaves() const;

	/** Returns whether the body is baked (false) at save-time or needs to be dynamically regenerated at runtime (true) and is therefore transient. */
	virtual bool IsBodyDynamic() const { return false; }
	
	/** Returns body's collision components */
	UFUNCTION(BlueprintCallable, Category = Collision)
	virtual TArray<UPrimitiveComponent*> GetCollisionComponents(bool bInOnlyEnabledComponents = true) const { return TArray<UPrimitiveComponent*>(); }

	/** Retrieves the list of primitive components that this water body uses when not being rendered by the water mesh (e.g. the static mesh component used when WaterMeshOverride is specified) */
	UFUNCTION(BlueprintCallable, Category = Rendering)
	virtual TArray<UPrimitiveComponent*> GetStandardRenderableComponents() const { return {}; }

	/** Returns the body's collision component bounds */
	virtual FBox GetCollisionComponentBounds() const;

	/** Returns the type of body */
	virtual EWaterBodyType GetWaterBodyType() const PURE_VIRTUAL(UWaterBodyComponent::GetWaterBodyType, return EWaterBodyType::Transition; )

	/** Returns collision half-extents */
	virtual FVector GetCollisionExtents() const { return FVector::ZeroVector; }

	/** Sets an additional water height (For internal use. Please use AWaterBodyOcean instead.) */
	virtual void SetHeightOffset(float InHeightOffset) { check(false); }

	/** Returns the additional water height added to the body (For internal use. Please use AWaterBodyOcean instead.) */
	virtual float GetHeightOffset() const { return 0.f; }

	/** Sets a static mesh to use as a replacement for the water mesh (for water bodies that are being rendered by the water mesh) */
	void SetWaterMeshOverride(UStaticMesh* InMesh);

	/** Returns River to lake transition material instance (For internal use. Please use AWaterBodyRiver instead.) */
	UFUNCTION(BlueprintCallable, Category = Rendering)
	virtual UMaterialInstanceDynamic* GetRiverToLakeTransitionMaterialInstance() { return nullptr; }

	/** Returns River to ocean transition material instance (For internal use. Please use AWaterBodyRiver instead.) */
	UFUNCTION(BlueprintCallable, Category = Rendering)
	virtual UMaterialInstanceDynamic* GetRiverToOceanTransitionMaterialInstance() { return nullptr; }

	/** Returns the WaterBodyActor who owns this component */
	UFUNCTION(BlueprintCallable, Category = Water)
	AWaterBody* GetWaterBodyActor() const;
	
	/** Returns water spline component */
	UFUNCTION(BlueprintCallable, Category = Water)
	UWaterSplineComponent* GetWaterSpline() const;

	UFUNCTION(BlueprintCallable, Category = Water)
	UWaterWavesBase* GetWaterWaves() const;

	/** Returns the unique id of this water body for accessing data in GPU buffers */
	int32 GetWaterBodyIndex() const { return WaterBodyIndex; }
	
	/** Returns water mesh override */
	UStaticMesh* GetWaterMeshOverride() const { return WaterMeshOverride; }

	/** Returns water material */
	UFUNCTION(BlueprintCallable, Category = Rendering)
	UMaterialInterface* GetWaterMaterial() const { return WaterMaterial; }

	/** Returns river to lake transition water material */
	UFUNCTION(BlueprintCallable, Category = Rendering)
	virtual UMaterialInterface* GetRiverToLakeTransitionMaterial() const { return nullptr; }

	/** Returns river to ocean transition water material */
	UFUNCTION(BlueprintCallable, Category = Rendering)
	virtual UMaterialInterface* GetRiverToOceanTransitionMaterial() const { return nullptr; }

	/** Returns material used to render the water as a static mesh */
	UMaterialInterface* GetWaterStaticMeshMaterial() const { return WaterStaticMeshMaterial; }

	/** Sets water material */
	UFUNCTION(BlueprintCallable, Category = Rendering)
	void SetWaterMaterial(UMaterialInterface* InMaterial);

	/** Sets water static mesh material */
	UFUNCTION(BlueprintCallable, Category = Rendering)
	void SetWaterStaticMeshMaterial(UMaterialInterface* InMaterial);

	/** Returns water MID */
	UFUNCTION(BlueprintCallable, Category = Rendering)
	UMaterialInstanceDynamic* GetWaterMaterialInstance();

	/** Returns water static mesh MID */
	UFUNCTION(BlueprintCallable, Category = Rendering)
	UMaterialInstanceDynamic* GetWaterStaticMeshMaterialInstance();

	/** Returns water LOD MID */
	UE_DEPRECATED(all, "GetWaterLODMaterialInstance has been renamed to GetWaterStaticMeshMaterialInstance.")
	UFUNCTION(BlueprintCallable, Category = Rendering, meta=(DeprecationMessage="GetWaterLODMaterialInstance has been renamed to GetWaterStaticMeshMaterialInstance"))
	UMaterialInstanceDynamic* GetWaterLODMaterialInstance() { return GetWaterStaticMeshMaterialInstance(); };

	/** Returns under water post process MID */
	UFUNCTION(BlueprintCallable, Category = Rendering)
	UMaterialInstanceDynamic* GetUnderwaterPostProcessMaterialInstance();
	
	/** Returns water info MID */
	UFUNCTION(BlueprintCallable, Category = Rendering)
	UMaterialInstanceDynamic* GetWaterInfoMaterialInstance();

	/** Sets under water post process material */
	UFUNCTION(BlueprintCallable, Category = Rendering)
	void SetUnderwaterPostProcessMaterial(UMaterialInterface* InMaterial);

	UFUNCTION(BlueprintCallable, Category = Rendering)
	void SetWaterAndUnderWaterPostProcessMaterial(UMaterialInterface* InWaterMaterial, UMaterialInterface* InUnderWaterPostProcessMaterial);

	/** Returns water spline metadata */
	UWaterSplineMetadata* GetWaterSplineMetadata() { return WaterSplineMetadata; }

	/** Returns water spline metadata */
	const UWaterSplineMetadata* GetWaterSplineMetadata() const { return WaterSplineMetadata; }

	/** Is this water body rendered with the WaterMeshComponent, with the quadtree-based water renderer? */
	bool ShouldGenerateWaterMeshTile() const;

	/** Returns nav collision offset */
	FVector GetWaterNavCollisionOffset() const { return FVector(0.0f, 0.0f, -GetMaxWaveHeight()); }

	/** Returns overlap material priority */
	int32 GetOverlapMaterialPriority() const { return OverlapMaterialPriority; }

	/** Returns channel depth */
	float GetChannelDepth() const { return CurveSettings.ChannelDepth; }

	void AddIsland(AWaterBodyIsland* Island);
	void RemoveIsland(AWaterBodyIsland* Island);
	void UpdateIslands();

	/** Adds WaterBody exclusion volume */
	void AddExclusionVolume(AWaterBodyExclusionVolume* InExclusionVolume);

	/** Removes WaterBody exclusion volume */
	void RemoveExclusionVolume(AWaterBodyExclusionVolume* InExclusionVolume);

	/** Returns post process properties */
	FPostProcessVolumeProperties GetPostProcessProperties() const;

	/** Returns the requested water info closest to this world location
	- InWorldLocation: world-space location closest to which the function returns the water info
	- InQueryFlags: flags to indicate which info is to be computed
	- InSplineInputKey: (optional) location on the spline, in case it has already been computed.
	*/
	virtual FWaterBodyQueryResult QueryWaterInfoClosestToWorldLocation(const FVector& InWorldLocation, EWaterBodyQueryFlags InQueryFlags, const TOptional<float>& InSplineInputKey = TOptional<float>()) const;

	UFUNCTION(BlueprintCallable, Category = WaterBody)
	void GetWaterSurfaceInfoAtLocation(const FVector& InLocation, FVector& OutWaterSurfaceLocation, FVector& OutWaterSurfaceNormal, FVector& OutWaterVelocity, float& OutWaterDepth, bool bIncludeDepth = false) const;

	/** Spline query helper. It's faster to get the spline key once then query properties using that key, rather than querying repeatedly by location etc. */
	float FindInputKeyClosestToWorldLocation(const FVector& WorldLocation) const;

	/*
	 * Spline queries specific to metadata type
	 */
	UFUNCTION(BlueprintCallable, Category = WaterBody)
	virtual float GetWaterVelocityAtSplineInputKey(float InKey) const;
	virtual FVector GetWaterVelocityVectorAtSplineInputKey(float InKey) const;
	virtual float GetAudioIntensityAtSplineInputKey(float InKey) const;

	/**
	 * Gets the islands that influence this water body
	 */
	UFUNCTION(BlueprintCallable, Category = Water)
	TArray<AWaterBodyIsland*> GetIslands() const;

	bool ContainsIsland(TSoftObjectPtr<AWaterBodyIsland> Island) const { return WaterBodyIslands.Contains(Island); }

	/**
	 * Gets the exclusion volume that influence this water body
	 */
	UFUNCTION(BlueprintCallable, Category = Water)
	TArray<AWaterBodyExclusionVolume*> GetExclusionVolumes() const;

	bool ContainsExclusionVolume(TSoftObjectPtr<AWaterBodyExclusionVolume> InExclusionVolume) const { return WaterBodyExclusionVolumes.Contains(InExclusionVolume); }

	/** Component interface */
	virtual void OnRegister() override;
	virtual void OnUnregister() override;
	virtual void PostDuplicate(bool bDuplicateForPie) override;

	void OnWaterBodyChanged(const FOnWaterBodyChangedParams& InParams);

	/** Fills wave-related information at the given world position and for this water depth.
	 - InPosition : water surface position at which to query the wave information
	 - InWaterDepth : water depth at this location
	 - bSimpleWaves : true for the simple version (faster computation, lesser accuracy, doesn't perturb the normal)
	 - FWaveInfo : input/output : the structure's field must be initialized prior to the call (e.g. InOutWaveInfo.Normal is the unperturbed normal)
	 Returns true if waves are supported, false otherwise. */
	bool GetWaveInfoAtPosition(const FVector& InPosition, float InWaterDepth, bool bInSimpleWaves, FWaveInfo& InOutWaveInfo) const;

	/** Returns the max height that this water body's waves can hit. Can be called regardless of whether the water body supports waves or not */
	UFUNCTION(BlueprintCallable, Category = Wave)
	float GetMaxWaveHeight() const;

	/** Sets the dynamic parameters needed by the material instance for rendering. Returns true if the operation was successfull */
	virtual bool SetDynamicParametersOnMID(UMaterialInstanceDynamic* InMID);

	/** Sets the dynamic parameters needed by the underwater post process material instance for rendering. Returns true if the operation was successfull */
	virtual bool SetDynamicParametersOnUnderwaterPostProcessMID(UMaterialInstanceDynamic* InMID);

	/** Sets the dynamic parameters needed by the material instance for rendering the water info texture. Returns true if the operation was successfull */
	virtual bool SetDynamicParametersOnWaterInfoMID(UMaterialInstanceDynamic* InMID);

	/** Returns true if the location is within one of this water body's exclusion volumes */
	bool IsWorldLocationInExclusionVolume(const FVector& InWorldLocation) const;

	UE_DEPRECATED(5.5, "Use UpdateComponentVisibility")
	virtual void UpdateComponentVisibility(bool bAllowWaterZoneRebuild);

	/** Updates the bVisible/bHiddenInGame flags on the component and eventually the child renderable components (e.g. custom water body) */
	void UpdateVisibility();

	/** Creates/Destroys/Updates necessary MIDS */
	virtual void UpdateMaterialInstances();

	/** Returns the time basis to use in waves computation (must be unique for all water bodies currently, to ensure proper transitions between water tiles) */
	virtual float GetWaveReferenceTime() const;

	/** Returns the minimum and maximum Z of the water surface, including waves */
	virtual void GetSurfaceMinMaxZ(float& OutMinZ, float& OutMaxZ) const;

	virtual ALandscapeProxy* FindLandscape() const;

	/** Returns what can be considered the single water depth of the water surface.
	Only really make sense for EWaterBodyType::Transition water bodies for which we don't really have a way to evaluate depth. */
	virtual float GetConstantDepth() const;

	/** Returns what can be considered the single water velocity of the water surface.
	Only really make sense for EWaterBodyType::Transition water bodies for which we don't really have a way to evaluate velocity. */
	virtual FVector GetConstantVelocity() const;

	/** Returns what can be considered the single base Z of the water surface.
	Doesn't really make sense for non-flat water bodies like EWaterBodyType::Transition or EWaterBodyType::River but can still be useful when using FixedZ for post-process, for example. */
	virtual float GetConstantSurfaceZ() const;

	virtual void Reset() {}

	/** Gets the water zone to which this component belongs */
	AWaterZone* GetWaterZone() const;

	/** Override the default behavior of water bodies finding their water zone based on bounds and set a specific water zone to which this water body should register. */
	UFUNCTION(BlueprintCallable, Category=Water)
	void SetWaterZoneOverride(const TSoftObjectPtr<AWaterZone>& InWaterZoneOverride);

	/** 
	 * Registers or this water body with corresponding overlapping water zones and unregisters it from any old zones if they are no longer overlapping.
	 *
	 * @param bAllowChangesDuringCook When disabled, this function will not make any changes during cook and just trust that the serialized pointer is correct. 
	 */
	void UpdateWaterZones(bool bAllowChangesDuringCook = false);

	/** Set the navigation area class */
	void SetNavAreaClass(TSubclassOf<UNavAreaBase> NewWaterNavAreaClass) { WaterNavAreaClass = NewWaterNavAreaClass; }

	/* Generates the mesh representation of the water body */
	virtual bool GenerateWaterBodyMesh(UE::Geometry::FDynamicMesh3& OutMesh, UE::Geometry::FDynamicMesh3* OutDilatedMesh = nullptr) const;

	UWaterBodyInfoMeshComponent* GetWaterInfoMeshComponent() const;
	UWaterBodyInfoMeshComponent* GetDilatedWaterInfoMeshComponent() const;

	const FWaterBodyStaticMeshSettings& GetWaterBodyStaticMeshSettings() const { return StaticMeshSettings; }
	
	UE_DEPRECATED(all, "Use version which takes FOnWaterBodyChangedParams")
	UFUNCTION(BlueprintCallable, Category=Water, meta=(Deprecated = "5.2"))
	void OnWaterBodyChanged(bool bShapeOrPositionChanged, bool bWeightmapSettingsChanged = false, bool bUserTriggeredChanged = false) {}

	/** Get the baked shallow water simulation for this water body */
	UBakedShallowWaterSimulationComponent* GetBakedShallowWaterSimulation() const { return BakedShallowWaterSim.Get(); }
	
	/** Set the baked shallow water simulation for this water body */
	void SetBakedShallowWaterSimulation(TObjectPtr<UBakedShallowWaterSimulationComponent> BakedSim) { BakedShallowWaterSim = BakedSim; }

	/** Set toggle to use baked simulations if they are valid */
	void SetUseBakedSimulationForQueriesAndPhysics(bool bUseBakedSimulation) { bUseBakedSimForQueriesAndPhysics = bUseBakedSimulation;  }
	
	/** Query for if the baked simulations is valid for use */
	bool UseBakedSimulationForQueriesAndPhysics() const { return bUseBakedSimForQueriesAndPhysics && BakedShallowWaterSim.IsValid() && BakedShallowWaterSim->SimulationData.IsValid(); }

	/** 
	 * Marks the owning water zone for rebuild. 
	 * If bOnlyWithinWaterBodyBounds is set, updates to the water zone that aren't relevant within the bounds of the water body are suppressed.
	 */
	void MarkOwningWaterZoneForRebuild(EWaterZoneRebuildFlags InRebuildFlags, bool bInOnlyWithinWaterBodyBounds = true) const;

protected:
	//~ Begin UActorComponent interface.
	virtual bool IsHLODRelevant() const override;
	//~ End UActorComponent interface.

	//~ Begin USceneComponent Interface.
	virtual void OnVisibilityChanged();
	virtual void OnHiddenInGameChanged();
	//~ End USceneComponent Interface.

	/** Returns whether the body has a flat surface or not */
	virtual bool IsFlatSurface() const;

	/** Returns whether the body's spline is closed */
	virtual bool IsWaterSplineClosedLoop() const;

	/** Returns whether the body support a height offset */
	virtual bool IsHeightOffsetSupported() const;

	/** Called every time UpdateAll is called on WaterBody (prior to UpdateWaterBody) */
	virtual void BeginUpdateWaterBody();

	/** Updates WaterBody (called 1st with bWithExclusionVolumes = false, then with true */
	virtual void UpdateWaterBody(bool bWithExclusionVolumes);

	virtual void OnUpdateBody(bool bWithExclusionVolumes) {}

	/** Called when the WaterBodyActor has had all its components registered. */
	virtual void OnPostRegisterAllComponents();

	/** Returns navigation area class */
	TSubclassOf<UNavAreaBase> GetNavAreaClass() const { return WaterNavAreaClass; }

	/** Copies the relevant collision settings from the water body component to the component passed in parameter (useful when using external components for collision) */
	void CopySharedCollisionSettingsToComponent(UPrimitiveComponent* InComponent);

	/** Copies the relevant navigation settings from the water body component to the component passed in parameter (useful when using external components for navigation) */
	void CopySharedNavigationSettingsToComponent(UPrimitiveComponent* InComponent);

	/** Computes the raw wave perturbation of the water height/normal */
	virtual float GetWaveHeightAtPosition(const FVector& InPosition, float InWaterDepth, float InTime, FVector& OutNormal) const;

	/** Computes the raw wave perturbation of the water height only (simple version : faster computation) */
	virtual float GetSimpleWaveHeightAtPosition(const FVector& InPosition, float InWaterDepth, float InTime) const;

	/** Computes the attenuation factor to apply to the raw wave perturbation. Attenuates : normal/wave height/max wave height. */
	virtual float GetWaveAttenuationFactor(const FVector& InPosition, float InWaterDepth) const;

	/** Called by the owning actor when it receives a PostActorCreated callback */
	virtual void OnPostActorCreated() {}

#if WITH_EDITOR
	/** Called by UWaterBodyComponent::PostEditChangeProperty. */
	virtual void OnPostEditChangeProperty(FOnWaterBodyChangedParams& InOutOnWaterBodyChangedParams);

	/** Validates this component's data */
	virtual void CheckForErrors() override;

	virtual TArray<TSharedRef<FTokenizedMessage>> CheckWaterBodyStatus();

	virtual const TCHAR* GetWaterSpriteTextureName() const { return TEXT("/Water/Icons/WaterSprite"); }

	virtual bool IsIconVisible() const { return true; }

	virtual FVector GetWaterSpriteLocation() const { return GetComponentLocation(); }

	virtual void OnWaterBodyRenderDataUpdated();

	/** Fixup any invalid transformations made to the water body in the editor that may have been made through the transform gizmo or property changes. */
	void FixupEditorTransform();
#endif // WITH_EDITOR

	EWaterBodyQueryFlags CheckAndAjustQueryFlags(EWaterBodyQueryFlags InQueryFlags) const;
	void UpdateSplineComponent();
	void UpdateExclusionVolumes();
	bool UpdateWaterHeight();
	virtual void CreateOrUpdateWaterMID();
	void CreateOrUpdateWaterStaticMeshMID();
	void CreateOrUpdateUnderwaterPostProcessMID();
	void CreateOrUpdateWaterInfoMID();
	void PrepareCurrentPostProcessSettings();
	void ApplyNavigationSettings();
	void ApplyCollisionSettings();
	void RequestGPUWaveDataUpdate();
	EObjectFlags GetTransientMIDFlags() const; 
	void DeprecateData();

	virtual void Serialize(FArchive& Ar) override;
	virtual void PostLoad() override;
	virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;
	virtual void OnComponentCollisionSettingsChanged(bool bUpdateOverlaps) override;
	virtual void OnGenerateOverlapEventsChanged() override;
	virtual void GetResourceSizeEx(FResourceSizeEx& CumulativeResourceSize) override;

#if WITH_EDITOR
	virtual void PreEditUndo() override;
	virtual void PostEditUndo() override;
	virtual void PostEditImport() override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	void OnWaterSplineDataChanged(const FOnWaterSplineDataChangedParams& InParams);
	void RegisterOnUpdateWavesData(UWaterWavesBase* InWaterWaves, bool bRegister);
	void OnWavesDataUpdated(UWaterWavesBase* InWaterWaves, EPropertyChangeType::Type InChangeType);
	void OnWaterSplineMetadataChanged(const FOnWaterSplineMetadataChangedParams& InParams);
	void RegisterOnChangeWaterSplineData(bool bRegister);

	void CreateWaterSpriteComponent();

	void UpdateWaterInfoMeshComponents();
	void UpdateWaterBodyStaticMeshComponents();

	virtual TSubclassOf<class UHLODBuilder> GetCustomHLODBuilderClass() const override;
#endif // WITH_EDITOR

public:
	// INavRelevantInterface start
	virtual void GetNavigationData(struct FNavigationRelevantData& Data) const override;
	virtual FBox GetNavigationBounds() const override;
	virtual bool IsNavigationRelevant() const override;
	// INavRelevantInterface end

	/** Public static constants : */
	static const FName WaterBodyIndexParamName;
	static const FName WaterZoneIndexParamName;
	static const FName WaterBodyZOffsetParamName;
	static const FName WaterVelocityAndHeightName;
	static const FName GlobalOceanHeightName;
	static const FName MaxFlowVelocityParamName;

	UPROPERTY(EditDefaultsOnly, Category = Collision, meta = (EditCondition = "bGenerateCollisions"))
	TObjectPtr<UPhysicalMaterial> PhysicalMaterial;

	/** Water depth at which waves start being attenuated. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Wave, DisplayName = "Wave Attenuation Water Depth", meta = (UIMin = 0, ClampMin = 0, UIMax = 10000.0))
	float TargetWaveMaskDepth;

	/** Offset added to the automatically calculated max wave height bounds. Use this in case the automatically calculated max height bounds don't match your waves. This can happen if the water surface is manually altered through World Position Offset or other means.*/
	UPROPERTY(EditAnywhere, Category = Wave)
	float MaxWaveHeightOffset = 0.0f;

	/** Post process settings to apply when the camera goes underwater (only available when bGenerateCollisions is true because collisions are needed to detect if it's under water).
	Note: Underwater post process material is setup using UnderwaterPostProcessMaterial. */
	UPROPERTY(Category = Rendering, EditAnywhere, BlueprintReadWrite, AdvancedDisplay, meta = (EditCondition = "bGenerateCollisions && UnderwaterPostProcessMaterial != nullptr", DisplayAfter = "UnderwaterPostProcessMaterial"))
	FUnderwaterPostProcessSettings UnderwaterPostProcessSettings;

	UPROPERTY(Category = Terrain, EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "bAffectsLandscape"))
	FWaterCurveSettings CurveSettings;

	UPROPERTY(Category = Rendering, EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UMaterialInterface> WaterMaterial;

	UPROPERTY(Category = HLOD, EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "Water HLOD Material"))
	TObjectPtr<UMaterialInterface> WaterHLODMaterial;

	UPROPERTY(Category = Rendering, EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UMaterialInterface> WaterStaticMeshMaterial;

	/** Post process material to apply when the camera goes underwater (only available when bGenerateCollisions is true because collisions are needed to detect if it's under water). */
	UPROPERTY(Category = Rendering, EditAnywhere, BlueprintReadOnly, meta = (EditCondition = "bGenerateCollisions", DisplayAfter = "WaterMaterial"))
	TObjectPtr<UMaterialInterface> UnderwaterPostProcessMaterial;

	UPROPERTY(Category = Rendering, EditAnywhere, BlueprintReadOnly, meta = (DisplayAfter = "WaterMaterial"))
	TObjectPtr<UMaterialInterface> WaterInfoMaterial;
	
	UPROPERTY(Category = Terrain, EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "bAffectsLandscape"))
	FWaterBodyHeightmapSettings WaterHeightmapSettings;

	UPROPERTY(Category = Terrain, EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "bAffectsLandscape"))
	TMap<FName, FWaterBodyWeightmapSettings> LayerWeightmapSettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = Rendering)
	float ShapeDilation = 4096.0f;

	/** The distance above the surface of the water where collision checks should still occur. Useful if the post process effect is not activating under really high waves. */ 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = Collision)
	float CollisionHeightOffset = 0.f;

	/** If enabled, landscape will be deformed based on this water body placed on top of it and landscape height will be considered when determining water depth at runtime */
	UPROPERTY(Category = Terrain, EditAnywhere, BlueprintReadWrite)
	bool bAffectsLandscape;

	UPROPERTY(Category = Water, EditAnywhere)
	FWaterBodyStaticMeshSettings StaticMeshSettings;

protected:

	/** Unique Id for accessing (wave, ... ) data in GPU buffers */
	UPROPERTY(Transient, DuplicateTransient, NonTransactional, VisibleAnywhere, BlueprintReadOnly, Category = Water)
	int32 WaterBodyIndex = INDEX_NONE;

	UPROPERTY(Category = Rendering, EditAnywhere, BlueprintReadWrite, Getter, Setter)
	TObjectPtr<UStaticMesh> WaterMeshOverride;

	/** 
	 * When this is set to true, the water mesh will always generate tiles for this water body. 
	 * For example, this can be useful to generate water tiles even when the water material is invalid, for the case where "empty" water tiles are actually desirable.
	 */
	UPROPERTY(Category = Rendering, EditAnywhere, BlueprintReadOnly, AdvancedDisplay)
	bool bAlwaysGenerateWaterMeshTiles = false;

	/** Higher number is higher priority. If two water bodies overlap and they don't have a transition material specified, this will be used to determine which water body to use the material from. Valid range is -8192 to 8191 */
	UPROPERTY(Category = Rendering, EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "-8192", ClampMax = "8191"))
	int32 OverlapMaterialPriority = 0;

	UPROPERTY(Transient)
	TObjectPtr<UWaterSplineMetadata> WaterSplineMetadata;

	UPROPERTY(Category = Debug, VisibleInstanceOnly, Transient, NonPIEDuplicateTransient, TextExportTransient, meta = (DisplayAfter = "WaterMaterial"))
	TObjectPtr<UMaterialInstanceDynamic> WaterMID;

	UPROPERTY(Category = Debug, VisibleInstanceOnly, Transient, NonPIEDuplicateTransient, TextExportTransient, meta = (DisplayAfter = "WaterStaticMeshMaterial"))
	TObjectPtr<UMaterialInstanceDynamic> WaterStaticMeshMID;

	UPROPERTY(Category = Debug, VisibleInstanceOnly, Transient, NonPIEDuplicateTransient, TextExportTransient, meta = (DisplayAfter = "UnderwaterPostProcessMaterial"))
	TObjectPtr<UMaterialInstanceDynamic> UnderwaterPostProcessMID;
	
	UPROPERTY(Category = Debug, VisibleInstanceOnly, Transient, NonPIEDuplicateTransient, TextExportTransient, meta = (DisplayAfter = "WaterInfoMaterial"))
	TObjectPtr<UMaterialInstanceDynamic> WaterInfoMID;

	/** Islands in this water body*/
	UPROPERTY(Category = Water, EditAnywhere, AdvancedDisplay)
	TArray<TSoftObjectPtr<AWaterBodyIsland>> WaterBodyIslands;

	UPROPERTY(Category = Water, EditAnywhere, AdvancedDisplay)
	TArray<TSoftObjectPtr<AWaterBodyExclusionVolume>> WaterBodyExclusionVolumes;

	UPROPERTY(Transient)
	mutable TWeakObjectPtr<ALandscapeProxy> Landscape;

	UPROPERTY(Category = Water, VisibleInstanceOnly, AdvancedDisplay, TextExportTransient)
	TSoftObjectPtr<AWaterZone> OwningWaterZone;

	UPROPERTY(Category = Water, EditAnywhere, BlueprintReadOnly, AdvancedDisplay)
	TSoftObjectPtr<AWaterZone> WaterZoneOverride;

	UPROPERTY(Transient)
	FPostProcessSettings CurrentPostProcessSettings;

	// The navigation area class that will be generated on nav mesh
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Navigation, meta = (EditCondition = "bCanAffectNavigation && bGenerateCollisions"))
	TSubclassOf<UNavAreaBase> WaterNavAreaClass;

	/** If the Water Material assigned to this component has Fixed Depth enabled, this is the depth that is passed. */
	UPROPERTY(Category = Water, EditAnywhere, AdvancedDisplay)
	double FixedWaterDepth = 512.0;

	/**  Baked simulation data for this water body, owned by a UShallowWaterRiverComponent */
	UPROPERTY(Category = BakedSimulation, AdvancedDisplay, VisibleAnywhere)
	TWeakObjectPtr<UBakedShallowWaterSimulationComponent> BakedShallowWaterSim;

	/**  Override to disable use of the baked shallow water simulation for collisons and other uses */
	UPROPERTY(Category = BakedSimulation, AdvancedDisplay, EditAnywhere)
	bool bUseBakedSimForQueriesAndPhysics = true;

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	TArray<TLazyObjectPtr<AWaterBodyIsland>> Islands_DEPRECATED;
	UPROPERTY()
	TArray<TLazyObjectPtr<AWaterBodyExclusionVolume>> ExclusionVolumes_DEPRECATED;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> WaterLODMaterial_DEPRECATED;

	UPROPERTY()
	bool bFillCollisionUnderWaterBodiesForNavmesh_DEPRECATED;

	UPROPERTY()
	FName CollisionProfileName_DEPRECATED;

	UPROPERTY()
	bool bGenerateCollisions_DEPRECATED = true;

	UPROPERTY()
	bool bCanAffectNavigation_DEPRECATED;

	UPROPERTY()
	bool bOverrideWaterMesh_DEPRECATED;
#endif // WITH_EDITORONLY_DATA

private:
	/** Boolean to keep track of whether the water body is visible in the water mesh. This avoids unnecessary calls to rebuild the water mesh whenever UpdateComponentVisibility is called */
	bool bIsRenderedByWaterMeshAndVisible = false;

};
