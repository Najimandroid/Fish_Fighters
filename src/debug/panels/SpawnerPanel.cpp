#ifdef DEBUG_MODE

#include "SpawnerPanel.h"

#include "../DebugWidgets.h"
#include "../../core/DataLoader.h"
#include "../../core/stages/Stage.h"

#include <algorithm>
#include <climits>
#include <string>

void SpawnerPanel::draw(DebugContext& context)
{
    if (!context.stage->is_loaded())
    {
        ImGui::TextDisabled("Load a stage to spawn units and enemies");
        return;
    }

    if (ImGui::CollapsingHeader("Unit", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::PushID("Unit");
        draw_unit_spawner(context);
        ImGui::PopID();
    }

    if (ImGui::CollapsingHeader("Enemy", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::PushID("Enemy");
        draw_enemy_spawner(context);
        ImGui::PopID();
    }
}

void SpawnerPanel::draw_unit_spawner(DebugContext& context)
{
    const auto& units = context.dataLoader->get_units_database();
    if (units.empty()) return;

    if (units.find(m_selectedUnit) == units.end())
        m_selectedUnit = units.begin()->first;

    // One entry per unit (named after its base form). Only defined units can be picked
    auto unitLabel = [&units](int uid)
        {
            auto baseForm = units.lower_bound({ uid, INT_MIN });
            return std::to_string(uid) + " - " + baseForm->second->name;
        };

    if (ImGui::BeginCombo("Unit", unitLabel(m_selectedUnit.first).c_str()))
    {
        int previousUid = INT_MIN;
        for (const auto& [key, data] : units)
        {
            if (key.first == previousUid) continue; // Other forms of the same unit
            previousUid = key.first;

            if (ImGui::Selectable(unitLabel(key.first).c_str(), key.first == m_selectedUnit.first))
                m_selectedUnit = units.lower_bound({ key.first, INT_MIN })->first; // Base form
        }
        ImGui::EndCombo();
    }

    // Only the forms this unit has can be picked
    auto formLabel = [](const std::pair<int, int>& key, const EntityData& data)
        {
            return (key.second == 0 ? std::string("Base") : "Evolved " + std::to_string(key.second)) + " - " + data.name;
        };

    auto selected = units.find(m_selectedUnit);
    if (ImGui::BeginCombo("Form", formLabel(selected->first, *selected->second).c_str()))
    {
        for (auto it = units.lower_bound({ m_selectedUnit.first, INT_MIN }); it != units.end() && it->first.first == m_selectedUnit.first; ++it)
        {
            if (ImGui::Selectable(formLabel(it->first, *it->second).c_str(), it->first == m_selectedUnit))
                m_selectedUnit = it->first;
        }
        ImGui::EndCombo();
    }

    const std::shared_ptr<EntityData>& unitData = units.find(m_selectedUnit)->second;

    // New unit picked: start from its JSON types and abilities
    if (m_editedUnit != m_selectedUnit)
    {
        m_unitTypes = unitData->types;
        m_unitAbilities = unitData->abilities;
        m_editedUnit = m_selectedUnit;
    }

    DebugWidgets::show_preview("UnitPreview", unitData->texture, unitData->frameCount);

    DebugWidgets::edit_types_and_abilities(m_unitTypes, m_unitAbilities, {});
    if (ImGui::Button("Reset to JSON data"))
    {
        m_unitTypes = unitData->types;
        m_unitAbilities = unitData->abilities;
    }

    ImGui::Checkbox("Free spawn (ignore cost and units limit)", &m_isFreeUnitSpawn);

    if (ImGui::Button("Spawn unit", ImVec2(-FLT_MIN, 0.f)))
    {
        auto customData = std::make_shared<EntityData>(*unitData);
        customData->types = m_unitTypes;
        customData->abilities = m_unitAbilities;
        context.stage->spawn_unit(customData, m_isFreeUnitSpawn);
    }
}

void SpawnerPanel::draw_enemy_spawner(DebugContext& context)
{
    // Sorted by UID so the list doesn't change order
    std::vector<std::shared_ptr<EntityData>> enemies;
    for (const auto& [uid, data] : context.dataLoader->get_enemies_database()) enemies.push_back(data);
    if (enemies.empty()) return;
    std::sort(enemies.begin(), enemies.end(), [](const auto& a, const auto& b) { return a->UID < b->UID; });

    auto findEnemy = [&enemies](int uid) -> std::shared_ptr<EntityData>
        {
            for (const auto& data : enemies) if (data->UID == uid) return data;
            return nullptr;
        };

    if (!findEnemy(m_selectedEnemy)) m_selectedEnemy = enemies.front()->UID;

    auto enemyLabel = [](const EntityData& data) { return std::to_string(data.UID) + " - " + data.name; };

    // Only defined enemies can be picked
    if (ImGui::BeginCombo("Enemy", enemyLabel(*findEnemy(m_selectedEnemy)).c_str()))
    {
        for (const auto& data : enemies)
        {
            if (ImGui::Selectable(enemyLabel(*data).c_str(), data->UID == m_selectedEnemy))
                m_selectedEnemy = data->UID;
        }
        ImGui::EndCombo();
    }

    auto enemyData = findEnemy(m_selectedEnemy);

    // New enemy picked: start from its JSON types and abilities
    if (m_editedEnemy != m_selectedEnemy)
    {
        m_enemyTypes = enemyData->types;
        m_enemyAbilities = enemyData->abilities;
        m_editedEnemy = m_selectedEnemy;
    }

    DebugWidgets::show_preview("EnemyPreview", enemyData->texture, enemyData->frameCount);

    // Enemy abilities target every fish by default
    DebugWidgets::edit_types_and_abilities(m_enemyTypes, m_enemyAbilities, { EntityType::FISH });
    if (ImGui::Button("Reset to JSON data"))
    {
        m_enemyTypes = enemyData->types;
        m_enemyAbilities = enemyData->abilities;
    }

    ImGui::SeparatorText("Spawn settings");
    ImGui::SliderFloat("HP multiplier", &m_hpMultiplier, 1.f, 100.f);
    ImGui::SliderFloat("Attack multiplier", &m_attackMultiplier, 1.f, 100.f);
    ImGui::SliderInt("Layer", &m_enemyLayer, -1, 50);
    DebugWidgets::help_marker("-1 picks a random layer.");
    ImGui::Checkbox("Boss (shockwave)", &m_isBoss);
    ImGui::SameLine();
    ImGui::Checkbox("Bypass enemy limit", &m_isBypassingEnemyLimit);

    if (ImGui::Button("Spawn enemy", ImVec2(-FLT_MIN, 0.f)))
    {
        auto customData = std::make_shared<EntityData>(*enemyData);
        customData->types = m_enemyTypes;
        customData->abilities = m_enemyAbilities;

        context.stage->spawn_enemy(customData, sf::Vector2f(m_hpMultiplier, m_attackMultiplier), m_enemyLayer, m_isBoss, m_isBypassingEnemyLimit);
    }
}

#endif
