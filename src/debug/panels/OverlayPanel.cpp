#ifdef DEBUG_MODE

#include "OverlayPanel.h"

#include "../DebugWidgets.h"

void OverlayPanel::draw(DebugContext& context)
{
    DebugOverlaySettings& overlays = context.overlays;

    ImGui::TextDisabled("Drawn on the battlefield, in the game window");

    ImGui::Checkbox("Labels", &overlays.showLabels);
    DebugWidgets::help_marker("Name #id, health, state and statuses above every entity.");

    ImGui::Checkbox("Selection outline", &overlays.showSelection);
    DebugWidgets::help_marker("Yellow outline around the entity selected in the Inspector.");

    ImGui::SeparatorText("Collision zones");
    ImGui::ColorButton("##HitboxColor", ImVec4(0.35f, 0.55f, 1.f, 1.f), ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoPicker, ImVec2(14.f, 14.f));
    ImGui::SameLine();
    ImGui::Checkbox("Hitboxes", &overlays.showHitboxes);

    ImGui::ColorButton("##RangeColor", ImVec4(1.f, 0.9f, 0.3f, 1.f), ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoPicker, ImVec2(14.f, 14.f));
    ImGui::SameLine();
    ImGui::Checkbox("Attack ranges", &overlays.showAttackRanges);
    DebugWidgets::help_marker("An entity starts attacking when an opponent's hitbox touches this zone.");

    ImGui::ColorButton("##DamageColor", ImVec4(1.f, 0.35f, 0.35f, 1.f), ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoPicker, ImVec2(14.f, 14.f));
    ImGui::SameLine();
    ImGui::Checkbox("Damage zones", &overlays.showDamageZones);
    DebugWidgets::help_marker("Where the hit actually lands.");
}

#endif
