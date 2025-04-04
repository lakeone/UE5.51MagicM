// Copyright Epic Games, Inc. All Rights Reserved.

#include "AvaTransitionExecutionContext.h"

FAvaTransitionExecutionContext::FAvaTransitionExecutionContext(const FAvaTransitionBehaviorInstance& InBehaviorInstance, UObject& InOwner, const UStateTree& InStateTree, FStateTreeInstanceData& InInstanceData)
	: FStateTreeExecutionContext(InOwner, InStateTree, InInstanceData)
	, BehaviorInstance(InBehaviorInstance)
{
}

void FAvaTransitionExecutionContext::SetSceneDescription(FString&& InSceneDescription)
{
	SceneDescription = MoveTemp(InSceneDescription);
}

FString FAvaTransitionExecutionContext::GetInstanceDescription() const
{
	if (!SceneDescription.IsEmpty())
	{
		return SceneDescription;
	}
	return FStateTreeExecutionContext::GetInstanceDescription();
}
