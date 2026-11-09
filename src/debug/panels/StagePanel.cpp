#ifdef DEBUG_MODE

#include "StagePanel.h"

#include "../DebugWidgets.h"
#include "../../core/GameSpeedState.h"
#include "../../core/DataLoader.h"
#include "../../core/stages/Stage.h"
#include "../../uis/UIManager.h"

#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>

void StagePanel::draw(DebugContext& context)
{
    if (context.stage->is_loaded()) draw_battle(context);
    else draw_menus(context);
}

void StagePanel::draw_menus(DebugContext& context)
{
    ImGui::TextDisabled("No stage loaded");

    // Only defined stages can be picked (the DEBUG stage is UID -1)
    std::vector<std::shared_ptr<StageData>> stages;
    for (const auto& [uid, data] : context.dataLoader->get_stages_database()) stages.push_back(data);
    std::sort(stages.begin(), stages.end(), [](const auto& a, const auto& b) { return a->UID < b->UID; });

    auto stageLabel = [](const StageData& data) { return std::to_string(data.UID) + " - " + data.name; };

    std::shared_ptr<StageData> selectedStage;
    for (const auto& data : stages) if (data->UID == m_selectedStageUid) selectedStage = data;
    if (!selectedStage && !stages.empty()) { selectedStage = stages.front(); m_selectedStageUid = selectedStage->UID; }

    if (selectedStage)
    {
        if (ImGui::BeginCombo("Stage", stageLabel(*selectedStage).c_str()))
        {
            for (const auto& data : stages)
            {
                if (ImGui::Selectable(stageLabel(*data).c_str(), data->UID == m_selectedStageUid))
                    m_selectedStageUid = data->UID;
            }
            ImGui::EndCombo();
        }

        if (ImGui::Button("Load Stage", ImVec2(-FLT_MIN, 0.f)))
        {
            context.stage->load(m_selectedStageUid);
            context.uiManager->generate_battle_uis();
        }
    }

    ImGui::SeparatorText("Player");
    if (auto player = context.dataLoader->get_player_data().lock())
    {
        ImGui::Text("Shells: %d", player->shells);
        ImGui::InputInt("##ShellAmount", &m_shellAmount, 100, 1000);
        ImGui::SameLine();
        if (ImGui::Button("Add shells")) player->gain_shells(m_shellAmount);
    }
}

void StagePanel::draw_battle(DebugContext& context)
{
    Stage& stage = *context.stage;
    auto stageData = context.dataLoader->get_stage_data(stage.get_current_uid());

    ImGui::Text("%d - %s", stage.get_current_uid(), stageData->name.c_str());
    ImGui::Text("Time: %.1f s", stage.get_elapsed_time());

    if (ImGui::Button("Exit stage (no rewards)", ImVec2(-FLT_MIN, 0.f)))
    {
        context.setGameSpeed(GameSpeedState::NORMAL);
        stage.unload();
        context.uiManager->generate_fish_tank_uis();
        return;
    }

    // ----- Instant result -----
    // Destroys a base through take_damage, like any other loss of health
    ImGui::SeparatorText("Result");
    auto destroy = [](const std::weak_ptr<BattleBase>& weakBase)
        {
            if (auto base = weakBase.lock())
            {
                DamageResult destroyHit;
                destroyHit.finalDamage = base->currentHealth;
                base->take_damage(destroyHit);
            }
        };
    if (ImGui::Button("Win now")) destroy(stage.get_enemy_base());
    ImGui::SameLine();
    if (ImGui::Button("Lose now")) destroy(stage.get_unit_base());

    // ----- Cash -----
    ImGui::SeparatorText("Cash");
    char cashText[64];
    std::snprintf(cashText, sizeof(cashText), "%d / %d", stage.get_cash(), stage.get_max_cash());
    ImGui::ProgressBar(stage.get_max_cash() > 0 ? static_cast<float>(stage.get_cash()) / stage.get_max_cash() : 0.f, ImVec2(-FLT_MIN, 0.f), cashText);
    if (ImGui::Button("Fill")) stage.set_cash(stage.get_max_cash());
    ImGui::SameLine();
    if (ImGui::Button("Empty")) stage.set_cash(0);

    // ----- Bases -----
    ImGui::SeparatorText("Bases");
    auto baseBar = [](const char* label, const std::weak_ptr<BattleBase>& weakBase)
        {
            if (auto base = weakBase.lock())
            {
                char text[96];
                std::snprintf(text, sizeof(text), "%s: %.0f / %.0f", label, base->currentHealth, base->maxHealth);
                ImGui::ProgressBar(base->maxHealth > 0.f ? base->currentHealth / base->maxHealth : 0.f, ImVec2(-FLT_MIN, 0.f), text);
            }
        };
    baseBar("Player base", stage.get_unit_base());
    baseBar("Enemy base", stage.get_enemy_base());
}

#endif
