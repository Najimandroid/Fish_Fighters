#include "ImageAssetEditor.h"

#include "../EditorWidgets.h"
#include "../ImageTools.h"
#include "../ProjectQueries.h"
#include "../../data/BattlefieldLayout.h"

#include <SFML/Graphics/Texture.hpp>

#include <algorithm>
#include <memory>

using namespace EditorWidgets;

namespace
{
    const ImU32 SEAM_COLOR = IM_COL32(255, 160, 115, 255);

    // ==================
    // Seamless tool: state for the image shown
    // ==================
    struct SeamlessTool
    {
        std::string path;                        // Image this state belongs to
        sf::Image source;                        // The file as it is on disk
        bool isSourceLoaded = false;
        int blendWidth = 128;                    // Columns the two edges overlap on
        bool isPreviewingBlend = true;
        int resultBlendWidth = -1;               // Blend width of `result`, -1 = not computed
        sf::Image result;
        std::unique_ptr<sf::Texture> resultTexture;
        float closeUpRow = -1.f;                 // Top row of the close-up, -1 = around the ground line
    };
    SeamlessTool s_tool;

    void load_source(EditorContext& ctx, const std::string& path)
    {
        s_tool.path = path;
        s_tool.isSourceLoaded = s_tool.source.loadFromFile(ctx.project.resolve(path));
        s_tool.resultBlendWidth = -1;
        s_tool.closeUpRow = -1.f;
    }

    bool compute_result()
    {
        s_tool.result = ImageTools::make_seamless(s_tool.source, static_cast<unsigned>(s_tool.blendWidth));
        if (!s_tool.resultTexture) s_tool.resultTexture = std::make_unique<sf::Texture>();
        if (!s_tool.resultTexture->loadFromImage(s_tool.result)) return false;
        s_tool.resultBlendWidth = s_tool.blendWidth;
        return true;
    }

    // Two copies side by side: the seam the game shows is in the middle
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

        // Arrows above and below the seam, nothing over it
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

    void tiling_tab(EditorContext& ctx, const std::string& path, const sf::Texture& fileTexture)
    {
        if (s_tool.path != path) load_source(ctx, path);
        if (!s_tool.isSourceLoaded)
        {
            ImGui::TextColored(ImVec4(1.f, 0.45f, 0.4f, 1.f), "Can't read this image.");
            return;
        }
        const sf::Vector2u size = s_tool.source.getSize();

        ImGui::PushTextWrapPos(0.f);
        ImGui::TextDisabled("The game repeats this background side by side. If the seam shows, the right edge is laid over "
            "the left one and cut along the path where both look most alike; only the light and colors are blended across "
            "the band. The image gets %d px narrower (the game takes any width).", s_tool.blendWidth);
        ImGui::PopTextWrapPos();

        // ----- Blend settings -----
        ImGui::Checkbox("Preview", &s_tool.isPreviewingBlend);
        ImGui::SameLine(0.f, 20.f);
        ImGui::SetNextItemWidth(220.f);
        ImGui::SliderInt("Overlap", &s_tool.blendWidth, 32, std::max(32, static_cast<int>(size.x / 3)), "%d px");
        const bool isSliding = ImGui::IsItemActive();
        ImGui::SameLine();
        help_marker("Wider: more room to find a hidden path and a softer change of light. "
            "Narrower: keeps more of the image.");

        // Recomputed when the slider is released (a few milliseconds)
        if (s_tool.isPreviewingBlend && !isSliding && s_tool.resultBlendWidth != s_tool.blendWidth) compute_result();
        const bool isShowingResult = s_tool.isPreviewingBlend && s_tool.resultBlendWidth >= 0 && s_tool.resultTexture;
        const sf::Texture& shown = isShowingResult ? *s_tool.resultTexture : fileTexture;

        // ----- Apply / restore -----
        if (ImGui::Button("Make seamless"))
        {
            std::string error;
            if ((s_tool.resultBlendWidth == s_tool.blendWidth || compute_result()) && ctx.assets.replace(path, s_tool.result, error))
            {
                ctx.textures.forget(path);
                load_source(ctx, path);
                s_tool.isPreviewingBlend = false; // The file is now the blended one
                ctx.notify("Made " + path + " seamless. The previous version is kept (Restore previous version)");
            }
            else ctx.notify(error.empty() ? "Can't make it seamless" : error, true);
        }
        ImGui::SetItemTooltip("Writes the blended image (the previous one is kept as a backup)");

        if (const auto backup = ctx.assets.latest_backup(path))
        {
            ImGui::SameLine();
            if (ImGui::Button("Restore previous version"))
            {
                std::string error;
                if (ctx.assets.restore_backup(path, error))
                {
                    ctx.textures.forget(path);
                    load_source(ctx, path);
                    ctx.notify("Restored " + backup->filename().string());
                }
                else ctx.notify(error, true);
            }
            ImGui::SetItemTooltip("Puts back %s", backup->filename().string().c_str());
        }

        // ----- Views -----
        section(isShowingResult ? "Side by side (blended)" : "Side by side");
        draw_side_by_side(shown, ImGui::GetContentRegionAvail().x);

        section("Seam close-up, 1:1");
        const float closeUpHeight = 260.f;
        const float maxRow = std::max(0.f, static_cast<float>(size.y) - closeUpHeight);
        if (s_tool.closeUpRow < 0.f)
            s_tool.closeUpRow = std::clamp(background_ground_row(-1.f, static_cast<float>(size.y)) - closeUpHeight / 2.f, 0.f, maxRow);
        ImGui::SetNextItemWidth(260.f);
        ImGui::SliderFloat("Height", &s_tool.closeUpRow, 0.f, maxRow, "from row %.0f");
        draw_seam_close_up(shown, ImGui::GetContentRegionAvail().x, closeUpHeight, s_tool.closeUpRow);
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

        // ----- Categories (Tiling only for stage backgrounds, the images the game repeats) -----
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
