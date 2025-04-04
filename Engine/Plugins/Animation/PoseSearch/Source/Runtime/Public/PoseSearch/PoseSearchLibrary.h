// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Animation/AnimExecutionContext.h"
#include "Animation/AnimNodeReference.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PoseSearch/PoseSearchDefines.h"
#include "PoseSearch/PoseSearchHistory.h"
#include "PoseSearch/PoseSearchResult.h"
#include "PoseSearch/PoseSearchRole.h"
#include "SequenceEvaluatorLibrary.h"
#include "SequencePlayerLibrary.h"
#include "PoseSearchLibrary.generated.h"

namespace UE::PoseSearch
{
	struct FSearchContext;
} // namespace UE::PoseSearch

struct FAnimationUpdateContext;
struct FAnimNode_PoseSearchHistoryCollector_Base;

UENUM()
enum class EPoseSearchInterruptMode : uint8
{
	// continuing pose search will be performed if valid
	DoNotInterrupt,

	// continuing pose search will be interrupted if its database is not listed in the searchable databases
	InterruptOnDatabaseChange,

	// continuing pose search will be interrupted if its database is not listed in the searchable databases, 
	// and continuing pose will be invalidated (forcing the schema to use pose history to build the query)
	InterruptOnDatabaseChangeAndInvalidateContinuingPose,

	// continuing pose search will always be interrupted
	ForceInterrupt,

	/// continuing pose search will always be interrupted
	// and continuing pose will be invalidated (forcing the schema to use pose history to build the query)
	ForceInterruptAndInvalidateContinuingPose,
};

PRAGMA_DISABLE_DEPRECATION_WARNINGS
struct FMotionMatchingState
{
PRAGMA_ENABLE_DEPRECATION_WARNINGS
	// Reset the state to a default state using the current Database
	void Reset(const FTransform& ComponentTransform);

	// Attempts to set the internal state to match the provided asset time including updating the internal DbPoseIdx. 
	// If the provided asset time is out of bounds for the currently playing asset then this function will reset the 
	// state back to the default state.
	void AdjustAssetTime(float AssetTime);

	// Internally stores the 'jump' to a new pose/sequence index and asset time for evaluation
	void JumpToPose(const FAnimationUpdateContext& Context, const UE::PoseSearch::FSearchResult& Result, int32 MaxActiveBlends, float BlendTime);

	void UpdateWantedPlayRate(const UE::PoseSearch::FSearchContext& SearchContext, const FFloatInterval& PlayRate, float TrajectorySpeedMultiplier);

	FVector GetEstimatedFutureRootMotionVelocity() const;

	UE::PoseSearch::FSearchResult CurrentSearchResult;

	// Time since the last pose jump
	float ElapsedPoseSearchTime = 0.f;

	// wanted PlayRate to have the selected animation playing at the estimated requested speed from the query.
	float WantedPlayRate = 1.f;

	// true if a new animation has been selected
	bool bJumpedToPose = false;

	UE::PoseSearch::FPoseIndicesHistory PoseIndicesHistory;

	// Component delta yaw (also considered as root bone delta yaw)
	UE_DEPRECATED(5.4, "Use Steering, OrientationWarping, OffsetRootBone nodes instead")
	float ComponentDeltaYaw = 0.f;

	// Internal component yaw in world space. Initialized as FRotator(AnimInstanceProxy->GetComponentTransform().GetRotation()).Yaw, but then integrated by ComponentDeltaYaw
	UE_DEPRECATED(5.4, "Use Steering, OrientationWarping, OffsetRootBone nodes instead")
	float ComponentWorldYaw = 0.f;
	
	// RootMotionTransformDelta yaw at the end of FAnimNode_MotionMatching::Evaluate_AnyThread (it represents the previous frame animation delta yaw)
	UE_DEPRECATED(5.4, "Use Steering, OrientationWarping, OffsetRootBone nodes instead")
	float AnimationDeltaYaw = 0.f;

#if UE_POSE_SEARCH_TRACE_ENABLED
	UE_DEPRECATED(5.4, "Debug RootMotionDelta is now calculated in-place.")
	FTransform RootMotionTransformDelta = FTransform::Identity;
#endif //UE_POSE_SEARCH_TRACE_ENABLED
};

USTRUCT(Experimental, BlueprintType, Category="Animation|Pose Search")
struct POSESEARCH_API FPoseSearchFutureProperties
{
	GENERATED_BODY()

public:
	// Animation to play (it'll start at AnimationTime seconds)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=State)
	TObjectPtr<const UObject> Animation;

	// Start time for Animation
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=State)
	float AnimationTime = 0.f;

	// Interval time before playing Animation
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=State)
	float IntervalTime = 0.f;
};

