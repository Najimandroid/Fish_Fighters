#include "ImageAssetEditor.h"

#include "../EditorWidgets.h"
#include "../ProjectQueries.h"
#include "../../data/BattlefieldLayout.h"

#include <algorithm>
#include <cmath>

using namespace EditorWidgets;

namespace
{
    const ImU32 SEAM_COLOR = IM_COL32(255, 160, 115, 255);
    constexpr int DEFAULT_OVERLAP = 128;

    // Tiling tab state for the image shown
    std::string s_tilingPath;
    int s_overlapSlider = DEFAULT_OVERLAP;   // Edited value, written to backgrounds.json when released
    float s_closeUpRow = -1.f;               // Top row of the close-up, -1 = around the ground line

    // Two copies side by side: the join the game shows is in the middle
    void draw_side_by_side(const sf::Texture& texture, float width)
    {
        const ImVec2 imageSize(static_cast<float>(texture.getSize().x), static_cast<float>(texture.getSize().y));
        const ImVec2 copySize = fit(imageSize, ImVec2(width / 2.f, 380.f));
        const ImVec2 origin = ImGui::GetCursorScreenPos();
        ImGui::Dummy(ImVec2(copySize.x * 2.f, copySize.y + 8.f));

        ImDrawList* drawList = ImGui::GetWindowDrawList();
        for (int copy = 0; copy < 2; ++copy)
        {
            const ImVec2 min(origin.x + copySize.x * copy, origin.y + 4.f);
            drawList->AddImage(texture_id(texture), min, ImVec2(min.x + copySize.x, min.y + copySize.y));
        }

        // Arrows above and below the join, nothing over it
        const float seamX = origin.x + copySize.x;
        drawList->AddTriangleFilled(ImVec2(seamX - 6.f, origin.y - 4.f), ImVec2(seamX + 6.f, origin.y - 4.f), ImVec2(seamX, origin.y + 4.f), SEAM_COLOR);
        const float bottom = origin.y + copySize.y + 4.f;
        drawList->AddTriangleFilled(ImVec2(seamX - 6.f, bottom + 8.f), ImVec2(seamX + 6.f, bottom + 8.f), ImVec2(seamX, bottom), SEAM_COLOR);
    }

    // The right edge next to the left edge at 1:1, rows [topRow, topRow + height)
    void draw_seam_close_up(const sf::Texture& texture, float width, float height, float topRow)
    {
        const float imageWidth = static_cast<float>(texture.getSize().x);
        const float imageHeight = static_cast<float>(texture.getSize().y);
        const float half = std::min(width / 2.f, imageWidth / 2.f);
        const float rows = std::min(height, imageHeight);
        const ImVec2 origin = ImGui::GetCursorScreenPos();
        ImGui::Dummy(ImVec2(half * 2.f, rows));

        const float v0 = topRow / imageHeight;
        const float v1 = (topRow + rows) / imageHeight;
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        drawList->AddImage(texture_id(texture), origin, ImVec2(origin.x + half, origin.y + rows),
            ImVec2((imageWidth - half) / imageWidth, v0), ImVec2(1.f, v1));
        drawList->AddImage(texture_id(texture), ImVec2(origin.x + half, origin.y), ImVec2(origin.x + half * 2.f, origin.y + rows),
            ImVec2(0.f, v0), ImVec2(half / imageWidth, v1));
        drawList->AddTriangleFilled(ImVec2(origin.x + half - 6.f, origin.y), ImVec2(origin.x + half + 6.f, origin.y), ImVec2(origin.x + half, origin.y + 8.f), SEAM_COLOR);
    }

    // ==================
    // Tabs
    // ==================
    void general_tab(EditorContext& ctx, const std::string& path, const sf::Texture& texture, std::optional<ImageFolder> folder)
    {
        const float width = std::max(100.f, ImGui::GetContentRegionAvail().x);
        const ImVec2 source(static_cast<float>(texture.getSize().x), static_cast<float>(texture.getSize().y));
        image_in_box(&texture, fit(source, ImVec2(width, std::max(200.f, width * 0.6f))), {}, true);

        section("Used by");
        const std::vector<Reference> uses = ProjectQueries::uses_of_image(ctx.project, path);
        if (folder == ImageFolder::MENU_BACKGROUNDS)
            ImGui::TextDisabled("Menu backgrounds are chosen in the game's code (UIManager), not in the data.");
        else if (folder == ImageFolder::UNIT_ICONS)
            ImGui::TextDisabled("Deploy icons are found by their name: icon_<unit UID>_<form>.png (form 0 is the first).");
        else if (uses.empty())
            ImGui::TextDisabled("Nothing uses it yet.");

        for (size_t i = 0; i < uses.size(); ++i)
        {
            ImGui::PushID(static_cast<int>(i));
            if (ImGui::TextLink(uses[i].label.c_str())) ctx.defer([&ctx, target = uses[i].target]() { ctx.select(target); });
            ImGui::PopID();
        }
    }

