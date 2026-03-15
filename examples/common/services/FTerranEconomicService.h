#pragma once

#include "common/services/IEconomicService.h"

namespace sc2
{

class FTerranEconomicService final : public IEconomicService
{
public:
    void RebuildEconomicState(const FAgentState& AgentStateValue,
                              const FEconomyDomainState& EconomyDomainStateValue,
                              FGameStateDescriptor& GameStateDescriptorValue) const final;
    bool CanReserveMandatory(const FGameStateDescriptor& GameStateDescriptorValue, uint32_t MineralsCostValue,
                             uint32_t VespeneCostValue, uint32_t SupplyCostValue) const final;
    bool CanReserveFlexible(const FGameStateDescriptor& GameStateDescriptorValue, uint32_t MineralsCostValue,
                            uint32_t VespeneCostValue, uint32_t SupplyCostValue) const final;
};

}  // namespace sc2
