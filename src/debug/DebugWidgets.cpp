#ifdef DEBUG_MODE

#include "DebugWidgets.h"

#include "../core/stages/Stage.h"
#include "../core/entities/BattleEntity.h"
#include "../core/entities/BattleBase.h"

#include "imgui-SFML.h"

#include <SFML/Graphics.hpp>

#include <algorithm>
#include <map>

namespace DebugWidgets
{
    std::vector<DebugEntityRow> collect_entities(Stage& stage)
    {
        std::vector<DebugEntityRow> rows;
        if (!stage.is_loaded()) return rows;

        if (auto base = stage.get_unit_base().lock()) rows.push_back({ base, "Player base" });
        if (auto base = stage.get_enemy_base().lock()) rows.push_back({ base, "Enemy base" });
        for (const auto& [layer, list] : stage.get_units()) for (const auto& entity : list) rows.push_back({ entity, "Unit" });
        for (const auto& [layer, list] : stage.get_enemies()) for (const auto& entity : list) rows.push_back({ entity, "Enemy" });

        std::sort(rows.begin(), rows.end(), [](const DebugEntityRow& a, const DebugEntityRow& b)
            {
                return a.entity->uniqueId < b.entity->uniqueId;
            });
        return rows;
    }

    std::string entity_name(const DebugEntityRow& row)
    {
        return row.entity->data ? row.entity->data->name : row.side;
    }

    ImVec4 side_color(const char* side)
    {
        const std::string text = side;
        if (text == "Unit") return ImVec4(0.55f, 0.85f, 1.f, 1.f);
        if (text == "Enemy") return ImVec4(1.f, 0.6f, 0.55f, 1.f);
        return ImVec4(0.85f, 0.85f, 0.85f, 1.f);
    }

    float max_health(const BattleEntity& entity)
    {
        if (auto base = dynamic_cast<const BattleBase*>(&entity)) return base->maxHealth;
        return entity.data ? entity.data->health * entity.magnification.x : 0.f;
    }

    float weaken_multiplier(const BattleEntity& entity)
    {
        const StatusEffect* weaken = entity.get_status(StatusType::WEAKEN);
        return weaken ? weaken->value : 1.f;
    }

    float damage_per_hit(const BattleEntity& entity)
    {
        if (!entity.data) return 0.f;
        return entity.data->attackPower * entity.magnification.y * weaken_multiplier(entity);
    }

    void edit_type_list(std::vector<EntityType>& types)
    {
        for (int i = 0; i < static_cast<int>(EntityType::COUNT); ++i)
        {
            EntityType type = static_cast<EntityType>(i);
            bool hasType = std::find(types.begin(), types.end(), type) != types.end();

            ImGui::PushID(i);
            if (ImGui::Checkbox(to_string(type), &hasType))
            {
                if (hasType) types.push_back(type);
                else types.erase(std::remove(types.begin(), types.end(), type), types.end());
            }
            ImGui::PopID();

            if (i + 1 < static_cast<int>(EntityType::COUNT)) ImGui::SameLine();
        }
    }

    void edit_types_and_abilities(std::vector<EntityType>& types, std::vector<AbilityData>& abilities,
        const std::vector<EntityType>& defaultTargets)
    {
        ImGui::SeparatorText("Types");
        ImGui::PushID("Types");
        edit_type_list(types);
        ImGui::PopID();

        ImGui::SeparatorText("Abilities");
        int abilityToRemove = -1;

        for (int a = 0; a < static_cast<int>(abilities.size()); ++a)
        {
            AbilityData& ability = abilities[a];
            ImGui::PushID(a);

            if (ImGui::BeginCombo("Ability", to_string(ability.type)))
            {
                for (int t = static_cast<int>(AbilityType::NONE) + 1; t < static_cast<int>(AbilityType::COUNT); ++t)
                {
                    AbilityType abilityType = static_cast<AbilityType>(t);
                    if (ImGui::Selectable(to_string(abilityType), abilityType == ability.type))
                    {
                        ability.type = abilityType;

                        // Sensible defaults so a new on-hit ability works right away
                        if (is_on_hit(abilityType) && ability.duration <= 0.f)
                        {
                            ability.value = 0.5f;
                            ability.chance = 100.f;
                            ability.duration = 3.f;
                        }
                    }
                }
                ImGui::EndCombo();
            }

            ImGui::TextUnformatted("Targets:");
            ImGui::SameLine();
            ImGui::PushID("Targets");
            edit_type_list(ability.targetTypes);
            ImGui::PopID();

            if (is_on_hit(ability.type))
            {
                if (ability.type != AbilityType::PARALYZE_ATTACK)
                    ImGui::SliderFloat("Value", &ability.value, 0.05f, 2.f, "x%.2f");
                ImGui::SliderFloat("Chance", &ability.chance, 0.f, 100.f, "%.0f%%");
                ImGui::SliderFloat("Duration", &ability.duration, 0.1f, 30.f, "%.1f s");
            }

            if (ImGui::SmallButton("Remove ability")) abilityToRemove = a;
            ImGui::Separator();

            ImGui::PopID();
        }

        if (abilityToRemove >= 0) abilities.erase(abilities.begin() + abilityToRemove);

        if (ImGui::Button("Add ability"))
            abilities.push_back(AbilityData{ AbilityType::STRONG_AGAINST, defaultTargets });
    }

    void show_preview(const std::string& id, const std::string& texturePath, int frameCount)
    {
        struct Preview
        {
            sf::Texture texture;
            sf::Sprite sprite{ texture };
            std::string loadedPath;
        };
        static std::map<std::string, std::unique_ptr<Preview>> previews;

        auto& preview = previews[id];
        if (!preview) preview = std::make_unique<Preview>();

        if (preview->loadedPath != texturePath)
        {
            if (!preview->texture.loadFromFile(texturePath)) return;
            preview->loadedPath = texturePath;

            int frameWidth = preview->texture.getSize().x / std::max(1, frameCount);
            int frameHeight = preview->texture.getSize().y;

            preview->sprite.setTexture(preview->texture);
            preview->sprite.setTextureRect(sf::IntRect({ 0, 0 }, { frameWidth, frameHeight }));

            float maxPreviewSize = 200.f;
            float scale = std::min(maxPreviewSize / frameWidth, maxPreviewSize / frameHeight);
            preview->sprite.setScale({ scale, scale });
        }

        ImGui::Image(preview->sprite);
    }

    void help_marker(const char* text)
    {
        ImGui::SameLine();
        ImGui::TextDisabled("(?)");
        if (ImGui::BeginItemTooltip())
        {
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 30.f);
            ImGui::TextUnformatted(text);
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
    }
}

#endif
