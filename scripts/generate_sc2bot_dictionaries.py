import argparse
import json
from pathlib import Path
from typing import Any

import yaml


GOAL_DOMAIN_NAMES = {
    "Economy",
    "Army",
    "Production",
    "Technology",
    "Defense",
    "Scouting",
}

GOAL_HORIZON_NAMES = {
    "Immediate",
    "NearTerm",
    "Strategic",
}

GOAL_TYPE_NAMES = {
    "HoldOwnedBase",
    "SaturateWorkers",
    "MaintainSupply",
    "ExpandBaseCount",
    "BuildProductionCapacity",
    "UnlockTechnology",
    "ResearchUpgrade",
    "ProduceArmy",
    "PressureEnemy",
    "ClearEnemyPresence",
    "ScoutExpansionLocations",
}

GOAL_ACTIVATION_RULE_NAMES = {
    "AlwaysActive",
    "ProjectedWorkersBelowTarget",
    "SupplyPressureOrProjectedDepotsBelowTarget",
    "ProjectedCommandCentersBelowTarget",
    "ProjectedRefineriesBelowTarget",
    "ProjectedBarracksBelowTarget",
    "ProjectedFactoryBelowTarget",
    "ProjectedStarportBelowTarget",
    "MissingBarracksReactor",
    "MissingFactoryTechLab",
    "MissingEngineeringBayForUpgrades",
    "MissingStarportReactor",
    "MissingStimpack",
    "MissingCombatShield",
    "MissingInfantryWeaponsLevel1",
    "MissingConcussiveShells",
    "ProjectedMarinesBelowTarget",
    "ProjectedMaraudersBelowTarget",
    "ProjectedCyclonesBelowTarget",
    "ProjectedSiegeTanksBelowTarget",
    "ProjectedMedivacsBelowTarget",
    "ProjectedLiberatorsBelowTarget",
}

GOAL_TARGET_RULE_NAMES = {
    "None",
    "DefaultTargetCount",
    "DesiredWorkerCount",
    "DesiredSupplyDepotCount",
    "DesiredBaseCount",
    "DesiredRefineryCount",
    "DesiredBarracksCount",
    "DesiredFactoryCount",
    "DesiredStarportCount",
    "DesiredMarineCount",
    "DesiredMarauderCount",
    "DesiredCycloneCount",
    "DesiredSiegeTankCount",
    "DesiredMedivacCount",
    "DesiredLiberatorCount",
}

PACKAGE_KIND_NAMES = {
    "Unknown",
    "Opening",
    "Expansion",
    "TechTransition",
    "TimingAttack",
    "Recovery",
    "Macro",
    "Supply",
    "Defense",
    "ProductionScale",
}

NEED_KIND_NAMES = {
    "Unknown",
    "Structure",
    "AddOn",
    "Expansion",
    "Unit",
    "Upgrade",
}

ACTION_KIND_NAMES = {
    "Unknown",
    "BuildStructure",
    "BuildAddon",
    "Expand",
    "MorphStructure",
    "TrainUnit",
    "ResearchUpgrade",
}

COMPLETION_KIND_NAMES = {
    "Unknown",
    "CountAtLeast",
}

TASK_TYPE_NAMES = {
    "Unknown",
    "Recovery",
    "WorkerProduction",
    "Supply",
    "Expansion",
    "Refinery",
    "ProductionStructure",
    "TechStructure",
    "AddOn",
    "UnitProduction",
    "UpgradeResearch",
    "StaticDefense",
    "ArmyMission",
}

TASK_ORIGIN_NAMES = {
    "Opening",
    "GoalMacro",
    "Recovery",
}

COMMITMENT_CLASS_NAMES = {
    "MandatoryOpening",
    "MandatoryRecovery",
    "FlexibleMacro",
    "Opportunistic",
}

EXECUTION_GUARANTEE_NAMES = {
    "MustExecute",
    "Preferred",
    "Skippable",
}

RETENTION_POLICY_NAMES = {
    "HotMustRun",
    "BufferedRetry",
    "DiscardableDuplicate",
}

BLOCKED_WAKE_KIND_NAMES = {
    "ProducerAvailability",
    "Resources",
    "Placement",
    "GoalRevision",
    "CooldownOnly",
}

PLACEMENT_SLOT_TYPE_NAMES = {
    "Unknown",
    "MainRampDepotLeft",
    "MainRampBarracksWithAddon",
    "MainRampDepotRight",
    "MainBarracksWithAddon",
    "MainFactoryWithAddon",
    "MainStarportWithAddon",
    "MainProductionWithAddon",
}

