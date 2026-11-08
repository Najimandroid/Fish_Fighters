#include "UnitEditor.h"

#include "EntityFields.h"
#include "../EditorWidgets.h"
#include "../ProjectQueries.h"

#include <algorithm>
#include <cmath>

using namespace EditorWidgets;

namespace
{
    constexpr float LEVEL_GROWTH = 0.15f;  // Same as Stage::spawn_unit: stats x1.15 per level above 1

    int s_formToSelect = -1;    // Tab to open on the next frame (after adding a form)
    int s_previewLevel = 1;

    // ===== Deploy icon (icon_<UID>_<form>.png, not stored in the JSON) =====
    void icon_field(EditorContext& ctx, int uid, int form, const ordered_json& formData)
    {
        property_label("Deploy icon", "Shown in the deploy bar and the equip menu.\nFile: icon_<UID>_<form>.png in assets/images/textures/icons/units.\n"
            "Replacing it keeps the previous file as icon_..(old).png.");

        const std::string iconPath = AssetLibrary::unit_icon_path(uid, form);
        const sf::Texture* icon = ctx.textures.get(iconPath);

        image_in_box(icon, ImVec2(112.f, 84.f), {}, true);
        ImGui::SameLine();
        ImGui::BeginGroup();
        if (icon) ImGui::TextDisabled("%u x %u", icon->getSize().x, icon->getSize().y);
        else ImGui::TextColored(ImVec4(1.f, 0.45f, 0.4f, 1.f), "No icon yet");

        const ImGuiID importId = ImGui::GetID("ImportIcon");
        if (ImGui::Button("Import...")) ctx.request_png_dialog(importId, "Pick the deploy icon (200 x 150)");

        ImGui::SameLine();
        const sf::Texture* sheet = ctx.textures.get(read_string(formData, "texture"));
        ImGui::BeginDisabled(!sheet);
        if (ImGui::Button("Make from first frame") && sheet)
        {
            std::string error;
            const auto image = AssetLibrary::make_icon(*sheet, read_int(formData, "frameCount", 1));
            if (image && ctx.assets.replace(iconPath, *image, error)) ctx.notify("Icon made from the first frame: " + iconPath);
            else ctx.notify(error.empty() ? "Can't make the icon" : error, true);
            ctx.textures.forget(iconPath);
            ctx.mark_changed();
        }
        ImGui::EndDisabled();
        ImGui::EndGroup();

        if (auto picked = ctx.take_dialog_result(importId))
        {
            std::string error;
            if (ctx.assets.replace(iconPath, *picked, error)) ctx.notify("Icon replaced: " + iconPath);
            else ctx.notify(error, true);
            ctx.textures.forget(iconPath);
            ctx.mark_changed();
        }
    }

    // ===== One form =====
    void draw_form(EditorContext& ctx, int uid, ordered_json& form, int formIndex)
    {
        const DataFile file = DataFile::UNITS;

        if (begin_properties("Identity"))
        {
            icon_field(ctx, uid, formIndex, form);
            text_field(ctx, file, "Name", form, "name");
            text_field(ctx, file, "Description", form, "description", "Shown in the equip and upgrade menus.", true);
            end_properties();
        }

        section("Levels and deployment");
        if (begin_properties("Levels"))
        {
            int_field(ctx, file, "Min level", form, "minLevel", 0, 999, "The unit uses this form from this level...", 0.1f);
            int_field(ctx, file, "Max level", form, "maxLevel", 0, 999, "...to this one. The next form starts at max level + 1.", 0.1f);
            int_field(ctx, file, "Deploy cost", form, "cost", 0, 1000000, "Cash spent in battle to deploy it.");
            number_field(ctx, file, "Cooldown", form, "cooldown", 0.f, 600.f, "Seconds before it can be deployed again.", 0.05f, "%.2f s");
            end_properties();
        }

        section("Stats");
        if (begin_properties("Stats"))
        {
            EntityFields::stats(ctx, file, form);
            end_properties();
        }

        section("Attack");
        if (begin_properties("Attack"))
        {
            EntityFields::attack(ctx, file, form);
            end_properties();
        }

        section("Abilities");
        abilities_field(ctx, file, form, false);

        section("Sprite");
        if (begin_properties("Sprite"))
        {
            EntityFields::sprite(ctx, file, form, ImageFolder::UNIT_SHEETS);
            end_properties();
        }

        // ----- What the player gets at each level -----
        section("Level preview");
        const int minLevel = std::max(1, read_int(form, "minLevel", 1));
        const int maxLevel = std::max(minLevel, read_int(form, "maxLevel", minLevel));
        s_previewLevel = std::clamp(s_previewLevel, minLevel, maxLevel);

        ImGui::SetNextItemWidth(260.f);
        ImGui::SliderInt("Level", &s_previewLevel, minLevel, maxLevel);
        ImGui::SameLine();
        help_marker("Stats are multiplied by 1.15 for each level above 1 (Stage::spawn_unit).");

        const float multiplier = std::pow(1.f + LEVEL_GROWTH, static_cast<float>(s_previewLevel - 1));
        if (ImGui::BeginTable("LevelPreview", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchSame))
        {
            ImGui::TableSetupColumn("Multiplier");
            ImGui::TableSetupColumn("Health");
            ImGui::TableSetupColumn("Attack");
            ImGui::TableSetupColumn("Damage per second");
            ImGui::TableHeadersRow();
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::Text("x%.2f", multiplier);
            ImGui::TableSetColumnIndex(1); ImGui::Text("%.0f", read_int(form, "health") * multiplier);
            ImGui::TableSetColumnIndex(2); ImGui::Text("%.0f", read_int(form, "attackPower") * multiplier);
            ImGui::TableSetColumnIndex(3); ImGui::Text("%.1f", EntityFields::damage_per_second(form, multiplier));
            ImGui::EndTable();
        }
    }
}

