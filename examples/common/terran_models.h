#pragma once

#include <cstddef>
#include <cstdint>

#include "sc2api/sc2_typeenums.h"

namespace sc2 {

static constexpr size_t NUM_TERRAN_UNITS = 25;
static constexpr size_t NUM_TERRAN_BUILDINGS = 30;
static constexpr size_t NUM_TERRAN_UPGRADES = 27;
static constexpr size_t INVALID_TERRAN_UNIT_TYPE_INDEX = NUM_TERRAN_UNITS;
static constexpr size_t INVALID_TERRAN_BUILDING_TYPE_INDEX = NUM_TERRAN_BUILDINGS;
static constexpr size_t INVALID_TERRAN_UPGRADE_TYPE_INDEX = NUM_TERRAN_UPGRADES;

static constexpr UNIT_TYPEID TERRAN_UNIT_TYPES[NUM_TERRAN_UNITS] = {
    UNIT_TYPEID::TERRAN_MARINE,
    UNIT_TYPEID::TERRAN_MARAUDER,
    UNIT_TYPEID::TERRAN_MEDIVAC,
    UNIT_TYPEID::TERRAN_WIDOWMINE,
    UNIT_TYPEID::TERRAN_WIDOWMINEBURROWED,
    UNIT_TYPEID::TERRAN_AUTOTURRET,
    UNIT_TYPEID::TERRAN_BANSHEE,
    UNIT_TYPEID::TERRAN_BATTLECRUISER,
    UNIT_TYPEID::TERRAN_CYCLONE,
    UNIT_TYPEID::TERRAN_GHOST,
    UNIT_TYPEID::TERRAN_HELLION,
    UNIT_TYPEID::TERRAN_HELLIONTANK,
    UNIT_TYPEID::TERRAN_LIBERATOR,
    UNIT_TYPEID::TERRAN_LIBERATORAG,
    UNIT_TYPEID::TERRAN_MULE,
    UNIT_TYPEID::TERRAN_NUKE,
    UNIT_TYPEID::TERRAN_RAVEN,
    UNIT_TYPEID::TERRAN_REAPER,
    UNIT_TYPEID::TERRAN_SCV,
    UNIT_TYPEID::TERRAN_SIEGETANK,
    UNIT_TYPEID::TERRAN_SIEGETANKSIEGED,
    UNIT_TYPEID::TERRAN_THOR,
    UNIT_TYPEID::TERRAN_THORAP,
    UNIT_TYPEID::TERRAN_VIKINGASSAULT,
    UNIT_TYPEID::TERRAN_VIKINGFIGHTER
};

inline bool IsTerranUnitTypeIndexValid(const size_t UnitTypeIndexValue) {
    return UnitTypeIndexValue < NUM_TERRAN_UNITS;
}

inline bool TryGetTerranUnitTypeIndex(const UNIT_TYPEID UnitType, size_t& UnitTypeIndexValue) {
    switch (UnitType) {
        case UNIT_TYPEID::TERRAN_MARINE:
            UnitTypeIndexValue = 0;
            return true;
        case UNIT_TYPEID::TERRAN_MARAUDER:
            UnitTypeIndexValue = 1;
            return true;
        case UNIT_TYPEID::TERRAN_MEDIVAC:
            UnitTypeIndexValue = 2;
            return true;
        case UNIT_TYPEID::TERRAN_WIDOWMINE:
            UnitTypeIndexValue = 3;
            return true;
        case UNIT_TYPEID::TERRAN_WIDOWMINEBURROWED:
            UnitTypeIndexValue = 4;
            return true;
        case UNIT_TYPEID::TERRAN_AUTOTURRET:
            UnitTypeIndexValue = 5;
            return true;
        case UNIT_TYPEID::TERRAN_BANSHEE:
            UnitTypeIndexValue = 6;
            return true;
        case UNIT_TYPEID::TERRAN_BATTLECRUISER:
            UnitTypeIndexValue = 7;
            return true;
        case UNIT_TYPEID::TERRAN_CYCLONE:
            UnitTypeIndexValue = 8;
            return true;
        case UNIT_TYPEID::TERRAN_GHOST:
            UnitTypeIndexValue = 9;
            return true;
        case UNIT_TYPEID::TERRAN_HELLION:
            UnitTypeIndexValue = 10;
            return true;
        case UNIT_TYPEID::TERRAN_HELLIONTANK:
            UnitTypeIndexValue = 11;
            return true;
        case UNIT_TYPEID::TERRAN_LIBERATOR:
            UnitTypeIndexValue = 12;
            return true;
        case UNIT_TYPEID::TERRAN_LIBERATORAG:
            UnitTypeIndexValue = 13;
            return true;
        case UNIT_TYPEID::TERRAN_MULE:
            UnitTypeIndexValue = 14;
            return true;
        case UNIT_TYPEID::TERRAN_NUKE:
            UnitTypeIndexValue = 15;
            return true;
        case UNIT_TYPEID::TERRAN_RAVEN:
            UnitTypeIndexValue = 16;
            return true;
        case UNIT_TYPEID::TERRAN_REAPER:
            UnitTypeIndexValue = 17;
            return true;
        case UNIT_TYPEID::TERRAN_SCV:
            UnitTypeIndexValue = 18;
            return true;
        case UNIT_TYPEID::TERRAN_SIEGETANK:
            UnitTypeIndexValue = 19;
            return true;
        case UNIT_TYPEID::TERRAN_SIEGETANKSIEGED:
            UnitTypeIndexValue = 20;
            return true;
        case UNIT_TYPEID::TERRAN_THOR:
            UnitTypeIndexValue = 21;
            return true;
        case UNIT_TYPEID::TERRAN_THORAP:
            UnitTypeIndexValue = 22;
            return true;
        case UNIT_TYPEID::TERRAN_VIKINGASSAULT:
            UnitTypeIndexValue = 23;
            return true;
        case UNIT_TYPEID::TERRAN_VIKINGFIGHTER:
            UnitTypeIndexValue = 24;
            return true;
        default:
            UnitTypeIndexValue = INVALID_TERRAN_UNIT_TYPE_INDEX;
            return false;
    }
}

inline size_t GetTerranUnitTypeIndex(const UNIT_TYPEID UnitType) {
    size_t UnitTypeIndexValue = INVALID_TERRAN_UNIT_TYPE_INDEX;
    const bool FoundValue = TryGetTerranUnitTypeIndex(UnitType, UnitTypeIndexValue);
    (void)FoundValue;
    return UnitTypeIndexValue;
}

inline bool IsTerranUnit(const UNIT_TYPEID UnitType) {
    switch (UnitType) {
        case UNIT_TYPEID::TERRAN_MARINE:
        case UNIT_TYPEID::TERRAN_MARAUDER:
        case UNIT_TYPEID::TERRAN_MEDIVAC:
        case UNIT_TYPEID::TERRAN_WIDOWMINE:
        case UNIT_TYPEID::TERRAN_WIDOWMINEBURROWED:
        case UNIT_TYPEID::TERRAN_AUTOTURRET:
        case UNIT_TYPEID::TERRAN_BANSHEE:
        case UNIT_TYPEID::TERRAN_BATTLECRUISER:
        case UNIT_TYPEID::TERRAN_CYCLONE:
        case UNIT_TYPEID::TERRAN_GHOST:
        case UNIT_TYPEID::TERRAN_HELLION:
        case UNIT_TYPEID::TERRAN_HELLIONTANK:
        case UNIT_TYPEID::TERRAN_LIBERATOR:
        case UNIT_TYPEID::TERRAN_LIBERATORAG:
        case UNIT_TYPEID::TERRAN_MULE:
        case UNIT_TYPEID::TERRAN_NUKE:
        case UNIT_TYPEID::TERRAN_RAVEN:
        case UNIT_TYPEID::TERRAN_REAPER:
        case UNIT_TYPEID::TERRAN_SCV:
        case UNIT_TYPEID::TERRAN_SIEGETANK:
        case UNIT_TYPEID::TERRAN_SIEGETANKSIEGED:
        case UNIT_TYPEID::TERRAN_THOR:
        case UNIT_TYPEID::TERRAN_THORAP:
        case UNIT_TYPEID::TERRAN_VIKINGASSAULT:
        case UNIT_TYPEID::TERRAN_VIKINGFIGHTER:
            return true;
        default:
            return false;  // Invalid type
    }
}

inline bool IsTrainTerranUnit(const ABILITY_ID Order) {
    switch (Order) {
        case ABILITY_ID::TRAIN_MARINE:
        case ABILITY_ID::TRAIN_MARAUDER:
        case ABILITY_ID::TRAIN_MEDIVAC:
        case ABILITY_ID::TRAIN_WIDOWMINE:
        case ABILITY_ID::TRAIN_BANSHEE:
        case ABILITY_ID::TRAIN_BATTLECRUISER:
        case ABILITY_ID::TRAIN_CYCLONE:
        case ABILITY_ID::TRAIN_GHOST:
        case ABILITY_ID::TRAIN_HELLION:
        case ABILITY_ID::TRAIN_LIBERATOR:
        case ABILITY_ID::TRAIN_RAVEN:
        case ABILITY_ID::TRAIN_REAPER:
        case ABILITY_ID::TRAIN_SCV:
        case ABILITY_ID::TRAIN_SIEGETANK:
        case ABILITY_ID::TRAIN_THOR:
        case ABILITY_ID::TRAIN_VIKINGFIGHTER:
            return true;
        default:
            return false;
    }
}

inline UNIT_TYPEID TerranUnitTrainToUnitType(const ABILITY_ID Order) {
    switch (Order) {
        case ABILITY_ID::TRAIN_MARINE:
            return UNIT_TYPEID::TERRAN_MARINE;
        case ABILITY_ID::TRAIN_MARAUDER:
            return UNIT_TYPEID::TERRAN_MARAUDER;
        case ABILITY_ID::TRAIN_MEDIVAC:
            return UNIT_TYPEID::TERRAN_MEDIVAC;
        case ABILITY_ID::TRAIN_WIDOWMINE:
            return UNIT_TYPEID::TERRAN_WIDOWMINE;
        case ABILITY_ID::TRAIN_BANSHEE:
            return UNIT_TYPEID::TERRAN_BANSHEE;
        case ABILITY_ID::TRAIN_BATTLECRUISER:
            return UNIT_TYPEID::TERRAN_BATTLECRUISER;
        case ABILITY_ID::TRAIN_CYCLONE:
            return UNIT_TYPEID::TERRAN_CYCLONE;
        case ABILITY_ID::TRAIN_GHOST:
            return UNIT_TYPEID::TERRAN_GHOST;
        case ABILITY_ID::TRAIN_HELLION:
            return UNIT_TYPEID::TERRAN_HELLION;
        case ABILITY_ID::TRAIN_LIBERATOR:
            return UNIT_TYPEID::TERRAN_LIBERATOR;
        case ABILITY_ID::TRAIN_RAVEN:
            return UNIT_TYPEID::TERRAN_RAVEN;
        case ABILITY_ID::TRAIN_REAPER:
            return UNIT_TYPEID::TERRAN_REAPER;
        case ABILITY_ID::TRAIN_SCV:
            return UNIT_TYPEID::TERRAN_SCV;
        case ABILITY_ID::TRAIN_SIEGETANK:
            return UNIT_TYPEID::TERRAN_SIEGETANK;
        case ABILITY_ID::TRAIN_THOR:
            return UNIT_TYPEID::TERRAN_THOR;
        case ABILITY_ID::TRAIN_VIKINGFIGHTER:
            return UNIT_TYPEID::TERRAN_VIKINGFIGHTER;
        default:
            return UNIT_TYPEID::INVALID;
    }
}

static constexpr UNIT_TYPEID TERRAN_BUILDING_TYPES[NUM_TERRAN_BUILDINGS] = {
    UNIT_TYPEID::TERRAN_ARMORY,
    UNIT_TYPEID::TERRAN_BARRACKS,
    UNIT_TYPEID::TERRAN_BARRACKSFLYING,
    UNIT_TYPEID::TERRAN_BARRACKSREACTOR,
    UNIT_TYPEID::TERRAN_BARRACKSTECHLAB,
    UNIT_TYPEID::TERRAN_BUNKER,
    UNIT_TYPEID::TERRAN_COMMANDCENTER,
    UNIT_TYPEID::TERRAN_COMMANDCENTERFLYING,
    UNIT_TYPEID::TERRAN_ORBITALCOMMAND,
    UNIT_TYPEID::TERRAN_ORBITALCOMMANDFLYING,
    UNIT_TYPEID::TERRAN_PLANETARYFORTRESS,
    UNIT_TYPEID::TERRAN_ENGINEERINGBAY,
    UNIT_TYPEID::TERRAN_FACTORY,
    UNIT_TYPEID::TERRAN_FACTORYFLYING,
    UNIT_TYPEID::TERRAN_FACTORYREACTOR,
    UNIT_TYPEID::TERRAN_FACTORYTECHLAB,
    UNIT_TYPEID::TERRAN_FUSIONCORE,
    UNIT_TYPEID::TERRAN_GHOSTACADEMY,
    UNIT_TYPEID::TERRAN_MISSILETURRET,
    UNIT_TYPEID::TERRAN_REACTOR,
    UNIT_TYPEID::TERRAN_REFINERY,
    UNIT_TYPEID::TERRAN_REFINERYRICH,
    UNIT_TYPEID::TERRAN_SENSORTOWER,
    UNIT_TYPEID::TERRAN_STARPORT,
    UNIT_TYPEID::TERRAN_STARPORTFLYING,
    UNIT_TYPEID::TERRAN_STARPORTREACTOR,
    UNIT_TYPEID::TERRAN_STARPORTTECHLAB,
    UNIT_TYPEID::TERRAN_SUPPLYDEPOT,
    UNIT_TYPEID::TERRAN_SUPPLYDEPOTLOWERED,
    UNIT_TYPEID::TERRAN_TECHLAB
};

inline bool IsTerranBuildingTypeIndexValid(const size_t BuildingTypeIndexValue) {
    return BuildingTypeIndexValue < NUM_TERRAN_BUILDINGS;
}

inline bool TryGetTerranBuildingTypeIndex(const UNIT_TYPEID BuildingType, size_t& BuildingTypeIndexValue) {
    switch (BuildingType) {
        case UNIT_TYPEID::TERRAN_ARMORY:
            BuildingTypeIndexValue = 0;
            return true;
        case UNIT_TYPEID::TERRAN_BARRACKS:
            BuildingTypeIndexValue = 1;
            return true;
        case UNIT_TYPEID::TERRAN_BARRACKSFLYING:
            BuildingTypeIndexValue = 2;
            return true;
        case UNIT_TYPEID::TERRAN_BARRACKSREACTOR:
            BuildingTypeIndexValue = 3;
            return true;
        case UNIT_TYPEID::TERRAN_BARRACKSTECHLAB:
            BuildingTypeIndexValue = 4;
            return true;
        case UNIT_TYPEID::TERRAN_BUNKER:
            BuildingTypeIndexValue = 5;
            return true;
        case UNIT_TYPEID::TERRAN_COMMANDCENTER:
            BuildingTypeIndexValue = 6;
            return true;
        case UNIT_TYPEID::TERRAN_COMMANDCENTERFLYING:
            BuildingTypeIndexValue = 7;
            return true;
        case UNIT_TYPEID::TERRAN_ORBITALCOMMAND:
            BuildingTypeIndexValue = 8;
            return true;
        case UNIT_TYPEID::TERRAN_ORBITALCOMMANDFLYING:
            BuildingTypeIndexValue = 9;
            return true;
        case UNIT_TYPEID::TERRAN_PLANETARYFORTRESS:
            BuildingTypeIndexValue = 10;
            return true;
        case UNIT_TYPEID::TERRAN_ENGINEERINGBAY:
            BuildingTypeIndexValue = 11;
            return true;
        case UNIT_TYPEID::TERRAN_FACTORY:
            BuildingTypeIndexValue = 12;
            return true;
        case UNIT_TYPEID::TERRAN_FACTORYFLYING:
            BuildingTypeIndexValue = 13;
            return true;
        case UNIT_TYPEID::TERRAN_FACTORYREACTOR:
            BuildingTypeIndexValue = 14;
            return true;
        case UNIT_TYPEID::TERRAN_FACTORYTECHLAB:
            BuildingTypeIndexValue = 15;
            return true;
        case UNIT_TYPEID::TERRAN_FUSIONCORE:
            BuildingTypeIndexValue = 16;
            return true;
        case UNIT_TYPEID::TERRAN_GHOSTACADEMY:
            BuildingTypeIndexValue = 17;
            return true;
        case UNIT_TYPEID::TERRAN_MISSILETURRET:
            BuildingTypeIndexValue = 18;
            return true;
        case UNIT_TYPEID::TERRAN_REACTOR:
            BuildingTypeIndexValue = 19;
            return true;
        case UNIT_TYPEID::TERRAN_REFINERY:
            BuildingTypeIndexValue = 20;
            return true;
        case UNIT_TYPEID::TERRAN_REFINERYRICH:
            BuildingTypeIndexValue = 21;
            return true;
        case UNIT_TYPEID::TERRAN_SENSORTOWER:
            BuildingTypeIndexValue = 22;
            return true;
        case UNIT_TYPEID::TERRAN_STARPORT:
            BuildingTypeIndexValue = 23;
            return true;
        case UNIT_TYPEID::TERRAN_STARPORTFLYING:
            BuildingTypeIndexValue = 24;
            return true;
        case UNIT_TYPEID::TERRAN_STARPORTREACTOR:
            BuildingTypeIndexValue = 25;
            return true;
        case UNIT_TYPEID::TERRAN_STARPORTTECHLAB:
            BuildingTypeIndexValue = 26;
            return true;
        case UNIT_TYPEID::TERRAN_SUPPLYDEPOT:
            BuildingTypeIndexValue = 27;
            return true;
        case UNIT_TYPEID::TERRAN_SUPPLYDEPOTLOWERED:
            BuildingTypeIndexValue = 28;
            return true;
        case UNIT_TYPEID::TERRAN_TECHLAB:
            BuildingTypeIndexValue = 29;
            return true;
        default:
            BuildingTypeIndexValue = INVALID_TERRAN_BUILDING_TYPE_INDEX;
            return false;
    }
}

inline size_t GetTerranBuildingTypeIndex(const UNIT_TYPEID BuildingType) {
    size_t BuildingTypeIndexValue = INVALID_TERRAN_BUILDING_TYPE_INDEX;
    const bool FoundValue = TryGetTerranBuildingTypeIndex(BuildingType, BuildingTypeIndexValue);
    (void)FoundValue;
    return BuildingTypeIndexValue;
}

inline bool IsTerranBuilding(const UNIT_TYPEID BuildingType) {
    switch (BuildingType) {
        case UNIT_TYPEID::TERRAN_ARMORY:
        case UNIT_TYPEID::TERRAN_BARRACKS:
        case UNIT_TYPEID::TERRAN_BARRACKSFLYING:
        case UNIT_TYPEID::TERRAN_BARRACKSREACTOR:
        case UNIT_TYPEID::TERRAN_BARRACKSTECHLAB:
        case UNIT_TYPEID::TERRAN_BUNKER:
        case UNIT_TYPEID::TERRAN_COMMANDCENTER:
        case UNIT_TYPEID::TERRAN_COMMANDCENTERFLYING:
        case UNIT_TYPEID::TERRAN_ORBITALCOMMAND:
        case UNIT_TYPEID::TERRAN_ORBITALCOMMANDFLYING:
        case UNIT_TYPEID::TERRAN_PLANETARYFORTRESS:
        case UNIT_TYPEID::TERRAN_ENGINEERINGBAY:
        case UNIT_TYPEID::TERRAN_FACTORY:
        case UNIT_TYPEID::TERRAN_FACTORYFLYING:
        case UNIT_TYPEID::TERRAN_FACTORYREACTOR:
        case UNIT_TYPEID::TERRAN_FACTORYTECHLAB:
        case UNIT_TYPEID::TERRAN_FUSIONCORE:
        case UNIT_TYPEID::TERRAN_GHOSTACADEMY:
        case UNIT_TYPEID::TERRAN_MISSILETURRET:
        case UNIT_TYPEID::TERRAN_REACTOR:
        case UNIT_TYPEID::TERRAN_REFINERY:
        case UNIT_TYPEID::TERRAN_REFINERYRICH:
        case UNIT_TYPEID::TERRAN_SENSORTOWER:
        case UNIT_TYPEID::TERRAN_STARPORT:
        case UNIT_TYPEID::TERRAN_STARPORTFLYING:
        case UNIT_TYPEID::TERRAN_STARPORTREACTOR:
        case UNIT_TYPEID::TERRAN_STARPORTTECHLAB:
        case UNIT_TYPEID::TERRAN_SUPPLYDEPOT:
        case UNIT_TYPEID::TERRAN_SUPPLYDEPOTLOWERED:
        case UNIT_TYPEID::TERRAN_TECHLAB:
            return true;
        default:
            return false;
    }
}

static constexpr ABILITY_ID TERRAN_RESEARCH_UPGRADE_TYPES[NUM_TERRAN_UPGRADES] = {
    ABILITY_ID::RESEARCH_STIMPACK,
    ABILITY_ID::RESEARCH_COMBATSHIELD,
    ABILITY_ID::RESEARCH_CONCUSSIVESHELLS,
    ABILITY_ID::RESEARCH_INFERNALPREIGNITER,
    ABILITY_ID::RESEARCH_DRILLINGCLAWS,
    ABILITY_ID::RESEARCH_SMARTSERVOS,
    ABILITY_ID::RESEARCH_CYCLONERAPIDFIRELAUNCHERS,
    ABILITY_ID::RESEARCH_LIBERATORAGMODE,
    ABILITY_ID::RESEARCH_RAVENCORVIDREACTOR,
    ABILITY_ID::RESEARCH_HISECAUTOTRACKING,
    ABILITY_ID::RESEARCH_TERRANSTRUCTUREARMORUPGRADE,
    ABILITY_ID::RESEARCH_TERRANINFANTRYWEAPONSLEVEL1,
    ABILITY_ID::RESEARCH_TERRANINFANTRYWEAPONSLEVEL2,
    ABILITY_ID::RESEARCH_TERRANINFANTRYWEAPONSLEVEL3,
    ABILITY_ID::RESEARCH_TERRANINFANTRYARMORLEVEL1,
    ABILITY_ID::RESEARCH_TERRANINFANTRYARMORLEVEL2,
    ABILITY_ID::RESEARCH_TERRANINFANTRYARMORLEVEL3,
    ABILITY_ID::RESEARCH_TERRANVEHICLEWEAPONSLEVEL1,
    ABILITY_ID::RESEARCH_TERRANVEHICLEWEAPONSLEVEL2,
    ABILITY_ID::RESEARCH_TERRANVEHICLEWEAPONSLEVEL3,
    ABILITY_ID::RESEARCH_TERRANSHIPWEAPONSLEVEL1,
    ABILITY_ID::RESEARCH_TERRANSHIPWEAPONSLEVEL2,
    ABILITY_ID::RESEARCH_TERRANSHIPWEAPONSLEVEL3,
    ABILITY_ID::RESEARCH_TERRANVEHICLEANDSHIPPLATINGLEVEL1,
    ABILITY_ID::RESEARCH_TERRANVEHICLEANDSHIPPLATINGLEVEL2,
    ABILITY_ID::RESEARCH_TERRANVEHICLEANDSHIPPLATINGLEVEL3,
    ABILITY_ID::RESEARCH_BATTLECRUISERWEAPONREFIT
};

inline bool IsTerranUpgradeTypeIndexValid(const size_t UpgradeTypeIndexValue) {
    return UpgradeTypeIndexValue < NUM_TERRAN_UPGRADES;
}

inline bool TryGetTerranUpgradeTypeIndex(const ABILITY_ID UpgradeType, size_t& UpgradeTypeIndexValue) {
    switch (UpgradeType) {
        case ABILITY_ID::RESEARCH_STIMPACK:
            UpgradeTypeIndexValue = 0;
            return true;
        case ABILITY_ID::RESEARCH_COMBATSHIELD:
            UpgradeTypeIndexValue = 1;
            return true;
        case ABILITY_ID::RESEARCH_CONCUSSIVESHELLS:
            UpgradeTypeIndexValue = 2;
            return true;
        case ABILITY_ID::RESEARCH_INFERNALPREIGNITER:
            UpgradeTypeIndexValue = 3;
            return true;
        case ABILITY_ID::RESEARCH_DRILLINGCLAWS:
            UpgradeTypeIndexValue = 4;
            return true;
        case ABILITY_ID::RESEARCH_SMARTSERVOS:
            UpgradeTypeIndexValue = 5;
            return true;
        case ABILITY_ID::RESEARCH_CYCLONERAPIDFIRELAUNCHERS:
            UpgradeTypeIndexValue = 6;
            return true;
        case ABILITY_ID::RESEARCH_LIBERATORAGMODE:
            UpgradeTypeIndexValue = 7;
            return true;
        case ABILITY_ID::RESEARCH_RAVENCORVIDREACTOR:
            UpgradeTypeIndexValue = 8;
            return true;
        case ABILITY_ID::RESEARCH_HISECAUTOTRACKING:
            UpgradeTypeIndexValue = 9;
            return true;
        case ABILITY_ID::RESEARCH_TERRANSTRUCTUREARMORUPGRADE:
            UpgradeTypeIndexValue = 10;
            return true;
        case ABILITY_ID::RESEARCH_TERRANINFANTRYWEAPONSLEVEL1:
            UpgradeTypeIndexValue = 11;
            return true;
        case ABILITY_ID::RESEARCH_TERRANINFANTRYWEAPONSLEVEL2:
            UpgradeTypeIndexValue = 12;
            return true;
        case ABILITY_ID::RESEARCH_TERRANINFANTRYWEAPONSLEVEL3:
            UpgradeTypeIndexValue = 13;
            return true;
        case ABILITY_ID::RESEARCH_TERRANINFANTRYARMORLEVEL1:
            UpgradeTypeIndexValue = 14;
            return true;
        case ABILITY_ID::RESEARCH_TERRANINFANTRYARMORLEVEL2:
            UpgradeTypeIndexValue = 15;
            return true;
        case ABILITY_ID::RESEARCH_TERRANINFANTRYARMORLEVEL3:
            UpgradeTypeIndexValue = 16;
            return true;
        case ABILITY_ID::RESEARCH_TERRANVEHICLEWEAPONSLEVEL1:
            UpgradeTypeIndexValue = 17;
            return true;
        case ABILITY_ID::RESEARCH_TERRANVEHICLEWEAPONSLEVEL2:
            UpgradeTypeIndexValue = 18;
            return true;
        case ABILITY_ID::RESEARCH_TERRANVEHICLEWEAPONSLEVEL3:
            UpgradeTypeIndexValue = 19;
            return true;
        case ABILITY_ID::RESEARCH_TERRANSHIPWEAPONSLEVEL1:
            UpgradeTypeIndexValue = 20;
            return true;
        case ABILITY_ID::RESEARCH_TERRANSHIPWEAPONSLEVEL2:
            UpgradeTypeIndexValue = 21;
            return true;
        case ABILITY_ID::RESEARCH_TERRANSHIPWEAPONSLEVEL3:
            UpgradeTypeIndexValue = 22;
            return true;
        case ABILITY_ID::RESEARCH_TERRANVEHICLEANDSHIPPLATINGLEVEL1:
            UpgradeTypeIndexValue = 23;
            return true;
        case ABILITY_ID::RESEARCH_TERRANVEHICLEANDSHIPPLATINGLEVEL2:
            UpgradeTypeIndexValue = 24;
            return true;
        case ABILITY_ID::RESEARCH_TERRANVEHICLEANDSHIPPLATINGLEVEL3:
            UpgradeTypeIndexValue = 25;
            return true;
        case ABILITY_ID::RESEARCH_BATTLECRUISERWEAPONREFIT:
            UpgradeTypeIndexValue = 26;
            return true;
        default:
            UpgradeTypeIndexValue = INVALID_TERRAN_UPGRADE_TYPE_INDEX;
            return false;
    }
}

inline size_t GetTerranUpgradeTypeIndex(const ABILITY_ID UpgradeType) {
    size_t UpgradeTypeIndexValue = INVALID_TERRAN_UPGRADE_TYPE_INDEX;
    const bool FoundValue = TryGetTerranUpgradeTypeIndex(UpgradeType, UpgradeTypeIndexValue);
    (void)FoundValue;
    return UpgradeTypeIndexValue;
}

}  // namespace sc2