ABILITY_ID_NAMES = {
    "INVALID",
    "BUILD_SUPPLYDEPOT",
    "BUILD_BARRACKS",
    "BUILD_REFINERY",
    "TRAIN_MARINE",
    "MORPH_ORBITALCOMMAND",
    "BUILD_COMMANDCENTER",
    "BUILD_REACTOR_BARRACKS",
    "BUILD_FACTORY",
    "BUILD_STARPORT",
    "TRAIN_HELLION",
    "BUILD_TECHLAB_FACTORY",
    "TRAIN_MEDIVAC",
    "TRAIN_CYCLONE",
    "TRAIN_LIBERATOR",
    "TRAIN_SIEGETANK",
    "BUILD_ENGINEERINGBAY",
    "RESEARCH_STIMPACK",
    "BUILD_REACTOR_STARPORT",
    "TRAIN_MARAUDER",
    "BUILD_REACTOR_FACTORY",
    "RESEARCH_COMBATSHIELD",
    "RESEARCH_TERRANINFANTRYWEAPONSLEVEL1",
    "TRAIN_WIDOWMINE",
    "RESEARCH_CONCUSSIVESHELLS",
    "BUILD_BUNKER",
    "BUILD_TECHLAB_BARRACKS",
    "TRAIN_SCV",
    "TRAIN_VIKINGFIGHTER",
}

UNIT_TYPE_ID_NAMES = {
    "INVALID",
    "TERRAN_SCV",
    "TERRAN_SUPPLYDEPOT",
    "TERRAN_BARRACKS",
    "TERRAN_REFINERY",
    "TERRAN_MARINE",
    "TERRAN_COMMANDCENTER",
    "TERRAN_BARRACKSREACTOR",
    "TERRAN_FACTORY",
    "TERRAN_STARPORT",
    "TERRAN_HELLION",
    "TERRAN_FACTORYTECHLAB",
    "TERRAN_MEDIVAC",
    "TERRAN_CYCLONE",
    "TERRAN_LIBERATOR",
    "TERRAN_SIEGETANK",
    "TERRAN_ENGINEERINGBAY",
    "TERRAN_STARPORTREACTOR",
    "TERRAN_MARAUDER",
    "TERRAN_FACTORYREACTOR",
    "TERRAN_WIDOWMINE",
    "TERRAN_BUNKER",
    "TERRAN_BARRACKSTECHLAB",
    "TERRAN_ORBITALCOMMAND",
    "TERRAN_VIKINGFIGHTER",
}

UPGRADE_ID_NAMES = {
    "INVALID",
    "STIMPACK",
    "SHIELDWALL",
    "TERRANINFANTRYWEAPONSLEVEL1",
    "PUNISHERGRENADES",
}


def read_yaml_file(file_path: Path) -> dict[str, Any]:
    with file_path.open("r", encoding="utf-8") as file_handle:
        parsed_value = yaml.safe_load(file_handle)
    if not isinstance(parsed_value, dict):
        raise ValueError(f"Expected mapping at top level in {file_path}")
    return parsed_value


def validate_name(value: str, valid_names: set[str], field_name: str, entry_label: str) -> None:
    if value not in valid_names:
        raise ValueError(f"{entry_label}: unknown {field_name} '{value}'")


def validate_goal_dictionary(goal_entries: list[dict[str, Any]], task_template_ids: set[str]) -> None:
    seen_definition_ids: set[str] = set()
    seen_goal_ids: set[int] = set()
    for goal_entry in goal_entries:
        entry_label = f"Goal '{goal_entry.get('DefinitionId', '<missing>')}'"
        definition_id = str(goal_entry["DefinitionId"])
        goal_id = int(goal_entry["GoalId"])
        if definition_id in seen_definition_ids:
            raise ValueError(f"{entry_label}: duplicate DefinitionId '{definition_id}'")
        if goal_id in seen_goal_ids:
            raise ValueError(f"{entry_label}: duplicate GoalId '{goal_id}'")
        seen_definition_ids.add(definition_id)
        seen_goal_ids.add(goal_id)
        validate_name(str(goal_entry["GoalDomain"]), GOAL_DOMAIN_NAMES, "GoalDomain", entry_label)
        validate_name(str(goal_entry["GoalHorizon"]), GOAL_HORIZON_NAMES, "GoalHorizon", entry_label)
        validate_name(str(goal_entry["GoalType"]), GOAL_TYPE_NAMES, "GoalType", entry_label)
        validate_name(str(goal_entry["ActivationRuleId"]), GOAL_ACTIVATION_RULE_NAMES, "ActivationRuleId",
                      entry_label)
        validate_name(str(goal_entry["TargetRuleId"]), GOAL_TARGET_RULE_NAMES, "TargetRuleId", entry_label)
        validate_name(str(goal_entry["DefaultTargetUnitTypeId"]), UNIT_TYPE_ID_NAMES, "DefaultTargetUnitTypeId",
                      entry_label)
        validate_name(str(goal_entry["DefaultTargetUpgradeId"]), UPGRADE_ID_NAMES, "DefaultTargetUpgradeId",
                      entry_label)
        task_template_id = str(goal_entry.get("TaskTemplateId", "Invalid"))
        if task_template_id != "Invalid" and task_template_id not in task_template_ids:
            raise ValueError(f"{entry_label}: unknown TaskTemplateId '{task_template_id}'")


