#include "ImageAssetEditor.h"

#include "../EditorWidgets.h"
#include "../ProjectQueries.h"

#include <algorithm>

using namespace EditorWidgets;

namespace ImageAssetEditor
{
    void draw(EditorContext& ctx)
    {
        const std::string& path = ctx.selection.name;
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

        // ----- Image -----
        const float width = std::max(100.f, ImGui::GetContentRegionAvail().x);
        const ImVec2 source(static_cast<float>(texture->getSize().x), static_cast<float>(texture->getSize().y));
        image_in_box(texture, fit(source, ImVec2(width, std::max(200.f, width * 0.6f))), {}, true);

        // ----- Uses -----
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
}