USTRUCT(Experimental, BlueprintType, Category="Animation|Pose Search")
struct POSESEARCH_API FPoseSearchContinuingProperties
{
	GENERATED_BODY()

public:
	// Currently playing animation
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=State)
	TObjectPtr<const UObject> PlayingAsset = nullptr;

	// Currently playing animation accumulated time
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=State)
	float PlayingAssetAccumulatedTime = 0.f;
};

UCLASS()
class POSESEARCH_API UPoseSearchLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

#if UE_POSE_SEARCH_TRACE_ENABLED

	static void TraceMotionMatching(
		UE::PoseSearch::FSearchContext& SearchContext,
		FMotionMatchingState& CurrentState,
		float DeltaTime,
		bool bSearch,
		float RecordingTime);
	
	UE_DEPRECATED(5.5, "Use TraceMotionMatching with different signature instead.")
	static void TraceMotionMatching(
		UE::PoseSearch::FSearchContext& SearchContext,
		const UE::PoseSearch::FSearchResult& CurrentResult,
		float ElapsedPoseSearchTime,
		const FTransform& RootMotionTransformDelta,
		float DeltaTime,
		bool bSearch,
		float RecordingTime)
	{
		FMotionMatchingState MotionMatchingState;
		MotionMatchingState.CurrentSearchResult = CurrentResult;
		MotionMatchingState.ElapsedPoseSearchTime = ElapsedPoseSearchTime;
		TraceMotionMatching(SearchContext, MotionMatchingState, DeltaTime, bSearch, RecordingTime);
	}

	UE_DEPRECATED(5.4, "Use TraceMotionMatching instead")
	static void TraceMotionMatchingState(
		UE::PoseSearch::FSearchContext& SearchContext,
		const UE::PoseSearch::FSearchResult& CurrentResult,
		float ElapsedPoseSearchTime,
		const FTransform& RootMotionTransformDelta,
		int32 NodeId,
		float DeltaTime,
		bool bSearch,
		float RecordingTime)
	{
		FMotionMatchingState MotionMatchingState;
		MotionMatchingState.CurrentSearchResult = CurrentResult;
		MotionMatchingState.ElapsedPoseSearchTime = ElapsedPoseSearchTime;
		TraceMotionMatching(SearchContext, MotionMatchingState, DeltaTime, bSearch, RecordingTime);
	}
	
#endif // UE_POSE_SEARCH_TRACE_ENABLED