def validate_task_dictionary(task_entries: list[dict[str, Any]]) -> None:
    seen_template_ids: set[str] = set()
    for task_entry in task_entries:
        entry_label = f"TaskTemplate '{task_entry.get('TemplateId', '<missing>')}'"
        template_id = str(task_entry["TemplateId"])
        if template_id in seen_template_ids:
            raise ValueError(f"{entry_label}: duplicate TemplateId '{template_id}'")
        seen_template_ids.add(template_id)
        validate_name(str(task_entry["PackageKind"]), PACKAGE_KIND_NAMES, "PackageKind", entry_label)
        validate_name(str(task_entry["NeedKind"]), NEED_KIND_NAMES, "NeedKind", entry_label)
        validate_name(str(task_entry["ActionKind"]), ACTION_KIND_NAMES, "ActionKind", entry_label)
        validate_name(str(task_entry["CompletionKind"]), COMPLETION_KIND_NAMES, "CompletionKind", entry_label)
        validate_name(str(task_entry["TaskType"]), TASK_TYPE_NAMES, "TaskType", entry_label)
        validate_name(str(task_entry["Origin"]), TASK_ORIGIN_NAMES, "Origin", entry_label)
        validate_name(str(task_entry["CommitmentClass"]), COMMITMENT_CLASS_NAMES, "CommitmentClass", entry_label)
        validate_name(str(task_entry["ExecutionGuarantee"]), EXECUTION_GUARANTEE_NAMES, "ExecutionGuarantee",
                      entry_label)
        validate_name(str(task_entry["RetentionPolicy"]), RETENTION_POLICY_NAMES, "RetentionPolicy", entry_label)
        validate_name(str(task_entry["BlockedTaskWakeKind"]), BLOCKED_WAKE_KIND_NAMES, "BlockedTaskWakeKind",
                      entry_label)
        validate_name(str(task_entry["ActionAbilityId"]), ABILITY_ID_NAMES, "ActionAbilityId", entry_label)
        validate_name(str(task_entry["ActionProducerUnitTypeId"]), UNIT_TYPE_ID_NAMES,
                      "ActionProducerUnitTypeId", entry_label)
        validate_name(str(task_entry["ActionResultUnitTypeId"]), UNIT_TYPE_ID_NAMES, "ActionResultUnitTypeId",
                      entry_label)
        validate_name(str(task_entry["ActionUpgradeId"]), UPGRADE_ID_NAMES, "ActionUpgradeId", entry_label)
        validate_name(str(task_entry["DefaultPreferredPlacementSlotType"]), PLACEMENT_SLOT_TYPE_NAMES,
                      "DefaultPreferredPlacementSlotType", entry_label)


def to_cpp_ability_id(ability_name: str) -> str:
    return f"ABILITY_ID::{ability_name}"


def to_cpp_unit_type_id(unit_type_name: str) -> str:
    return f"UNIT_TYPEID::{unit_type_name}"


def to_cpp_upgrade_id(upgrade_name: str) -> str:
    return f"UpgradeID(UPGRADE_ID::{upgrade_name})"


def escape_cpp_string(raw_value: str) -> str:
    return raw_value.replace("\\", "\\\\").replace("\"", "\\\"")


def render_goal_enum_header(goal_entries: list[dict[str, Any]]) -> str:
    enumerator_lines = ["    Invalid = 0,"]
    for goal_entry in goal_entries:
        enumerator_lines.append(f"    {goal_entry['DefinitionId']},")
    enumerator_lines.append("    Count")
    body = "\n".join(enumerator_lines)
    return (
        "#pragma once\n\n"
        "#include <cstdint>\n\n"
        "namespace sc2\n"
        "{\n\n"
        "enum class ETerranGoalDefinitionId : uint16_t\n"
        "{\n"
        f"{body}\n"
        "};\n\n"
        "}  // namespace sc2\n"
    )


