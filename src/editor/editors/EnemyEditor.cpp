#include "EnemyEditor.h"

#include "EntityFields.h"
#include "../EditorWidgets.h"
#include "../ProjectQueries.h"

using namespace EditorWidgets;

namespace EnemyEditor
{
    void draw(EditorContext& ctx)
    {
        const int uid = ctx.selection.uid;
        ordered_json* enemy = ctx.project.find_uid(DataFile::ENEMIES, uid);
        if (!enemy)
        {
            ImGui::TextDisabled("Enemy %d doesn't exist anymore.", uid);
            return;
        }

        const DataFile file = DataFile::ENEMIES;
        ImGui::PushID(uid);

        // ----- Header -----
        enemy_icon(ctx, uid, ImVec2(96.f, 72.f));
        ImGui::SameLine();
        ImGui::BeginGroup();
        ImGui::Text("%s", ProjectQueries::enemy_name(ctx.project, uid).c_str());
        const auto uses = ProjectQueries::uses_of_enemy(ctx.project, uid);
        ImGui::TextDisabled("Enemy #%d   in %zu wave(s)   enemies.json", uid, uses.size());
        ImGui::EndGroup();

        if (begin_properties("Identity"))
        {
            text_field(ctx, file, "Name", *enemy, "name");
            text_field(ctx, file, "Description", *enemy, "description", nullptr, true);
            types_field(ctx, file, *enemy);
            end_properties();
        }

        section("Stats");
        if (begin_properties("Stats"))
        {
            EntityFields::stats(ctx, file, *enemy);
            end_properties();
        }

        section("Attack");
        if (begin_properties("Attack"))
        {
            EntityFields::attack(ctx, file, *enemy);
            end_properties();
        }

        section("Abilities");
        abilities_field(ctx, file, *enemy, true);

        section("Sprite");
        if (begin_properties("Sprite"))
        {
            EntityFields::sprite(ctx, file, *enemy, ImageFolder::ENEMY_SHEETS);
            end_properties();
        }

        // ----- Where it appears -----
        section("Used in");
        if (uses.empty()) ImGui::TextDisabled("No stage spawns it yet.");
        for (size_t i = 0; i < uses.size(); ++i)
        {
            ImGui::PushID(static_cast<int>(i));
            if (ImGui::TextLink(uses[i].label.c_str())) ctx.defer([&ctx, target = uses[i].target]() { ctx.select(target); });
            ImGui::PopID();
        }

        ImGui::PopID();
    }
}
