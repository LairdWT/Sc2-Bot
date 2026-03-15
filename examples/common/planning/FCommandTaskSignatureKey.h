#pragma once

#include <cstddef>
#include <cstdint>

#include "common/services/FBuildPlacementSlotId.h"
#include "sc2api/sc2_gametypes.h"
#include "sc2api/sc2_typeenums.h"

namespace sc2
{

struct FCommandOrderRecord;
struct FCommandTaskDescriptor;

struct FCommandTaskSignatureKey
{
public:
    FCommandTaskSignatureKey();

    void Reset();

    static FCommandTaskSignatureKey FromOrderRecord(const FCommandOrderRecord& CommandOrderRecordValue);
    static FCommandTaskSignatureKey FromTaskDescriptor(const FCommandTaskDescriptor& CommandTaskDescriptorValue);

    bool operator==(const FCommandTaskSignatureKey& CommandTaskSignatureKeyValue) const;

public:
    uint32_t TaskId;
    uint32_t SourceGoalId;
    AbilityID AbilityId;
    UNIT_TYPEID ResultUnitTypeId;
    UpgradeID UpgradeId;
    FBuildPlacementSlotId PreferredPlacementSlotId;
};

struct FCommandTaskSignatureKeyHash
{
public:
    size_t operator()(const FCommandTaskSignatureKey& CommandTaskSignatureKeyValue) const;
};

}  // namespace sc2