def render_task_enum_header(task_entries: list[dict[str, Any]]) -> str:
    enumerator_lines = ["    Invalid = 0,"]
    for task_entry in task_entries:
        enumerator_lines.append(f"    {task_entry['TemplateId']},")
    enumerator_lines.append("    Count")
    body = "\n".join(enumerator_lines)
    return (
        "#pragma once\n\n"
        "#include <cstdint>\n\n"
        "namespace sc2\n"
        "{\n\n"
        "enum class ETerranTaskTemplateId : uint16_t\n"
        "{\n"
        f"{body}\n"
        "};\n\n"
        "}  // namespace sc2\n"
    )


def render_goal_data_header(goal_entries: list[dict[str, Any]]) -> str:
    return (
        "#pragma once\n\n"
        "#include <array>\n\n"
        "#include <cstddef>\n\n"
        "#include \"common/catalogs/FTerranGoalDefinition.h\"\n\n"
        "namespace sc2\n"
        "{\n\n"
        f"constexpr size_t TerranGoalDefinitionCountValue = {len(goal_entries)}U;\n\n"
        "extern const std::array<FTerranGoalDefinition, TerranGoalDefinitionCountValue> GTerranGoalDefinitions;\n\n"
        "}  // namespace sc2\n"
    )


def render_task_data_header(task_entries: list[dict[str, Any]]) -> str:
    return (
        "#pragma once\n\n"
        "#include <array>\n\n"
        "#include <cstddef>\n\n"
        "#include \"common/catalogs/FTerranTaskTemplateDefinition.h\"\n\n"
        "namespace sc2\n"
        "{\n\n"
        f"constexpr size_t TerranTaskTemplateDefinitionCountValue = {len(task_entries)}U;\n\n"
        "extern const std::array<FTerranTaskTemplateDefinition, TerranTaskTemplateDefinitionCountValue> "
        "GTerranTaskTemplateDefinitions;\n\n"
        "}  // namespace sc2\n"
    )


def render_goal_data_source(goal_entries: list[dict[str, Any]]) -> str:
    entry_lines: list[str] = []
    for goal_entry in goal_entries:
        entry_lines.append(
            "    FTerranGoalDefinition{"
            f"ETerranGoalDefinitionId::{goal_entry['DefinitionId']}, "
            f"{int(goal_entry['GoalId'])}U, "
            f"\"{escape_cpp_string(str(goal_entry['DisplayName']))}\", "
            f"EGoalDomain::{goal_entry['GoalDomain']}, "
            f"EGoalHorizon::{goal_entry['GoalHorizon']}, "
            f"EGoalType::{goal_entry['GoalType']}, "
            f"{int(goal_entry['BasePriorityValue'])}, "
            f"EGoalActivationRuleId::{goal_entry['ActivationRuleId']}, "
            f"EGoalTargetRuleId::{goal_entry['TargetRuleId']}, "
            f"{int(goal_entry['DefaultTargetCount'])}U, "
            f"{to_cpp_unit_type_id(str(goal_entry['DefaultTargetUnitTypeId']))}, "
            f"{to_cpp_upgrade_id(str(goal_entry['DefaultTargetUpgradeId']))}, "
            f"ETerranTaskTemplateId::{goal_entry.get('TaskTemplateId', 'Invalid')}}},"
        )
    joined_entries = "\n".join(entry_lines)
    return (
        "#include \"common/catalogs/generated/FTerranGoalDictionaryData.generated.h\"\n\n"
        "namespace sc2\n"
        "{\n\n"
        "const std::array<FTerranGoalDefinition, TerranGoalDefinitionCountValue> GTerranGoalDefinitions = {\n"
        f"{joined_entries}\n"
        "};\n\n"
        "}  // namespace sc2\n"
    )


