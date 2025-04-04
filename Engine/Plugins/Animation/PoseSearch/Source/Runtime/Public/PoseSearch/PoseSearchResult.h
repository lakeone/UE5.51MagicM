// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "PoseSearch/PoseSearchCost.h"
#include "PoseSearch/PoseSearchDefines.h"
#include "PoseSearchResult.generated.h"

class UPoseSearchDatabase;
class UPoseSearchSchema;

namespace UE::PoseSearch
{
struct FSearchIndexAsset;

struct FSearchResult
{
	// best cost of the currently selected PoseIdx (it could be equal to ContinuingPoseCost)
	FPoseSearchCost PoseCost;
	int32 PoseIdx = INDEX_NONE;
	TWeakObjectPtr<const UPoseSearchDatabase> Database;
	float AssetTime = 0.f;
	bool bIsContinuingPoseSearch = false;

#if WITH_EDITOR && ENABLE_ANIM_DEBUG && UE_POSE_SEARCH_TRACE_ENABLED
	FPoseSearchCost BruteForcePoseCost;
	int32 BestPosePos = 0;
#endif // WITH_EDITOR && ENABLE_ANIM_DEBUG && UE_POSE_SEARCH_TRACE_ENABLED

	// Attempts to set the internal state to match the provided asset time including updating the internal DbPoseIdx. 
	// If the provided asset time is out of bounds for the currently playing asset then this function will reset the 
	// state back to the default state.
	void Update(float NewAssetTime);

	bool IsValid() const { return PoseIdx != INDEX_NONE && Database != nullptr; }

	void Reset() { PoseIdx = INDEX_NONE; Database = nullptr; AssetTime = 0.0f; bIsContinuingPoseSearch = false; }

	POSESEARCH_API const FSearchIndexAsset* GetSearchIndexAsset(bool bMandatory = false) const;
	
	bool CanAdvance(float DeltaTime) const;

	bool operator==(const FSearchResult& Other) const
	{
		// best cost of the currently selected PoseIdx (it could be equal to ContinuingPoseCost)
		return	PoseCost == Other.PoseCost &&
			PoseIdx == Other.PoseIdx &&
			Database == Other.Database &&
			AssetTime == Other.AssetTime &&
			bIsContinuingPoseSearch == Other.bIsContinuingPoseSearch;
	}

};

} // namespace UE::PoseSearch

USTRUCT(BlueprintType, Category="Animation|Pose Search")
struct POSESEARCH_API FPoseSearchBlueprintResult
{
	GENERATED_BODY()
public:

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category=State)
	TObjectPtr<const UObject> SelectedAnimation = nullptr;
	
	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category=State)
	float SelectedTime = 0.f;
	
	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category=State)
	bool bIsContinuingPoseSearch = false;
	
	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category=State)
	float WantedPlayRate = 0.f;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category=State)
	bool bLoop = false;
	
	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category=State)
	bool bIsMirrored = false;
	
	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category=State)
	FVector BlendParameters = FVector::ZeroVector;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category=State)
	TWeakObjectPtr<const UPoseSearchDatabase> SelectedDatabase = nullptr;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category=State)
	float SearchCost = MAX_flt;
};

