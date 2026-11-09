#pragma once

#ifdef DEBUG_MODE

#include "imgui.h"
#include "../data/AbilityData.h"
#include "../data/EntityType.h"

#include <memory>
#include <string>
#include <vector>

class BattleEntity;
class Stage;

/*
 * DebugEntityRow
 * --------------
 * One entity on the field, with its side ("Unit", "Enemy", "Player base", "Enemy base").
 */
struct DebugEntityRow
{
    std::shared_ptr<BattleEntity> entity;
    const char* side;
};

/*
 * DebugWidgets
 * ------------
 * Helpers shared by several debug panels and the battlefield overlays.
 */
namespace DebugWidgets
{
    // Every entity of the loaded stage (bases, units, enemies), sorted by spawn order
    std::vector<DebugEntityRow> collect_entities(Stage& stage);

    std::string entity_name(const DebugEntityRow& row);   // Bases have no data: uses the side
    ImVec4 side_color(const char* side);
    float max_health(const BattleEntity& entity);
    float weaken_multiplier(const BattleEntity& entity);   // 1 without the WEAKEN status

    // One hit before strong against / resistant (they depend on the target): attack x level/stage x weaken
    float damage_per_hit(const BattleEntity& entity);

    // One checkbox per entity type
    void edit_type_list(std::vector<EntityType>& types);

    // Types and abilities editor used before spawning. Edits copies, never the loaded data
    void edit_types_and_abilities(std::vector<EntityType>& types, std::vector<AbilityData>& abilities,
        const std::vector<EntityType>& defaultTargets);

    // First frame of a sprite sheet, scaled to fit 200x200 (each id keeps its own texture)
    void show_preview(const std::string& id, const std::string& texturePath, int frameCount);

    // Greyed "(?)" that shows `text` when hovered
    void help_marker(const char* text);
}

#endif
