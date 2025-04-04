// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "rltests/Defs.h"

#include "riglogic/joints/cpu/CPUJointsEvaluator.h"
#include "riglogic/joints/cpu/bpcm/BPCMJointsEvaluator.h"

namespace rl4 {

struct CPUJointsEvaluator::Accessor {

    static JointsEvaluator* getBPCMEvaluator(CPUJointsEvaluator* parent) {
        return parent->bpcmEvaluator.get();
    }

    static JointsEvaluator* getQuaternionEvaluator(CPUJointsEvaluator* parent) {
        return parent->quaternionEvaluator.get();
    }

};

namespace bpcm {

template<typename TValue>
struct Evaluator<TValue>::Accessor {

    static void assertRawDataEqual(const Evaluator<TValue>& result, const Evaluator<TValue>& expected) {
        ASSERT_EQ(result.storage.inputIndices, expected.storage.inputIndices);
        ASSERT_EQ(result.storage.outputIndices, expected.storage.outputIndices);
        ASSERT_EQ(result.storage.values, expected.storage.values);
        ASSERT_EQ(result.storage.outputRotationIndices, expected.storage.outputRotationIndices);
        ASSERT_EQ(result.storage.outputRotationLODs, expected.storage.outputRotationLODs);
    }

    static void assertJointGroupsEqual(const Evaluator<TValue>& result, const Evaluator<TValue>& expected) {
        ASSERT_EQ(result.storage.jointGroups.size(), expected.storage.jointGroups.size());
        for (std::size_t jgIdx = 0ul; jgIdx < expected.storage.jointGroups.size(); ++jgIdx) {
            const auto& jointGroup = result.storage.jointGroups[jgIdx];
            const auto& expectedJointGroup = expected.storage.jointGroups[jgIdx];
            ASSERT_EQ(jointGroup.inputIndicesOffset, expectedJointGroup.inputIndicesOffset);
            ASSERT_EQ(jointGroup.inputIndicesSize, expectedJointGroup.inputIndicesSize);
            ASSERT_EQ(jointGroup.inputIndicesSizeAlignedTo4, expectedJointGroup.inputIndicesSizeAlignedTo4);
            ASSERT_EQ(jointGroup.inputIndicesSizeAlignedTo8, expectedJointGroup.inputIndicesSizeAlignedTo8);
            ASSERT_EQ(jointGroup.lodsOffset, expectedJointGroup.lodsOffset);
            ASSERT_EQ(jointGroup.outputIndicesOffset, expectedJointGroup.outputIndicesOffset);
            ASSERT_EQ(jointGroup.outputRotationIndicesOffset, expectedJointGroup.outputRotationIndicesOffset);
            ASSERT_EQ(jointGroup.outputRotationLODsOffset, expectedJointGroup.outputRotationLODsOffset);
            ASSERT_EQ(jointGroup.valuesOffset, expectedJointGroup.valuesOffset);
            ASSERT_EQ(jointGroup.valuesSize, expectedJointGroup.valuesSize);
        }
    }

    static void assertLODsEqual(const Evaluator<TValue>& result, const Evaluator<TValue>& expected) {
        ASSERT_EQ(result.storage.lodRegions.size(), expected.storage.lodRegions.size());
        for (std::size_t lod = 0; lod < expected.storage.lodRegions.size(); ++lod) {
            const auto& lodRegion = result.storage.lodRegions[lod];
            const auto& expectedLodRegion = expected.storage.lodRegions[lod];
            ASSERT_EQ(lodRegion.size, expectedLodRegion.size);
            ASSERT_EQ(lodRegion.sizePaddedToLastFullBlock, expectedLodRegion.sizePaddedToLastFullBlock);
            ASSERT_EQ(lodRegion.sizePaddedToSecondLastFullBlock, expectedLodRegion.sizePaddedToSecondLastFullBlock);
        }
    }

};

}  // namespace bpcm

}  // namespace rl4