public:
	/**
	* Implementation of the core motion matching algorithm
	*
	* @param Context						Input animation update context providing access to the proxy and delta time
	* @param Databases						Input array of databases to search
	* @param BlendTime						Input time in seconds to blend out to the new pose. Uses either inertial blending, requiring an Inertialization node after this node, or the internal blend stack, if MaxActiveBlends is greater than zero.
	* @param MaxActiveBlends				Input number of max active animation segments being blended together in the blend stack. If MaxActiveBlends is zero then the blend stack is disabled.
	* @param PoseJumpThresholdTime			Input don't jump to poses of the same segment that are within the interval this many seconds away from the continuing pose.
	* @param PoseReselectHistory			Input prevent re-selection of poses that have been selected previously within this much time (in seconds) in the past. This is across all animation segments that have been selected within this time range.
	* @param SearchThrottleTime				Input minimum amount of time to wait between searching for a new pose segment. It allows users to define how often the system searches, default for locomotion is searching every update, but you may only want to search once for other situations, like jump.
	* @param PlayRate						Input effective range of play rate that can be applied to the animations to account for discrepancies in estimated velocity between the movement modeland the animation.
	* @param InOutMotionMatchingState		Input/Output encapsulated motion matching algorithm and state
	* @param InterruptMode					Input continuing pose search interrupt mode
	* @param bShouldSearch					Input if false search will happen only if there's no valid continuing pose
	* @param bDebugDrawQuery				Input draw the composed query if valid
	* @param bDebugDrawCurResult			Input draw the current result if valid
	*/
	static void UpdateMotionMatchingState(
		const FAnimationUpdateContext& Context,
		const TArray<TObjectPtr<const UPoseSearchDatabase>>& Databases,
		float BlendTime,
		int32 MaxActiveBlends,
		const FFloatInterval& PoseJumpThresholdTime,
		float PoseReselectHistory,
		float SearchThrottleTime,
		const FFloatInterval& PlayRate,
		FMotionMatchingState& InOutMotionMatchingState,
		EPoseSearchInterruptMode InterruptMode = EPoseSearchInterruptMode::DoNotInterrupt,
		bool bShouldSearch = true,
		bool bShouldUseCachedChannelData = true,
		bool bDebugDrawQuery = false,
		bool bDebugDrawCurResult = false);

	/**
	* Implementation of the core motion matching algorithm
	*
	* @param AnimInstance					Input animation instance
	* @param AssetsToSearch					Input assets to search (UPoseSearchDatabase or any animation asset containing UAnimNotifyState_PoseSearchBranchIn)
	* @param PoseHistoryName				Input tag of the associated PoseSearchHistoryCollector node in the anim graph
	* @param Future							Input future properties to match (animation / start time / time offset)
	* @param SelectedAnimation				Output selected animation from the Database asset
	* @param Result							Output FPoseSearchBlueprintResult with the search result
	*/
	UFUNCTION(BlueprintPure, Category = "Animation|Pose Search|Experimental", meta = (BlueprintThreadSafe, Keywords = "PoseMatch"))
	static void MotionMatch(
		UAnimInstance* AnimInstance,
		TArray<UObject*> AssetsToSearch,
		const FName PoseHistoryName,
		const FPoseSearchContinuingProperties ContinuingProperties,
		const FPoseSearchFutureProperties Future,
		FPoseSearchBlueprintResult& Result);

	static void MotionMatch(
		const TArrayView<UAnimInstance*> AnimInstances,
		const TArrayView<const UE::PoseSearch::FRole> Roles,
		const TArrayView<const UObject*> AssetsToSearch,
		const FName PoseHistoryName,
		const FPoseSearchContinuingProperties& ContinuingProperties,
		const FPoseSearchFutureProperties& Future,
		FPoseSearchBlueprintResult& Result);

	static UE::PoseSearch::FSearchResult MotionMatch(
		const TArrayView<UAnimInstance*> AnimInstances,
		const TArrayView<const UE::PoseSearch::FRole> Roles,
		const TArrayView<const UE::PoseSearch::IPoseHistory*> PoseHistories, 
		const TArrayView<const UObject*> AssetsToSearch,
		const FPoseSearchContinuingProperties& ContinuingProperties,
		const FPoseSearchFutureProperties& Future);

	UE_DEPRECATED(5.4, "Use other MotionMatch signatures instead")
	static void MotionMatch(
		const TArrayView<UAnimInstance*> AnimInstances,
		const TArrayView<const UE::PoseSearch::FRole> Roles,
		const TArrayView<const UObject*> AssetsToSearch,
		const FName PoseHistoryName,
		const FPoseSearchContinuingProperties& ContinuingProperties,
		const FPoseSearchFutureProperties& Future,
		FPoseSearchBlueprintResult& Result,
		const int32 DebugSessionUniqueIdentifier);

	UE_DEPRECATED(5.4, "Use other MotionMatch signatures instead")
	static UE::PoseSearch::FSearchResult MotionMatch(
		const TArrayView<UAnimInstance*> AnimInstances,
		const TArrayView<const UE::PoseSearch::FRole> Roles,
		const TArrayView<const UE::PoseSearch::IPoseHistory*> PoseHistories, 
		const TArrayView<const UObject*> AssetsToSearch,
		const FPoseSearchContinuingProperties& ContinuingProperties,
		const FPoseSearchFutureProperties& Future,
		const int32 DebugSessionUniqueIdentifier);

	UE_DEPRECATED(5.4, "Use other MotionMatch signatures instead")
	static UE::PoseSearch::FSearchResult MotionMatch(
		const FAnimationBaseContext& Context,
		TArrayView<const UObject*> AssetsToSearch,
		const FPoseSearchContinuingProperties& ContinuingProperties);
		
	UE_DEPRECATED(5.4, "Use other MotionMatch signatures instead")
	static UE::PoseSearch::FSearchResult MotionMatch(
		TArrayView<UAnimInstance*> AnimInstances,
		TArrayView<const UE::PoseSearch::FRole> Roles,
		TArrayView<const UE::PoseSearch::IPoseHistory*> PoseHistories, 
		TArrayView<const UObject*> AssetsToSearch,
		const FPoseSearchContinuingProperties& ContinuingProperties,
		const int32 DebugSessionUniqueIdentifier,
		float DesiredPermutationTimeOffset = 0.f);

	static const FAnimNode_PoseSearchHistoryCollector_Base* FindPoseHistoryNode(
		const FName PoseHistoryName,
		const UAnimInstance* AnimInstance);

	UFUNCTION(BlueprintPure, Category = "Animation|Pose Search|Experimental", meta = (BlueprintThreadSafe))
	static void IsAnimationAssetLooping(const UObject* Asset, bool& bIsAssetLooping);

	UFUNCTION(BlueprintPure, Category = "Animation|Pose Search|Experimental", meta = (BlueprintThreadSafe))
	static void GetDatabaseTags(const UPoseSearchDatabase* Database, TArray<FName>& Tags);
};

