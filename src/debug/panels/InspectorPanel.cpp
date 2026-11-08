#ifdef DEBUG_MODE

#include "InspectorPanel.h"

#include "../../core/stages/Stage.h"

#include <algorithm>
#include <cstdio>
#include <string>

void InspectorPanel::draw(DebugContext& context)
{
    if (!context.stage->is_loaded())
    {
        ImGui::TextDisabled("Load a stage to inspect entities");
        return;
    }

    const std::vector<DebugEntityRow> rows = DebugWidgets::collect_entities(*context.stage);
    const DebugEntityRow* selectedRow = nullptr;
    for (const auto& row : rows) if (row.entity->uniqueId == context.selectedEntityId) selectedRow = &row;

    ImGui::TextDisabled("%d entities on the field. Click one for details.", static_cast<int>(rows.size()));

    const ImGuiTableFlags tableFlags = ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_ScrollY
        | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_Resizable;
    if (ImGui::BeginTable("EntityTable", 5, tableFlags, ImVec2(0.f, 260.f)))
    {
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn("ID");
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("State");
        ImGui::TableSetupColumn("HP");
        ImGui::TableSetupColumn("Dmg/hit");
        ImGui::TableHeadersRow();

        for (const auto& row : rows)
        {
            const BattleEntity& entity = *row.entity;
            ImGui::TableNextRow();

            ImGui::TableNextColumn();
            char idLabel[32];
            std::snprintf(idLabel, sizeof(idLabel), "%d##Entity", entity.uniqueId);
            if (ImGui::Selectable(idLabel, entity.uniqueId == context.selectedEntityId, ImGuiSelectableFlags_SpanAllColumns))
                context.selectedEntityId = entity.uniqueId;

            ImGui::TableNextColumn();
            ImGui::TextColored(DebugWidgets::side_color(row.side), "%s", DebugWidgets::entity_name(row).c_str());

            ImGui::TableNextColumn();
            ImGui::TextUnformatted(entity.data ? entity.stateMachine->get_active_state_id().c_str() : "-");

            ImGui::TableNextColumn();
            ImGui::Text("%.0f/%.0f", entity.currentHealth, DebugWidgets::max_health(entity));

            ImGui::TableNextColumn();
            if (entity.data) ImGui::Text("%.1f", DebugWidgets::damage_per_hit(entity));
            else ImGui::TextUnformatted("-");
        }
        ImGui::EndTable();
    }

    if (selectedRow) draw_details(context, *selectedRow);
    else ImGui::TextDisabled("No entity selected");
}

void InspectorPanel::draw_details(DebugContext& context, const DebugEntityRow& row)
{
    BattleEntity& entity = *row.entity;

    ImGui::SeparatorText((DebugWidgets::entity_name(row) + " #" + std::to_string(entity.uniqueId)).c_str());
    ImGui::TextColored(DebugWidgets::side_color(row.side), "%s", row.side);

    // ----- Health -----
    const float maxHealth = DebugWidgets::max_health(entity);
    char healthText[64];
    std::snprintf(healthText, sizeof(healthText), "%.0f / %.0f HP", entity.currentHealth, maxHealth);
    ImGui::ProgressBar(maxHealth > 0.f ? entity.currentHealth / maxHealth : 0.f, ImVec2(-FLT_MIN, 0.f), healthText);

    // Debug kill goes through take_damage, like every other loss of health
    if (ImGui::Button("Kill"))
    {
        DamageResult kill;
        kill.finalDamage = entity.currentHealth;
        entity.take_damage(kill);
    }

    if (!entity.data) return; // Bases: health only

    const EntityData& data = *entity.data;
    ImGui::SameLine();
    ImGui::Text("UID %d  |  State: %s", data.UID, entity.stateMachine->get_active_state_id().c_str());
    ImGui::Text("Next knockback at %.0f HP (%d knockbacks in total)", entity.healthLeftBeforeNextKnockback, data.knockbackCount);

    // ----- Attack -----
    ImGui::SeparatorText("Attack");
    ImGui::Text("Damage per hit: %.1f", DebugWidgets::damage_per_hit(entity));
    ImGui::TextDisabled("%d attack x%.2f level/stage x%.2f weaken", data.attackPower, entity.magnification.y, DebugWidgets::weaken_multiplier(entity));
    DebugWidgets::help_marker("Strong against and resistant depend on the target: see the Combat Log for real hits.");
    ImGui::Text("%s attack, range %.0f, every %.2f s", data.attackType == 1 ? "Single" : "Area", data.attackRange, data.attackFrequency);

    const float cooldownProgress = data.attackFrequency > 0.f ? std::min(1.f, entity.currentAttackCooldown / data.attackFrequency) : 1.f;
    ImGui::ProgressBar(cooldownProgress, ImVec2(-FLT_MIN, 0.f), cooldownProgress >= 1.f ? "Attack ready" : "Attack cooldown");

    // ----- Movement -----
    ImGui::SeparatorText("Movement");
    ImGui::Text("x = %.0f  |  layer %d  |  speed %.0f px/s", entity.position.x, entity.currentLayer, data.movementSpeed * 10.f);
    ImGui::Text("HP x%.2f  |  attack x%.2f", entity.magnification.x, entity.magnification.y);

    // ----- Types and abilities -----
    ImGui::SeparatorText("Types");
    if (data.types.empty()) ImGui::TextDisabled("None");
    for (size_t i = 0; i < data.types.size(); ++i)
    {
        if (i > 0) ImGui::SameLine();
        ImGui::TextUnformatted(to_string(data.types[i]));
    }

    ImGui::SeparatorText("Abilities");
    if (data.abilities.empty()) ImGui::TextDisabled("None");
    for (const AbilityData& ability : data.abilities)
    {
        std::string targets;
        for (EntityType type : ability.targetTypes) targets += std::string(targets.empty() ? "" : ", ") + to_string(type);

        if (is_on_hit(ability.type))
            ImGui::BulletText("%s -> %s  x%.2f  %.0f%%  %.1f s", to_string(ability.type), targets.c_str(), ability.value, ability.chance, ability.duration);
        else
            ImGui::BulletText("%s -> %s", to_string(ability.type), targets.c_str());
    }

    // ----- Statuses -----
    ImGui::SeparatorText("Statuses");
    if (entity.get_statuses().empty()) ImGui::TextDisabled("None");
    for (const StatusEffect& status : entity.get_statuses())
        ImGui::BulletText("%s  x%.2f  %.1f s left", to_string(status.type), status.value, status.durationRemaining);

    // Give a status directly, without an ability or a chance roll
    if (ImGui::BeginCombo("Status", to_string(m_statusToApply)))
    {
        for (int s = static_cast<int>(StatusType::NONE) + 1; s < static_cast<int>(StatusType::COUNT); ++s)
        {
            StatusType type = static_cast<StatusType>(s);
            if (ImGui::Selectable(to_string(type), type == m_statusToApply)) m_statusToApply = type;
        }
        ImGui::EndCombo();
    }
    if (m_statusToApply != StatusType::PARALYZED)
        ImGui::SliderFloat("Value", &m_statusValue, 0.05f, 2.f, "x%.2f");
    ImGui::SliderFloat("Duration", &m_statusDuration, 0.1f, 30.f, "%.1f s");

    StatusManager& statusManager = context.stage->get_status_manager();
    if (ImGui::Button("Apply status"))
        statusManager.debug_apply(entity, m_statusToApply, m_statusValue, m_statusDuration, context.stage->get_elapsed_time());
    ImGui::SameLine();
    if (ImGui::Button("Clear statuses"))
        statusManager.debug_clear(entity);
}

#endif
