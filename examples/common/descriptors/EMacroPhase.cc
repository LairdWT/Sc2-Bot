#include "common/descriptors/EMacroPhase.h"

namespace sc2
{

const char* ToString(const EMacroPhase MacroPhaseValue)
{
    switch (MacroPhaseValue)
    {
        case EMacroPhase::Opening:
            return "Opening";
        case EMacroPhase::EarlyGame:
            return "EarlyGame";
        case EMacroPhase::MidGame:
            return "MidGame";
        case EMacroPhase::LateGame:
            return "LateGame";
        case EMacroPhase::Recovery:
            return "Recovery";
        default:
            return "Opening";
    }
}

}  // namespace sc2
