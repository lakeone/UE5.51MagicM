// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameFramework/Actor.h"
#include "MassCommonTypes.h"
#include "Misc/MTAccessDetector.h"
#include "MassRepresentationTypes.h"
#include "MassActorSpawnerSubsystem.h"
#include "MassSubsystemBase.h"
#include "MassRepresentationSubsystem.generated.h"

class UMassVisualizationComponent;
class AMassVisualizer;
struct FStaticMeshInstanceVisualizationDesc;
struct FMassInstancedStaticMeshInfo;
struct FMassActorSpawnRequestHandle;
class UMassActorSpawnerSubsystem;
class UMassAgentComponent;
struct FMassEntityManager;
enum class EMassProcessingPhase : uint8;
class UWorldPartitionSubsystem;

/**
 * Subsystem responsible for all visual of mass agents, will handle actors spawning and static mesh instances
 */
UCLASS()
class MASSREPRESENTATION_API UMassRepresentationSubsystem : public UMassSubsystemBase
{
	GENERATED_BODY()

public:
	/** 
	 * Get the index of the static mesh visual type, will add a new one if does not exist  
	 * @param Desc is the information for the static mesh that will be instantiated later via AddStaticMeshInstance()
	 * @return The index of the static mesh type 
	 */
	FStaticMeshInstanceVisualizationDescHandle FindOrAddStaticMeshDesc(const FStaticMeshInstanceVisualizationDesc& Desc);

	/**
	 * Creates a dedicated visual type described by host Desc and ties ISMComponent to it.
	 * @note this is a helper function for a common "single ISMComponent" case. Calls AddVisualDescWithISMComponents under the hood.
	 * @return The index of the visual type
	 */
	FStaticMeshInstanceVisualizationDescHandle AddVisualDescWithISMComponent(const FStaticMeshInstanceVisualizationDesc& Desc, UInstancedStaticMeshComponent& ISMComponent);

	/**
	 * Creates a dedicated visual type described by host Desc and ties given ISMComponents to it.
	 * @return The index of the visual type
	 */
	FStaticMeshInstanceVisualizationDescHandle AddVisualDescWithISMComponents(const FStaticMeshInstanceVisualizationDesc& Desc, TArrayView<TObjectPtr<UInstancedStaticMeshComponent>> ISMComponents);

	/**
	 * Fetches FMassISMCSharedData indicated by DescriptionIndex, or nullptr if it's not a valid index
	 */
	const FMassISMCSharedData* GetISMCSharedDataForDescriptionIndex(const int32 DescriptionIndex) const;

	/**
	 * Fetches FMassISMCSharedData indicated by an ISMC, or nullptr if the ISMC is not represented by any shared data.
	 */
	const FMassISMCSharedData* GetISMCSharedDataForInstancedStaticMesh(const UInstancedStaticMeshComponent* ISMC) const;

	/**
	 * Removes the visualization data associated with the given ISM component. Note that this is safe to do only when
	 * there are no entities relying on this data. No entity data patching will take place.
	 * Note that the function will assert if there's more ISM components associated with given visualization. Also, in 
	 * that case RemoveVisualDescByIndex will be called under the hood. 
	 */
	UE_DEPRECATED(5.4, "RemoveISMComponent has been deprecated in favor of RemoveVisualDescByIndex. Please use that instead.")
	void RemoveISMComponent(UInstancedStaticMeshComponent& ISMComponent);

	/** 
	 * Removes all data associated with a given VisualizationIndex. Note that this is safe to do only if there are no
	 * entities relying on this index. No entity data patching will take place.
	 */
	void RemoveVisualDesc(const FStaticMeshInstanceVisualizationDescHandle VisualizationHandle);

	/** 
	 * @return the array of all the static mesh instance component information
	 */
	FMassInstancedStaticMeshInfoArrayView GetMutableInstancedStaticMeshInfos();

	/** Mark render state of the static mesh instances dirty */
	void DirtyStaticMeshInstances();

	/** 
	 * Store the template actor uniquely and return an index to it 
	 * @param ActorClass is a template actor class we will need to spawn for an agent 
	 * @return The index of the template actor type
	 */
	int16 FindOrAddTemplateActor(const TSubclassOf<AActor>& ActorClass);

	/** 
	 * Get or spawn an actor from the TemplateActorIndex
	 * @param MassAgent is the handle to the associated mass agent
	 * @param Transform where to create this actor
	 * @param TemplateActorIndex is the index of the type fetched with FindOrAddTemplateActor()
	 * @param SpawnRequestHandle [IN/OUT] IN: previously requested spawn OUT: newly requested spawn
	 * @param Priority of this spawn request in comparison with the others, lower value means higher priority (optional)
	 * @param ActorPreSpawnDelegate is an optional delegate called before the spawning of an actor
	 * @param ActorPostSpawnDelegate is an optional delegate called once the actor is spawned
	 * @return The spawned actor from the template actor type if ready
	 */
	AActor* GetOrSpawnActorFromTemplate(const FMassEntityHandle MassAgent, const FTransform& Transform, const int16 TemplateActorIndex, FMassActorSpawnRequestHandle& InOutSpawnRequestHandle, float Priority = MAX_FLT,
		FMassActorPreSpawnDelegate ActorPreSpawnDelegate = FMassActorPreSpawnDelegate(), FMassActorPostSpawnDelegate ActorPostSpawnDelegate = FMassActorPostSpawnDelegate());

