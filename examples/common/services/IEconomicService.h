#pragma once

#include <cstdint>

namespace sc2
{

struct FAgentState;
struct FEconomyDomainState;
struct FGameStateDescriptor;

class IEconomicService
{
public:
    virtual ~IEconomicService() = default;

    virtual void RebuildEconomicState(const FAgentState& AgentStateValue,
                                      const FEconomyDomainState& EconomyDomainStateValue,
                                      FGameStateDescriptor& GameStateDescriptorValue) const = 0;
    virtual bool CanReserveMandatory(const FGameStateDescriptor& GameStateDescriptorValue,
                                     uint32_t MineralsCostValue, uint32_t VespeneCostValue,
                                     uint32_t SupplyCostValue) const = 0;
    virtual bool CanReserveFlexible(const FGameStateDescriptor& GameStateDescriptorValue,
                                    uint32_t MineralsCostValue, uint32_t VespeneCostValue,
                                    uint32_t SupplyCostValue) const = 0;
};

}  // namespace sc2