def render_task_data_source(task_entries: list[dict[str, Any]]) -> str:
    entry_lines: list[str] = []
    for task_entry in task_entries:
        entry_lines.append(
            "    FTerranTaskTemplateDefinition{"
            f"ETerranTaskTemplateId::{task_entry['TemplateId']}, "
            f"\"{escape_cpp_string(str(task_entry['DisplayName']))}\", "
            f"ECommandTaskPackageKind::{task_entry['PackageKind']}, "
            f"ECommandTaskNeedKind::{task_entry['NeedKind']}, "
            f"ECommandTaskActionKind::{task_entry['ActionKind']}, "
            f"ECommandTaskCompletionKind::{task_entry['CompletionKind']}, "
            f"ECommandTaskType::{task_entry['TaskType']}, "
            f"ECommandTaskOrigin::{task_entry['Origin']}, "
            f"ECommandCommitmentClass::{task_entry['CommitmentClass']}, "
            f"ECommandTaskExecutionGuarantee::{task_entry['ExecutionGuarantee']}, "
            f"ECommandTaskRetentionPolicy::{task_entry['RetentionPolicy']}, "
            f"EBlockedTaskWakeKind::{task_entry['BlockedTaskWakeKind']}, "
            f"{to_cpp_ability_id(str(task_entry['ActionAbilityId']))}, "
            f"{to_cpp_unit_type_id(str(task_entry['ActionProducerUnitTypeId']))}, "
            f"{to_cpp_unit_type_id(str(task_entry['ActionResultUnitTypeId']))}, "
            f"{to_cpp_upgrade_id(str(task_entry['ActionUpgradeId']))}, "
            f"{int(task_entry['DefaultTargetCount'])}U, "
            f"{int(task_entry['DefaultRequestedQueueCount'])}U, "
            f"EBuildPlacementSlotType::{task_entry['DefaultPreferredPlacementSlotType']}}},"
        )
    joined_entries = "\n".join(entry_lines)
    return (
        "#include \"common/catalogs/generated/FTerranTaskTemplateDictionaryData.generated.h\"\n\n"
        "namespace sc2\n"
        "{\n\n"
        "const std::array<FTerranTaskTemplateDefinition, TerranTaskTemplateDefinitionCountValue> "
        "GTerranTaskTemplateDefinitions = {\n"
        f"{joined_entries}\n"
        "};\n\n"
        "}  // namespace sc2\n"
    )


def render_json_catalog(goal_entries: list[dict[str, Any]], task_entries: list[dict[str, Any]]) -> str:
    catalog_value = {
        "Goals": goal_entries,
        "TaskTemplates": task_entries,
    }
    return json.dumps(catalog_value, indent=2, sort_keys=False) + "\n"


def write_or_check_file(file_path: Path, content_value: str, check_only: bool) -> None:
    existing_content_value = file_path.read_text(encoding="utf-8") if file_path.exists() else None
    if check_only:
        if existing_content_value != content_value:
            raise ValueError(f"Generated file is out of date: {file_path}")
        return
    file_path.parent.mkdir(parents=True, exist_ok=True)
    if existing_content_value != content_value:
        file_path.write_text(content_value, encoding="utf-8", newline="\n")


def main() -> int:
    script_root = Path(__file__).resolve().parent.parent
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--authoring-root",
        default=str(script_root / "examples" / "common" / "catalogs" / "terran"),
        help="Directory containing GoalDictionary.yaml and TaskTemplateDictionary.yaml",
    )
    parser.add_argument(
        "--generated-root",
        default=str(script_root / "examples" / "common" / "catalogs" / "generated"),
        help="Directory receiving generated C++ and JSON outputs",
    )
    parser.add_argument("--check", action="store_true", help="Validate generated files without rewriting them")
    arguments = parser.parse_args()

    authoring_root = Path(arguments.authoring_root)
    generated_root = Path(arguments.generated_root)
    goal_dictionary = read_yaml_file(authoring_root / "GoalDictionary.yaml")
    task_dictionary = read_yaml_file(authoring_root / "TaskTemplateDictionary.yaml")
    goal_entries = list(goal_dictionary.get("Goals", []))
    task_entries = list(task_dictionary.get("TaskTemplates", []))
    if not goal_entries:
        raise ValueError("GoalDictionary.yaml does not contain any Goals entries")
    if not task_entries:
        raise ValueError("TaskTemplateDictionary.yaml does not contain any TaskTemplates entries")

    validate_task_dictionary(task_entries)
    task_template_ids = {str(task_entry["TemplateId"]) for task_entry in task_entries}
    validate_goal_dictionary(goal_entries, task_template_ids)

    output_map = {
        generated_root / "ETerranGoalDefinitionId.generated.h": render_goal_enum_header(goal_entries),
        generated_root / "ETerranTaskTemplateId.generated.h": render_task_enum_header(task_entries),
        generated_root / "FTerranGoalDictionaryData.generated.h": render_goal_data_header(goal_entries),
        generated_root / "FTerranGoalDictionaryData.generated.cc": render_goal_data_source(goal_entries),
        generated_root / "FTerranTaskTemplateDictionaryData.generated.h": render_task_data_header(task_entries),
        generated_root / "FTerranTaskTemplateDictionaryData.generated.cc": render_task_data_source(task_entries),
        generated_root / "TerranGoalTaskDictionary.json": render_json_catalog(goal_entries, task_entries),
    }

    for output_path, content_value in output_map.items():
        write_or_check_file(output_path, content_value, arguments.check)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