	/**
	 * Cancel spawning request that is matching the TemplateActorIndex
	 * @param MassAgent is the handle to the associated mass agent
	 * @param TemplateActorIndex is the template type of the actor to release in case it was successfully spawned
	 * @param SpawnRequestHandle [IN/OUT] previously requested spawn, gets invalidated as a result of this call.
	 * @return True if spawning request was canceled
	 */
	bool CancelSpawning(const FMassEntityHandle MassAgent, const int16 TemplateActorIndex, FMassActorSpawnRequestHandle & SpawnRequestHandle);

	/**
	 * Release an actor that is matching the TemplateActorIndex
	 * @param MassAgent is the handle to the associated mass agent
	 * @param TemplateActorIndex is the template type of the actor to release in case it was successfully spawned
	 * @param ActorToRelease is the actual actor to release if any
	 * @param bImmediate means it needs to be done immediately and not queue for later
	 * @return True if actor was released
	 */
	bool ReleaseTemplateActor(const FMassEntityHandle MassAgent, const int16 TemplateActorIndex, AActor* ActorToRelease, bool bImmediate);

	/**
	 * Release an actor or cancel its spawning if it is matching the TemplateActorIndex
	 * @param MassAgent is the handle to the associated mass agent
	 * @param TemplateActorIndex is the template type of the actor to release in case it was successfully spawned
	 * @param ActorToRelease is the actual actor to release if any
	 * @param SpawnRequestHandle [IN/OUT] previously requested spawn, gets invalidated as a result of this call.
	 * @return True if actor was released or spawning request was canceled
	 */
	bool ReleaseTemplateActorOrCancelSpawning(const FMassEntityHandle MassAgent, const int16 TemplateActorIndex, AActor* ActorToRelease, FMassActorSpawnRequestHandle& SpawnRequestHandle);


	/**
	 * Compare if an actor matches the registered template actor
	 * @param Actor to compare its class against the template
	 * @param TemplateActorIndex is the template type of the actor to compare against
	 * @return True if actor matches the template
	 */
	bool DoesActorMatchTemplate(const AActor& Actor, const int16 TemplateActorIndex) const;

	TSubclassOf<AActor> GetTemplateActorClass(const int16 TemplateActorIndex);

	bool IsCollisionLoaded(const FName TargetGrid, const FTransform& Transform) const;

	/**
	 * Responds to the FMassEntityTemplate getting destroyed, and releases reference to corresponding Actor in TemplateActors
	 */
	void ReleaseTemplate(const TSubclassOf<AActor>& ActorClass);

	/**
	 * Release all references to static meshes and template actors
	 * Use with caution, all entities using this representation subsystem must be destroy otherwise they will point to invalid resources */
	void ReleaseAllResources();

	UMassActorSpawnerSubsystem* GetActorSpawnerSubsystem() const { return ActorSpawnerSubsystem; }

protected:
	// USubsystem BEGIN
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	// USubsystem END

	/** Needed for batching the update of static mesh transform */
	void OnProcessingPhaseStarted(const float DeltaSeconds, const EMassProcessingPhase Phase) const;

	void OnMassAgentComponentEntityAssociated(const UMassAgentComponent& AgentComponent);
	void OnMassAgentComponentEntityDetaching(const UMassAgentComponent& AgentComponent);

	bool ReleaseTemplateActorInternal(const int16 TemplateActorIndex, AActor* ActorToRelease, bool bImmediate);
	bool CancelSpawningInternal(const int16 TemplateActorIndex, FMassActorSpawnRequestHandle& SpawnRequestHandle);

	static void AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector);

protected:

	struct FTemplateActorData
	{
		TSubclassOf<AActor> Actor;
		uint32 RefCount{0u};
	};
	
	struct FTemplateActorEqualsPredicate
	{
		const TSubclassOf<AActor>& ActorClass;

		FTemplateActorEqualsPredicate(const TSubclassOf<AActor>& ActorClass) : ActorClass(ActorClass) {}

		bool operator()(const FTemplateActorData& ActorData) const
		{
			return ActorData.Actor == ActorClass;
		}
	};

	/** The array of all the template actors */
	TSparseArray<FTemplateActorData> TemplateActors;
	UE_MT_DECLARE_RW_ACCESS_DETECTOR(TemplateActorsMTAccessDetector);

	/** The component that handles all the static mesh instances */
	UPROPERTY(Transient)
	TObjectPtr<UMassVisualizationComponent> VisualizationComponent;

	/** The actor owning the above visualization component */
	UPROPERTY(Transient)
	TObjectPtr<AMassVisualizer> Visualizer;

	UPROPERTY(Transient)
	TObjectPtr<UMassActorSpawnerSubsystem> ActorSpawnerSubsystem;

	TSharedPtr<FMassEntityManager> EntityManager;

	UPROPERTY(Transient)
	TObjectPtr<UWorldPartitionSubsystem> WorldPartitionSubsystem;

	/** The time to wait before retrying a to spawn actor that failed */
	float RetryMovedDistanceSq = 1000000.0f;

	/** The distance a failed spawned actor needs to move before we retry */
	float RetryTimeInterval = 10.0f;

	/** Keeping track of all the mass agent this subsystem is responsible for spawning actors */
	TMap<FMassEntityHandle, int32> HandledMassAgents;
};

template<>
struct TMassExternalSubsystemTraits<UMassRepresentationSubsystem> final
{
	enum
	{
		GameThreadOnly = true
	};
};
