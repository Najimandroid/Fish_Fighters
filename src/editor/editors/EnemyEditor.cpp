#include "EnemyEditor.h"

#include "EntityFields.h"
#include "../EditorWidgets.h"
#include "../ProjectQueries.h"

using namespace EditorWidgets;

namespace
{
    const DataFile ENEMIES_FILE = DataFile::ENEMIES;

    void general_tab(EditorContext& ctx, int uid, ordered_json& enemy)
    {
        if (begin_properties("General"))
        {
            text_field(ctx, ENEMIES_FILE, "Name", enemy, "name");
            text_field(ctx, ENEMIES_FILE, "Description", enemy, "description", nullptr, true);
            types_field(ctx, ENEMIES_FILE, enemy);
            end_properties();
        }

        section("Spawned in");
        const auto uses = ProjectQueries::uses_of_enemy(ctx.project, uid);
        if (uses.empty()) ImGui::TextDisabled("No stage spawns it yet.");
        for (size_t i = 0; i < uses.size(); ++i)
        {
            ImGui::PushID(static_cast<int>(i));
            if (ImGui::TextLink(uses[i].label.c_str())) ctx.defer([&ctx, target = uses[i].target]() { ctx.select(target); });
            ImGui::PopID();
        }
    }

    void combat_tab(EditorContext& ctx, ordered_json& enemy)
    {
        section("Stats");
        if (begin_properties("Stats"))
        {
            EntityFields::stats(ctx, ENEMIES_FILE, enemy);
            end_properties();
        }

        section("Attack");
        if (begin_properties("Attack"))
        {
            EntityFields::attack(ctx, ENEMIES_FILE, enemy);
            end_properties();
        }

        section("Abilities");
        abilities_field(ctx, ENEMIES_FILE, enemy, true);
    }

    void appearance_tab(EditorContext& ctx, ordered_json& enemy)
    {
        if (begin_properties("Appearance"))
        {
            EntityFields::sprite(ctx, ENEMIES_FILE, enemy, ImageFolder::ENEMY_SHEETS);
            end_properties();
        }
    }
}

namespace EnemyEditor
{
    void draw(EditorContext& ctx)
    {
        const int uid = ctx.selection.uid;
        ordered_json* enemy = ctx.project.find_uid(ENEMIES_FILE, uid);
        if (!enemy)
        {
            ImGui::TextDisabled("Enemy %d doesn't exist anymore.", uid);
            return;
        }

        // ----- Header -----
        ImGui::PushID(uid);
        enemy_icon(ctx, uid, ImVec2(96.f, 72.f));
        ImGui::SameLine();
        ImGui::BeginGroup();
        ImGui::Text("%s", ProjectQueries::enemy_name(ctx.project, uid).c_str());
        ImGui::TextDisabled("Enemy #%d   enemies.json", uid);
        ImGui::EndGroup();
        ImGui::PopID();
        ImGui::Spacing();

        // ----- Categories (same tab bar as units: the open tab stays when switching) -----
        if (ImGui::BeginTabBar("EntityCategories"))
        {
            auto tab = [&](const char* name, auto drawContent)
                {
                    if (!ImGui::BeginTabItem(name)) return;
                    ImGui::PushID(uid);
                    drawContent();
                    ImGui::PopID();
                    ImGui::EndTabItem();
                };
            tab("General", [&]() { general_tab(ctx, uid, *enemy); });
            tab("Combat", [&]() { combat_tab(ctx, *enemy); });
            tab("Appearance", [&]() { appearance_tab(ctx, *enemy); });
            ImGui::EndTabBar();
        }
    }
}
