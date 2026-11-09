#ifdef DEBUG_MODE

#include "CombatLogPanel.h"

#include "../DebugWidgets.h"
#include "../../core/stages/Stage.h"

namespace
{
    // Multipliers other than x1 are highlighted, so the ones that changed a hit stand out
    void multiplier_cell(float multiplier)
    {
        if (multiplier == 1.f) ImGui::TextDisabled("x1");
        else if (multiplier > 1.f) ImGui::TextColored(ImVec4(0.5f, 1.f, 0.6f, 1.f), "x%.2f", multiplier);
        else ImGui::TextColored(ImVec4(1.f, 0.6f, 0.4f, 1.f), "x%.2f", multiplier);
    }
}

void CombatLogPanel::draw(DebugContext& context)
{
    if (!context.stage->is_loaded())
    {
        ImGui::TextDisabled("Load a stage to see hits");
        return;
    }

    CombatLog& combatLog = context.stage->get_combat_log();

    ImGui::BeginDisabled(context.selectedEntityId < 0);
    ImGui::Checkbox("Only the selected entity", &m_isOnlyShowingSelected);
    ImGui::EndDisabled();
    ImGui::SameLine();
    if (ImGui::Button("Clear log")) combatLog.clear();
    DebugWidgets::help_marker("Damage = base x level/stage x strong against x resistant x weaken.");

    const ImGuiTableFlags tableFlags = ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_ScrollY
        | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_Resizable;
    if (ImGui::BeginTable("CombatLogTable", 9, tableFlags))
    {
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn("Time");
        ImGui::TableSetupColumn("Hit", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Base");
        ImGui::TableSetupColumn("Lvl/Stage");
        ImGui::TableSetupColumn("Strong");
        ImGui::TableSetupColumn("Resist");
        ImGui::TableSetupColumn("Weaken");
        ImGui::TableSetupColumn("Damage");
        ImGui::TableSetupColumn("HP left");
        ImGui::TableHeadersRow();

        const auto& entries = combatLog.get_entries();
        for (auto it = entries.rbegin(); it != entries.rend(); ++it)
        {
            const bool isSelectedInvolved = it->attackerId == context.selectedEntityId || it->targetId == context.selectedEntityId;
            if (m_isOnlyShowingSelected && context.selectedEntityId >= 0 && !isSelectedInvolved) continue;

            const DamageResult& result = it->result;
            ImGui::TableNextRow();

            ImGui::TableNextColumn(); ImGui::Text("%.1fs", it->time);
            ImGui::TableNextColumn(); ImGui::Text("%s #%d -> %s #%d", it->attacker.c_str(), it->attackerId, it->target.c_str(), it->targetId);
            ImGui::TableNextColumn(); ImGui::Text("%.0f", result.baseDamage);
            ImGui::TableNextColumn(); multiplier_cell(result.attackMultiplier);
            ImGui::TableNextColumn(); multiplier_cell(result.strongAgainstMultiplier);
            ImGui::TableNextColumn(); multiplier_cell(result.resistanceMultiplier);
            ImGui::TableNextColumn(); multiplier_cell(result.weakenMultiplier);
            ImGui::TableNextColumn(); ImGui::Text("%.1f", result.finalDamage);
            ImGui::TableNextColumn(); ImGui::Text("%.0f", it->targetHealthAfter);
        }
        ImGui::EndTable();
    }
}

#endif
