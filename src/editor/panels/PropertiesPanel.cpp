#include "PropertiesPanel.h"

#include "../EditorContext.h"
#include "../EditorWidgets.h"
#include "../editors/UnitEditor.h"
#include "../editors/EnemyEditor.h"
#include "../editors/StageEditor.h"
#include "../editors/ChapterEditor.h"
#include "../editors/ImageAssetEditor.h"

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
        ImGui::Text("%s", read_string(status, "name", ctx.selection.name).c_str());
        ImGui::TextDisabled("Status %s   statuses.json", ctx.selection.name.c_str());

        if (!begin_properties("Status")) return;
        const DataFile file = DataFile::STATUSES;
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

        int_field(ctx, file, "Icon index", status, "iconIndex", -1, 255, "Index in the status icons sprite sheet.", 0.05f);
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
    case ContentKind::BASE:
    case ContentKind::BACKGROUND: ImageAssetEditor::draw(ctx); break;
    case ContentKind::STATUS:     draw_status(ctx); break;
    default:                      draw_welcome(); break;
    }
}