    // Seamless joins: a setting of the background ("seamlessOverlap" in backgrounds.json) the game applies when
    // it loads the stage. The file isn't changed, and the views show exactly what the game draws
    void tiling_tab(EditorContext& ctx, const std::string& path, const sf::Texture& fileTexture)
    {
        const DataFile file = DataFile::BACKGROUNDS;
        ordered_json& backgrounds = ctx.project.get(file);
        const int overlap = ProjectQueries::background_seamless_overlap(ctx.project, path);
        const bool isSeamless = overlap > 0;

        if (s_tilingPath != path)
        {
            s_tilingPath = path;
            s_overlapSlider = isSeamless ? overlap : DEFAULT_OVERLAP;
            s_closeUpRow = -1.f;
        }

        ImGui::PushTextWrapPos(0.f);
        ImGui::TextDisabled("The game repeats this background side by side. With Seamless on, the game joins the copies "
            "when it loads the stage: the right edge is laid over the left one and cut along the path where both look most "
            "alike, only the light and colors are blended. The image file is never changed. Saved in backgrounds.json, "
            "for every stage using this background.");
        ImGui::PopTextWrapPos();

        // ----- Setting -----
        bool isEnabled = isSeamless;
        if (ImGui::Checkbox("Seamless in the game", &isEnabled))
        {
            if (isEnabled) backgrounds[path]["seamlessOverlap"] = s_overlapSlider;
            else
            {
                backgrounds[path].erase("seamlessOverlap");
                if (backgrounds[path].empty()) backgrounds.erase(path);
            }
            ctx.commit(file);
        }

        ImGui::SameLine(0.f, 20.f);
        ImGui::BeginDisabled(!isEnabled);
        ImGui::SetNextItemWidth(220.f);
        const int maxOverlap = std::max(32, static_cast<int>(fileTexture.getSize().x / 3));
        if (isSeamless && !ImGui::IsAnyItemActive()) s_overlapSlider = overlap; // Follows undo / redo when not dragged
        ImGui::SliderInt("Overlap", &s_overlapSlider, 32, maxOverlap, "%d px");
        if (ImGui::IsItemDeactivatedAfterEdit() && isEnabled)
        {
            backgrounds[path]["seamlessOverlap"] = s_overlapSlider;
            ctx.commit(file);
        }
        ImGui::EndDisabled();
        ImGui::SameLine();
        help_marker("Columns the two edges overlap on: the image gets that much narrower in the game.\n"
            "Wider: more room to hide the cut and a softer change of light. Narrower: keeps more of the image.");

        // ----- What the game draws -----
        const sf::Texture* shown = background_texture(ctx, path);
        if (!shown) shown = &fileTexture;
        if (isSeamless)
            ImGui::TextColored(ImVec4(0.45f, 0.9f, 0.6f, 1.f), "The game joins the copies seamlessly (%u px wide instead of %u).",
                shown->getSize().x, fileTexture.getSize().x);
        else
            ImGui::TextDisabled("The game draws the file as it is: check the join below.");

        section("Side by side, as the game draws it");
        draw_side_by_side(*shown, ImGui::GetContentRegionAvail().x);

        section("Join close-up, 1:1");
        const float closeUpHeight = 260.f;
        const float maxRow = std::max(0.f, static_cast<float>(shown->getSize().y) - closeUpHeight);
        if (s_closeUpRow < 0.f)
            s_closeUpRow = std::clamp(ProjectQueries::background_ground_row(ctx.project, path, static_cast<float>(shown->getSize().y))
                - closeUpHeight / 2.f, 0.f, maxRow);
        ImGui::SetNextItemWidth(260.f);
        ImGui::SliderFloat("Height", &s_closeUpRow, 0.f, maxRow, "from row %.0f");
        draw_seam_close_up(*shown, ImGui::GetContentRegionAvail().x, closeUpHeight, s_closeUpRow);
    }
}

namespace ImageAssetEditor
{
    void draw(EditorContext& ctx)
    {
        const std::string path = ctx.selection.name;
        const sf::Texture* texture = ctx.textures.get(path);

        const auto folder = AssetLibrary::folder_of(path);
        ImGui::Text("%s", path.substr(path.find_last_of('/') + 1).c_str());
        ImGui::TextDisabled("%s   %s", folder ? AssetLibrary::display_name(*folder) : "Image", path.c_str());

        if (!texture)
        {
            ImGui::TextColored(ImVec4(1.f, 0.45f, 0.4f, 1.f), "This file can't be read.");
            return;
        }
        ImGui::Text("%u x %u px", texture->getSize().x, texture->getSize().y);

        // ----- Categories (Ground line and Tiling only for stage backgrounds, the images the game repeats) -----
        if (!ImGui::BeginTabBar("ImageCategories")) return;
        if (ImGui::BeginTabItem("General"))
        {
            general_tab(ctx, path, *texture, folder);
            ImGui::EndTabItem();
        }
        if (folder == ImageFolder::STAGE_BACKGROUNDS && ImGui::BeginTabItem("Ground line"))
        {
            ground_line_field(ctx, path);
            ImGui::EndTabItem();
        }
        if (folder == ImageFolder::STAGE_BACKGROUNDS && ImGui::BeginTabItem("Tiling"))
        {
            tiling_tab(ctx, path, *texture);
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
}
