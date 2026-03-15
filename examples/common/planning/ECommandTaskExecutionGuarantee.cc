#include "common/planning/ECommandTaskExecutionGuarantee.h"

namespace sc2
{

const char* ToString(const ECommandTaskExecutionGuarantee CommandTaskExecutionGuaranteeValue)
{
    switch (CommandTaskExecutionGuaranteeValue)
    {
        case ECommandTaskExecutionGuarantee::MustExecute:
            return "MustExecute";
        case ECommandTaskExecutionGuarantee::Preferred:
            return "Preferred";
        case ECommandTaskExecutionGuarantee::Skippable:
            return "Skippable";
        default:
            return "Preferred";
    }
}

}  // namespace sc2