namespace UnitEditor
{
    void draw(EditorContext& ctx)
    {
        const int uid = ctx.selection.uid;
        ordered_json* unit = ctx.project.find_uid(DataFile::UNITS, uid);
        if (!unit)
        {
            ImGui::TextDisabled("Unit %d doesn't exist anymore.", uid);
            return;
        }
        if (!unit->contains("forms") || !(*unit)["forms"].is_array()) (*unit)["forms"] = ordered_json::array();
        ordered_json& forms = (*unit)["forms"];

        ImGui::PushID(uid);

        // ----- Header -----
        unit_icon(ctx, uid, std::clamp(ctx.selection.form, 0, std::max(0, static_cast<int>(forms.size()) - 1)), ImVec2(96.f, 72.f));
        ImGui::SameLine();
        ImGui::BeginGroup();
        ImGui::Text("%s", ProjectQueries::unit_name(ctx.project, uid).c_str());
        ImGui::TextDisabled("Unit #%d   %zu form(s)   units.json", uid, forms.size());
        ImGui::EndGroup();

        if (begin_properties("Unit"))
        {
            int_field(ctx, DataFile::UNITS, "Upgrade base cost", *unit, "baseUpgradeCost", 0, 100000000,
                "Shells for the first upgrade. Later upgrades cost more.");
            end_properties();
        }

        // ----- Forms -----
        int removedForm = -1;
        if (ImGui::BeginTabBar("Forms", ImGuiTabBarFlags_FittingPolicyScroll))
        {
            for (size_t i = 0; i < forms.size(); ++i)
            {
                const int formIndex = static_cast<int>(i);
                const std::string label = "Form " + std::to_string(i + 1) + ": " + read_string(forms[i], "name", "?") + "###Form" + std::to_string(i);
                ImGuiTabItemFlags flags = 0;
                if (s_formToSelect == formIndex) flags |= ImGuiTabItemFlags_SetSelected;

                if (ImGui::BeginTabItem(label.c_str(), nullptr, flags))
                {
                    if (s_formToSelect == formIndex) s_formToSelect = -1;
                    ctx.selection.form = formIndex;

                    ImGui::PushID(formIndex);
                    if (i + 1 == forms.size() && forms.size() > 1)
                    {
                        if (ImGui::SmallButton("Remove this form")) removedForm = formIndex;
                        ImGui::SetItemTooltip("Only the last form can be removed (forms are numbered in order)");
                    }
                    draw_form(ctx, uid, forms[i], formIndex);
                    ImGui::PopID();
                    ImGui::EndTabItem();
                }
            }

            if (ImGui::TabItemButton("+", ImGuiTabItemFlags_Trailing | ImGuiTabItemFlags_NoTooltip))
            {
                // New evolution: a copy of the last form, starting after its max level
                ordered_json form = forms.empty() ? ordered_json::object() : forms.back();
                const int index = static_cast<int>(forms.size());
                const int previousMax = forms.empty() ? -1 : read_int(forms.back(), "maxLevel", 0);
                form["form"] = index;
                form["minLevel"] = previousMax + 1;
                form["maxLevel"] = previousMax + 20;
                form["name"] = read_string(form, "name", "Fish") + " (evolved)";
                forms.push_back(std::move(form));
                ctx.commit(DataFile::UNITS);
                s_formToSelect = index;
                ctx.notify("Form " + std::to_string(index + 1) + " added: give it its own sprite sheet and icon");
            }
            ImGui::SetItemTooltip("Add a form (evolution)");
            ImGui::EndTabBar();
        }

        if (removedForm >= 0)
        {
            forms.erase(static_cast<size_t>(removedForm));
            ctx.selection.form = std::max(0, removedForm - 1);
            ctx.commit(DataFile::UNITS);
        }

        ImGui::PopID();
    }
}
