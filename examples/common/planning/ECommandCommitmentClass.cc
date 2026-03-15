#include "common/planning/ECommandCommitmentClass.h"

namespace sc2
{

size_t GetCommandCommitmentClassIndex(const ECommandCommitmentClass CommandCommitmentClassValue)
{
    switch (CommandCommitmentClassValue)
    {
        case ECommandCommitmentClass::MandatoryOpening:
            return 0U;
        case ECommandCommitmentClass::MandatoryRecovery:
            return 1U;
        case ECommandCommitmentClass::FlexibleMacro:
            return 2U;
        case ECommandCommitmentClass::Opportunistic:
            return 3U;
        default:
            return 2U;
    }
}

const char* ToString(const ECommandCommitmentClass CommandCommitmentClassValue)
{
    switch (CommandCommitmentClassValue)
    {
        case ECommandCommitmentClass::MandatoryOpening:
            return "MandatoryOpening";
        case ECommandCommitmentClass::MandatoryRecovery:
            return "MandatoryRecovery";
        case ECommandCommitmentClass::FlexibleMacro:
            return "FlexibleMacro";
        case ECommandCommitmentClass::Opportunistic:
            return "Opportunistic";
        default:
            return "FlexibleMacro";
    }
}

}  // namespace sc2
