#pragma once

#include <cstddef>
#include <cstdint>

namespace sc2
{

enum class ECommandCommitmentClass : uint8_t
{
    MandatoryOpening,
    MandatoryRecovery,
    FlexibleMacro,
    Opportunistic,
};

constexpr size_t CommandCommitmentClassCountValue = 4U;

size_t GetCommandCommitmentClassIndex(ECommandCommitmentClass CommandCommitmentClassValue);
const char* ToString(ECommandCommitmentClass CommandCommitmentClassValue);

}  // namespace sc2
