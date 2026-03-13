#include "common/descriptors/FTerranGameStateDescriptorBuilder.h"

#include "common/armies/FArmyDomainState.h"
#include "common/bot_status_models.h"
#include "common/build_planning/FBuildPlanningState.h"
#include "common/descriptors/FGameStateDescriptor.h"
#include "common/descriptors/FMacroStateDescriptor.h"

namespace sc2
{

namespace
{

constexpr float WallStructureMatchRadiusSquaredValue = 6.25f;

bool DoesWallSlotAcceptUnitType(const EBuildPlacementSlotType BuildPlacementSlotTypeValue,
                                const UNIT_TYPEID CandidateUnitTypeIdValue)
{
    switch (BuildPlacementSlotTypeValue)
    {
        case EBuildPlacementSlotType::MainRampDepotLeft:
        case EBuildPlacementSlotType::MainRampDepotRight:
            return CandidateUnitTypeIdValue == UNIT_TYPEID::TERRAN_SUPPLYDEPOT ||
                   CandidateUnitTypeIdValue == UNIT_TYPEID::TERRAN_SUPPLYDEPOTLOWERED;
        case EBuildPlacementSlotType::MainRampBarracksWithAddon:
            return CandidateUnitTypeIdValue == UNIT_TYPEID::TERRAN_BARRACKS;
        default:
            return false;
    }
}

EObservedWallSlotState DetermineObservedWallSlotState(const FTerranUnitContainer& UnitContainerValue,
                                                      const EBuildPlacementSlotType BuildPlacementSlotTypeValue,
                                                      const Point2D& BuildPointValue)
{
    for (const Unit* ControlledUnitValue : UnitContainerValue.ControlledUnits)
    {
        if (ControlledUnitValue == nullptr || ControlledUnitValue->build_progress < 1.0f ||
            !DoesWallSlotAcceptUnitType(BuildPlacementSlotTypeValue, ControlledUnitValue->unit_type.ToType()))
        {
            continue;
        }

        const float DistanceSquaredValue = DistanceSquared2D(Point2D(ControlledUnitValue->pos), BuildPointValue);
        if (DistanceSquaredValue <= WallStructureMatchRadiusSquaredValue)
        {
            return EObservedWallSlotState::Occupied;
        }
    }

    return EObservedWallSlotState::Empty;
}

}  // namespace

void FTerranGameStateDescriptorBuilder::RebuildGameStateDescriptor(
    const uint64_t CurrentStepValue, const uint64_t CurrentGameLoopValue, const FAgentState& AgentStateValue,
    FGameStateDescriptor& GameStateDescriptorValue) const
{
    GameStateDescriptorValue.CurrentStep = CurrentStepValue;
    GameStateDescriptorValue.CurrentGameLoop = CurrentGameLoopValue;
    RebuildMacroStateDescriptor(CurrentGameLoopValue, AgentStateValue, GameStateDescriptorValue.MacroState);
    RebuildArmyDomainState(AgentStateValue, GameStateDescriptorValue.ArmyState);
    RebuildBuildPlanningState(CurrentGameLoopValue, AgentStateValue, GameStateDescriptorValue.BuildPlanning);
    GameStateDescriptorValue.ObservedRampWallState.Reset();
    if (GameStateDescriptorValue.RampWallDescriptor.bIsValid)
    {
        GameStateDescriptorValue.ObservedRampWallState.LeftDepotState = DetermineObservedWallSlotState(
            AgentStateValue.UnitContainer, EBuildPlacementSlotType::MainRampDepotLeft,
            GameStateDescriptorValue.RampWallDescriptor.LeftDepotSlot.BuildPoint);
        GameStateDescriptorValue.ObservedRampWallState.BarracksState = DetermineObservedWallSlotState(
            AgentStateValue.UnitContainer, EBuildPlacementSlotType::MainRampBarracksWithAddon,
            GameStateDescriptorValue.RampWallDescriptor.BarracksSlot.BuildPoint);
        GameStateDescriptorValue.ObservedRampWallState.RightDepotState = DetermineObservedWallSlotState(
            AgentStateValue.UnitContainer, EBuildPlacementSlotType::MainRampDepotRight,
            GameStateDescriptorValue.RampWallDescriptor.RightDepotSlot.BuildPoint);
    }
}

void FTerranGameStateDescriptorBuilder::RebuildMacroStateDescriptor(
    const uint64_t CurrentGameLoopValue, const FAgentState& AgentStateValue,
    FMacroStateDescriptor& MacroStateDescriptorValue) const
{
    MacroStateDescriptorValue.CurrentGameLoop = CurrentGameLoopValue;
    MacroStateDescriptorValue.ActiveBaseCount = AgentStateValue.Buildings.GetTownHallCount();
    MacroStateDescriptorValue.WorkerCount = AgentStateValue.Units.GetWorkerCount();
    MacroStateDescriptorValue.ArmyUnitCount = AgentStateValue.Units.GetArmyCount();
    MacroStateDescriptorValue.ArmySupply = AgentStateValue.Units.ArmySupply;
    MacroStateDescriptorValue.BarracksCount = AgentStateValue.Buildings.GetBarracksCount();
    MacroStateDescriptorValue.FactoryCount = AgentStateValue.Buildings.GetFactoryCount();
    MacroStateDescriptorValue.StarportCount = AgentStateValue.Buildings.GetStarportCount();
    MacroStateDescriptorValue.SupplyUsed = AgentStateValue.Economy.Supply;
    MacroStateDescriptorValue.SupplyCap = AgentStateValue.Economy.SupplyCap;
    MacroStateDescriptorValue.ActiveMacroPhase = DetermineMacroPhase(MacroStateDescriptorValue);
}

void FTerranGameStateDescriptorBuilder::RebuildArmyDomainState(const FAgentState& AgentStateValue,
                                                               FArmyDomainState& ArmyDomainStateValue) const
{
    ArmyDomainStateValue.ReserveUnitCount = AgentStateValue.Units.GetArmyCount();
    ArmyDomainStateValue.ActiveSquadCount = 0;
    ArmyDomainStateValue.EnsurePrimaryArmyExists();
}

void FTerranGameStateDescriptorBuilder::RebuildBuildPlanningState(
    const uint64_t CurrentGameLoopValue, const FAgentState& AgentStateValue,
    FBuildPlanningState& BuildPlanningStateValue) const
{
    BuildPlanningStateValue.Reset();
    BuildPlanningStateValue.CurrentGameLoop = CurrentGameLoopValue;
    BuildPlanningStateValue.AvailableMinerals = AgentStateValue.Economy.Minerals;
    BuildPlanningStateValue.AvailableVespene = AgentStateValue.Economy.Vespene;
    BuildPlanningStateValue.AvailableSupply = AgentStateValue.Economy.SupplyAvailable;
    BuildPlanningStateValue.ObservedTownHallCount = AgentStateValue.Buildings.GetTownHallCount();
    BuildPlanningStateValue.ObservedOrbitalCommandCount =
        AgentStateValue.Buildings.GetBuildingCount(UNIT_TYPEID::TERRAN_ORBITALCOMMAND) +
        AgentStateValue.Buildings.GetBuildingCount(UNIT_TYPEID::TERRAN_ORBITALCOMMANDFLYING);
    BuildPlanningStateValue.ObservedUnitCounts = AgentStateValue.Units.UnitCounts;
    BuildPlanningStateValue.ObservedUnitsInConstruction = AgentStateValue.Units.UnitsInConstruction;
    BuildPlanningStateValue.ObservedBuildingCounts = AgentStateValue.Buildings.BuildingCounts;
    BuildPlanningStateValue.ObservedBuildingsInConstruction = AgentStateValue.Buildings.CurrentlyInConstruction;
    BuildPlanningStateValue.ObservedCompletedUpgradeCounts = AgentStateValue.CompletedUpgradeCounts;
}

EMacroPhase FTerranGameStateDescriptorBuilder::DetermineMacroPhase(
    const FMacroStateDescriptor& MacroStateDescriptorValue) const
{
    if (MacroStateDescriptorValue.ActiveBaseCount == 0U ||
        (MacroStateDescriptorValue.ActiveBaseCount == 1U && MacroStateDescriptorValue.WorkerCount < 8U))
    {
        return EMacroPhase::Recovery;
    }

    if (MacroStateDescriptorValue.ActiveBaseCount >= 4U || MacroStateDescriptorValue.ArmySupply >= 120U)
    {
        return EMacroPhase::LateGame;
    }

    if (MacroStateDescriptorValue.ActiveBaseCount >= 2U &&
        (MacroStateDescriptorValue.StarportCount > 0U || MacroStateDescriptorValue.ArmySupply >= 40U))
    {
        return EMacroPhase::MidGame;
    }

    if (MacroStateDescriptorValue.ActiveBaseCount >= 2U || MacroStateDescriptorValue.BarracksCount >= 2U ||
        MacroStateDescriptorValue.WorkerCount >= 24U)
    {
        return EMacroPhase::EarlyGame;
    }

    return EMacroPhase::Opening;
}

}  // namespace sc2
