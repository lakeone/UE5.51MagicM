// Copyright Epic Games, Inc. All Rights Reserved.

#include "riglogic/animatedmaps/AnimatedMapsNullOutputInstance.h"

#include "riglogic/TypeDefs.h"

namespace rl4 {

ArrayView<float> AnimatedMapsNullOutputInstance::getOutputBuffer() {
    return {};
}

void AnimatedMapsNullOutputInstance::resetOutputBuffer() {
}

}  // namespace rl4
