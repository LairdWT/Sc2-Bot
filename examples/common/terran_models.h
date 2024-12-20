#pragma once

namespace sc2 {

static constexpr size_t NUM_TERRAN_UNITS = 25;
static constexpr size_t NUM_TERRAN_BUILDINGS = 30;
static constexpr size_t NUM_TERRAN_UPGRADES = 27;

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

uint8_t GetTerranUnitTypeIndex(const UNIT_TYPEID UnitType) {
    switch (UnitType) {
        case UNIT_TYPEID::TERRAN_MARINE:
            return 0;
        case UNIT_TYPEID::TERRAN_MARAUDER:
            return 1;
        case UNIT_TYPEID::TERRAN_MEDIVAC:
            return 2;
        case UNIT_TYPEID::TERRAN_WIDOWMINE:
            return 3;
        case UNIT_TYPEID::TERRAN_WIDOWMINEBURROWED:
            return 4;
        case UNIT_TYPEID::TERRAN_AUTOTURRET:
            return 5;
        case UNIT_TYPEID::TERRAN_BANSHEE:
            return 6;
        case UNIT_TYPEID::TERRAN_BATTLECRUISER:
            return 7;
        case UNIT_TYPEID::TERRAN_CYCLONE:
            return 8;
        case UNIT_TYPEID::TERRAN_GHOST:
            return 9;
        case UNIT_TYPEID::TERRAN_HELLION:
            return 10;
        case UNIT_TYPEID::TERRAN_HELLIONTANK:
            return 11;
        case UNIT_TYPEID::TERRAN_LIBERATOR:
            return 12;
        case UNIT_TYPEID::TERRAN_LIBERATORAG:
            return 13;
        case UNIT_TYPEID::TERRAN_MULE:
            return 14;
        case UNIT_TYPEID::TERRAN_NUKE:
            return 15;
        case UNIT_TYPEID::TERRAN_RAVEN:
            return 16;
        case UNIT_TYPEID::TERRAN_REAPER:
            return 17;
        case UNIT_TYPEID::TERRAN_SCV:
            return 18;
        case UNIT_TYPEID::TERRAN_SIEGETANK:
            return 19;
        case UNIT_TYPEID::TERRAN_SIEGETANKSIEGED:
            return 20;
        case UNIT_TYPEID::TERRAN_THOR:
            return 21;
        case UNIT_TYPEID::TERRAN_THORAP:
            return 22;
        case UNIT_TYPEID::TERRAN_VIKINGASSAULT:
            return 23;
        case UNIT_TYPEID::TERRAN_VIKINGFIGHTER:
            return 24;
        default: return 255;  // Invalid index
    }
}

bool IsTerranUnit(const UNIT_TYPEID UnitType) {
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

bool IsTrainTerranUnit(const ABILITY_ID Order) {
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

UNIT_TYPEID TerranUnitTrainToUnitType(const ABILITY_ID Order) {
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

uint8_t GetTerranBuildingTypeIndex(const UNIT_TYPEID BuildingType) {
    switch (BuildingType) {
        case UNIT_TYPEID::TERRAN_ARMORY:
            return 0;
        case UNIT_TYPEID::TERRAN_BARRACKS:
            return 1;
        case UNIT_TYPEID::TERRAN_BARRACKSFLYING:
            return 2;
        case UNIT_TYPEID::TERRAN_BARRACKSREACTOR:
            return 3;
        case UNIT_TYPEID::TERRAN_BARRACKSTECHLAB:
            return 4;
        case UNIT_TYPEID::TERRAN_BUNKER:
            return 5;
        case UNIT_TYPEID::TERRAN_COMMANDCENTER:
            return 6;
        case UNIT_TYPEID::TERRAN_COMMANDCENTERFLYING:
            return 7;
        case UNIT_TYPEID::TERRAN_ORBITALCOMMAND:
            return 8;
        case UNIT_TYPEID::TERRAN_ORBITALCOMMANDFLYING:
            return 9;
        case UNIT_TYPEID::TERRAN_PLANETARYFORTRESS:
            return 10;
        case UNIT_TYPEID::TERRAN_ENGINEERINGBAY:
            return 11;
        case UNIT_TYPEID::TERRAN_FACTORY:
            return 12;
        case UNIT_TYPEID::TERRAN_FACTORYFLYING:
            return 13;
        case UNIT_TYPEID::TERRAN_FACTORYREACTOR:
            return 14;
        case UNIT_TYPEID::TERRAN_FACTORYTECHLAB:
            return 15;
        case UNIT_TYPEID::TERRAN_FUSIONCORE:
            return 16;
        case UNIT_TYPEID::TERRAN_GHOSTACADEMY:
            return 17;
        case UNIT_TYPEID::TERRAN_MISSILETURRET:
            return 18;
        case UNIT_TYPEID::TERRAN_REACTOR:
            return 19;
        case UNIT_TYPEID::TERRAN_REFINERY:
            return 20;
        case UNIT_TYPEID::TERRAN_REFINERYRICH:
            return 21;
        case UNIT_TYPEID::TERRAN_SENSORTOWER:
            return 22;
        case UNIT_TYPEID::TERRAN_STARPORT:
            return 23;
        case UNIT_TYPEID::TERRAN_STARPORTFLYING:
            return 24;
        case UNIT_TYPEID::TERRAN_STARPORTREACTOR:
            return 25;
        case UNIT_TYPEID::TERRAN_STARPORTTECHLAB:
            return 26;
        case UNIT_TYPEID::TERRAN_SUPPLYDEPOT:
            return 27;
        case UNIT_TYPEID::TERRAN_SUPPLYDEPOTLOWERED:
            return 28;
        case UNIT_TYPEID::TERRAN_TECHLAB:
            return 29;
        default: return 255;
    }
}

bool IsTerranBuilding(const UNIT_TYPEID BuildingType) {
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

uint8_t GetTerranUpgradeTypeIndex(const ABILITY_ID UpgradeType) {
    switch (UpgradeType) {
        case ABILITY_ID::RESEARCH_STIMPACK: return 0;
        case ABILITY_ID::RESEARCH_COMBATSHIELD: return 1;
        case ABILITY_ID::RESEARCH_CONCUSSIVESHELLS: return 2;
        case ABILITY_ID::RESEARCH_INFERNALPREIGNITER: return 3;
        case ABILITY_ID::RESEARCH_DRILLINGCLAWS: return 4;
        case ABILITY_ID::RESEARCH_SMARTSERVOS: return 5;
        case ABILITY_ID::RESEARCH_CYCLONERAPIDFIRELAUNCHERS: return 6;
        case ABILITY_ID::RESEARCH_LIBERATORAGMODE: return 7;
        case ABILITY_ID::RESEARCH_RAVENCORVIDREACTOR: return 8;
        case ABILITY_ID::RESEARCH_HISECAUTOTRACKING: return 9;
        case ABILITY_ID::RESEARCH_TERRANSTRUCTUREARMORUPGRADE: return 10;
        case ABILITY_ID::RESEARCH_TERRANINFANTRYWEAPONSLEVEL1: return 11;
        case ABILITY_ID::RESEARCH_TERRANINFANTRYWEAPONSLEVEL2: return 12;
        case ABILITY_ID::RESEARCH_TERRANINFANTRYWEAPONSLEVEL3: return 13;
        case ABILITY_ID::RESEARCH_TERRANINFANTRYARMORLEVEL1: return 14;
        case ABILITY_ID::RESEARCH_TERRANINFANTRYARMORLEVEL2: return 15;
        case ABILITY_ID::RESEARCH_TERRANINFANTRYARMORLEVEL3: return 16;
        case ABILITY_ID::RESEARCH_TERRANVEHICLEWEAPONSLEVEL1: return 17;
        case ABILITY_ID::RESEARCH_TERRANVEHICLEWEAPONSLEVEL2: return 18;
        case ABILITY_ID::RESEARCH_TERRANVEHICLEWEAPONSLEVEL3: return 19;
        case ABILITY_ID::RESEARCH_TERRANSHIPWEAPONSLEVEL1: return 20;
        case ABILITY_ID::RESEARCH_TERRANSHIPWEAPONSLEVEL2: return 21;
        case ABILITY_ID::RESEARCH_TERRANSHIPWEAPONSLEVEL3: return 22;
        case ABILITY_ID::RESEARCH_TERRANVEHICLEANDSHIPPLATINGLEVEL1: return 23;
        case ABILITY_ID::RESEARCH_TERRANVEHICLEANDSHIPPLATINGLEVEL2: return 24;
        case ABILITY_ID::RESEARCH_TERRANVEHICLEANDSHIPPLATINGLEVEL3: return 25;
        case ABILITY_ID::RESEARCH_BATTLECRUISERWEAPONREFIT: return 26;
        default: return 255;
    }
}

}  // namespace sc2
