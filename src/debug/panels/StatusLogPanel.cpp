#ifdef DEBUG_MODE

#include "StatusLogPanel.h"

#include "../DebugWidgets.h"
#include "../../core/stages/Stage.h"

#include <string>

namespace
{
    // "Fish #3", or "Debug" for statuses given from the Inspector
    std::string log_name(const std::string& name, int id)
    {
        return id < 0 ? name : name + " #" + std::to_string(id);
    }
}

void StatusLogPanel::draw(DebugContext& context)
{
    if (!context.stage->is_loaded())
    {
        ImGui::TextDisabled("Load a stage to see statuses");
        return;
    }

    StatusManager& statusManager = context.stage->get_status_manager();

    // Guaranteed mode makes effects easy to see, normal mode keeps the real chances for balancing
    bool isGuaranteed = statusManager.is_guaranteed_mode();
    if (ImGui::Checkbox("Guaranteed effects (every chance = 100%)", &isGuaranteed))
        statusManager.set_guaranteed_mode(isGuaranteed);

    // ----- Active statuses -----
    ImGui::SeparatorText("Active statuses");

    bool hasAnyStatus = false;
    for (const auto& row : DebugWidgets::collect_entities(*context.stage))
    {
        if (row.entity->get_statuses().empty()) continue;
        hasAnyStatus = true;

        ImGui::TextColored(DebugWidgets::side_color(row.side), "%s #%d (%.0f HP)",
            DebugWidgets::entity_name(row).c_str(), row.entity->uniqueId, row.entity->currentHealth);
        for (const StatusEffect& status : row.entity->get_statuses())
            ImGui::BulletText("%s  x%.2f  %.1f s left", to_string(status.type), status.value, status.durationRemaining);
    }
    if (!hasAnyStatus) ImGui::TextDisabled("No entity has a status");

    // ----- Log -----
    ImGui::SeparatorText("Log (newest first)");
    ImGui::Checkbox("Hide misses", &m_isHidingMisses);
    ImGui::SameLine();
    if (ImGui::Button("Clear log")) statusManager.clear_log();

    ImGui::BeginChild("StatusLog");
    const auto& log = statusManager.get_log();
    for (auto it = log.rbegin(); it != log.rend(); ++it)
    {
        if (m_isHidingMisses && it->outcome == StatusOutcome::MISSED) continue;

        const std::string source = log_name(it->source, it->sourceId);
        const std::string target = log_name(it->target, it->targetId);

        if (it->outcome == StatusOutcome::MISSED)
        {
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.f), "[%.1fs] %s -> %s: %s missed",
                it->time, source.c_str(), target.c_str(), to_string(it->status));
            ImGui::TextDisabled("    roll %.1f vs %.0f%% chance", it->roll, it->chance);
        }
        else if (it->outcome == StatusOutcome::STACKED)
        {
            ImGui::TextColored(ImVec4(1.f, 0.8f, 0.4f, 1.f), "[%.1fs] %s -> %s: %s stacked x%d",
                it->time, source.c_str(), target.c_str(), to_string(it->status), it->stackCount);
            ImGui::TextDisabled("    first applied at %.1fs, now x%.2f with %.1f s left", it->firstTime, it->value, it->duration);
        }
        else
        {
            ImGui::TextColored(ImVec4(0.5f, 1.f, 0.6f, 1.f), "[%.1fs] %s -> %s: %s applied",
                it->time, source.c_str(), target.c_str(), to_string(it->status));
            if (it->guaranteed)
                ImGui::TextDisabled("    guaranteed, x%.2f for %.1f s", it->value, it->duration);
            else
                ImGui::TextDisabled("    roll %.1f vs %.0f%% chance, x%.2f for %.1f s", it->roll, it->chance, it->value, it->duration);
        }
    }
    if (log.empty()) ImGui::TextDisabled("No status requested yet");
    ImGui::EndChild();
}

#endif
