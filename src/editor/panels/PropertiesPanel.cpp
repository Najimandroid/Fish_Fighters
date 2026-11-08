#include "PropertiesPanel.h"

#include "../EditorContext.h"
#include "../EditorWidgets.h"
#include "../editors/UnitEditor.h"
#include "../editors/EnemyEditor.h"
#include "../editors/StageEditor.h"
#include "../editors/ChapterEditor.h"
#include "../editors/ImageAssetEditor.h"
#include "../../core/entities/status/StatusEffect.h"

#include <string>

using namespace EditorWidgets;

namespace
{
    // statuses.json: the game needs every status, so they're only edited, never added or removed
    void draw_status(EditorContext& ctx)
    {
        ordered_json& statuses = ctx.project.get(DataFile::STATUSES);
        if (!statuses.contains(ctx.selection.name))
        {
            ImGui::TextDisabled("Status %s doesn't exist anymore.", ctx.selection.name.c_str());
            return;
        }

        ordered_json& status = statuses[ctx.selection.name];
        const DataFile file = DataFile::STATUSES;
        const int iconIndex = read_int(status, "iconIndex", -1);

        // ----- Header: the icon shown in battle -----
        FrameUV uv;
        const sf::Texture* icon = status_icon(ctx, iconIndex, uv);
        image_in_box(icon, ImVec2(64.f, 64.f), uv, true);
        ImGui::SameLine();
        ImGui::BeginGroup();
        ImGui::Text("%s", read_string(status, "name", ctx.selection.name).c_str());
        ImGui::TextDisabled("Status %s   statuses.json", ctx.selection.name.c_str());
        ImGui::EndGroup();

        // ----- Icon: one cell of the status icons sheet -----
        section("Icon");
        const int cellCount = status_icon_count(ctx);
        if (cellCount == 0) ImGui::TextColored(ImVec4(1.f, 0.45f, 0.4f, 1.f), "Missing icon sheet: %s", STATUS_ICONS_TEXTURE);
        else ImGui::TextDisabled("%s, %d px cells. Shown under the entities in battle.", STATUS_ICONS_TEXTURE, STATUS_ICON_CELL_SIZE);

        for (int cell = -1; cell < cellCount; ++cell)
        {
            if (cell > -1) ImGui::SameLine();
            ImGui::PushID(cell);
            FrameUV cellUv;
            const sf::Texture* cellTexture = status_icon(ctx, cell, cellUv);
            if (tile("##cell", cellTexture, cellUv, cell < 0 ? "None" : std::to_string(cell), cell == iconIndex, ImVec2(76.f, 92.f)))
            {
                status["iconIndex"] = cell;
                ctx.commit(file);
            }
            ImGui::PopID();
        }

        section("General");
        if (!begin_properties("Status")) return;
        text_field(ctx, file, "Name", status, "name", "Shown to the player.");
        text_field(ctx, file, "Description", status, "description", "For the \"more information\" panel.", true);

        property_label("Stronger value", "When an entity already has this status, the duration is added and the stronger value is kept.");
        const bool isHighest = read_string(status, "strongest") == "highest";
        int choice = isHighest ? 1 : 0;
        const char* choices[] = { "Lowest (e.g. WEAKEN 0.3 beats 0.5)", "Highest" };
        if (ImGui::Combo("##strongest", &choice, choices, 2))
        {
            status["strongest"] = choice == 1 ? "highest" : "lowest";
            ctx.commit(file);
        }
        end_properties();
    }

    void draw_welcome()
    {
        ImGui::TextDisabled("Select something in Content to edit it.");
        ImGui::Spacing();
        ImGui::BulletText("Changes are checked with the game's own loader (Validation panel).");
        ImGui::BulletText("Ctrl+S saves the JSON files of game_data, Ctrl+Z / Ctrl+Y undo and redo.");
        ImGui::BulletText("Images are copied into the project's assets folder when imported.");
    }
}

void PropertiesPanel::draw(EditorContext& ctx)
{
    switch (ctx.selection.kind)
    {
    case ContentKind::UNIT:       UnitEditor::draw(ctx); break;
    case ContentKind::ENEMY:      EnemyEditor::draw(ctx); break;
    case ContentKind::STAGE:      StageEditor::draw(ctx); break;
    case ContentKind::CHAPTER:    ChapterEditor::draw(ctx); break;
    case ContentKind::IMAGE:      ImageAssetEditor::draw(ctx); break;
    case ContentKind::STATUS:     draw_status(ctx); break;
    default:                      draw_welcome(); break;
    }
}
