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
    const DataFile UNITS_FILE = DataFile::UNITS;

    int s_previewLevel = 1;

    // ==================
    // Deploy icon (icon_<UID>_<form>.png, not stored in the JSON)
    // ==================
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

    // ==================
    // Forms: one chip per form (its icon), "+" adds an evolution
    // ==================
    void form_chips(EditorContext& ctx, int uid, ordered_json& forms, int& removedForm)
    {
        const ImVec2 chipSize(96.f, 84.f);
        for (size_t i = 0; i < forms.size(); ++i)
        {
            const int formIndex = static_cast<int>(i);
            ImGui::PushID(formIndex);
            FrameUV uv;
            const sf::Texture* texture = unit_texture(ctx, uid, formIndex, uv);
            if (tile("##form", texture, uv, "Form " + std::to_string(i + 1), ctx.selection.form == formIndex, chipSize))
                ctx.selection.form = formIndex;
            ImGui::SetItemTooltip("%s", read_string(forms[i], "name", "?").c_str());
            ImGui::PopID();
            ImGui::SameLine();
        }

        // New evolution: a copy of the last form, starting after its max level
        if (tile("##addForm", nullptr, {}, "+ Form", false, ImVec2(70.f, chipSize.y)))
        {
            ordered_json form = forms.empty() ? ordered_json::object() : forms.back();
            const int index = static_cast<int>(forms.size());
            const int previousMax = forms.empty() ? -1 : read_int(forms.back(), "maxLevel", 0);
            form["form"] = index;
            form["minLevel"] = previousMax + 1;
            form["maxLevel"] = previousMax + 20;
            form["name"] = read_string(form, "name", "Fish") + " (evolved)";
            forms.push_back(std::move(form));
            ctx.commit(UNITS_FILE);
            ctx.selection.form = index;
            ctx.notify("Form " + std::to_string(index + 1) + " added: give it its own sprite sheet and icon (Appearance)");
        }
        ImGui::SetItemTooltip("Add a form (evolution), copied from the last one");

        // Only the last form can go (forms are numbered in order)
        if (forms.size() > 1 && ctx.selection.form == static_cast<int>(forms.size()) - 1)
        {
            ImGui::SameLine();
            if (ImGui::Button(("Remove form " + std::to_string(forms.size())).c_str())) removedForm = ctx.selection.form;
        }
    }

    // ==================
    // Tabs
    // ==================
    void general_tab(EditorContext& ctx, int uid, ordered_json& form)
    {
        if (begin_properties("General"))
        {
            text_field(ctx, UNITS_FILE, "Name", form, "name");
            text_field(ctx, UNITS_FILE, "Description", form, "description", "Shown in the equip and upgrade menus.", true);
            end_properties();
        }

        section("Unlocked by");
        const auto uses = ProjectQueries::uses_of_unit(ctx.project, uid);
        if (uses.empty()) ImGui::TextDisabled("No stage rewards this unit.");
        for (size_t i = 0; i < uses.size(); ++i)
        {
            ImGui::PushID(static_cast<int>(i));
            if (ImGui::TextLink(uses[i].label.c_str())) ctx.defer([&ctx, target = uses[i].target]() { ctx.select(target); });
            ImGui::PopID();
        }
    }

    void combat_tab(EditorContext& ctx, ordered_json& form)
    {
        section("Stats");
        if (begin_properties("Stats"))
        {
            EntityFields::stats(ctx, UNITS_FILE, form);
            end_properties();
        }

        section("Attack");
        if (begin_properties("Attack"))
        {
            EntityFields::attack(ctx, UNITS_FILE, form);
            end_properties();
        }

        section("Abilities");
        abilities_field(ctx, UNITS_FILE, form, false);
    }

    void deployment_tab(EditorContext& ctx, ordered_json& unit, ordered_json& form)
    {
        section("In battle");
        if (begin_properties("Deploy"))
        {
            int_field(ctx, UNITS_FILE, "Deploy cost", form, "cost", 0, 1000000, "Cash spent in battle to deploy it.");
            number_field(ctx, UNITS_FILE, "Cooldown", form, "cooldown", 0.f, 600.f, "Seconds before it can be deployed again.", 0.05f, "%.2f s");
            end_properties();
        }

        section("Levels");
        if (begin_properties("Levels"))
        {
            int_field(ctx, UNITS_FILE, "Min level", form, "minLevel", 0, 999, "The unit uses this form from this level...", 0.1f);
            int_field(ctx, UNITS_FILE, "Max level", form, "maxLevel", 0, 999, "...to this one. The next form starts at max level + 1.", 0.1f);
            int_field(ctx, UNITS_FILE, "Upgrade base cost", unit, "baseUpgradeCost", 0, 100000000,
                "Shells for the first upgrade, later upgrades cost more. Shared by every form.");
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

    void appearance_tab(EditorContext& ctx, int uid, ordered_json& form, int formIndex)
    {
        if (begin_properties("Appearance"))
        {
            icon_field(ctx, uid, formIndex, form);
            EntityFields::sprite(ctx, UNITS_FILE, form, ImageFolder::UNIT_SHEETS);
            end_properties();
        }
    }
}

namespace UnitEditor
{
    void draw(EditorContext& ctx)
    {
        const int uid = ctx.selection.uid;
        ordered_json* unit = ctx.project.find_uid(UNITS_FILE, uid);
        if (!unit)
        {
            ImGui::TextDisabled("Unit %d doesn't exist anymore.", uid);
            return;
        }
        if (!unit->contains("forms") || !(*unit)["forms"].is_array() || (*unit)["forms"].empty())
        {
            ImGui::TextColored(ImVec4(1.f, 0.45f, 0.4f, 1.f), "Unit %d has no form.", uid);
            return;
        }
        ordered_json& forms = (*unit)["forms"];
        ctx.selection.form = std::clamp(ctx.selection.form, 0, static_cast<int>(forms.size()) - 1);
        const int formIndex = ctx.selection.form;

        // ----- Header: the form shown, and every form as a chip -----
        ImGui::PushID(uid);
        ImGui::Text("%s", ProjectQueries::unit_name(ctx.project, uid, formIndex).c_str());
        ImGui::SameLine();
        ImGui::TextDisabled("Unit #%d, form %d of %zu   units.json", uid, formIndex + 1, forms.size());

        int removedForm = -1;
        form_chips(ctx, uid, forms, removedForm);
        ImGui::PopID();
        ImGui::Spacing();

        // ----- Categories (the tab bar is shared with enemies: the open tab stays when switching) -----
        ordered_json& form = forms[formIndex];
        if (ImGui::BeginTabBar("EntityCategories"))
        {
            // Tab names keep the same IDs for every unit; the content is scoped to this unit and form
            auto tab = [&](const char* name, auto drawContent)
                {
                    if (!ImGui::BeginTabItem(name)) return;
                    ImGui::PushID(uid);
                    ImGui::PushID(formIndex);
                    drawContent();
                    ImGui::PopID();
                    ImGui::PopID();
                    ImGui::EndTabItem();
                };
            tab("General", [&]() { general_tab(ctx, uid, form); });
            tab("Combat", [&]() { combat_tab(ctx, form); });
            tab("Deployment", [&]() { deployment_tab(ctx, *unit, form); });
            tab("Appearance", [&]() { appearance_tab(ctx, uid, form, formIndex); });
            ImGui::EndTabBar();
        }

        if (removedForm >= 0)
        {
            forms.erase(static_cast<size_t>(removedForm));
            ctx.selection.form = std::max(0, removedForm - 1);
            ctx.commit(UNITS_FILE);
        }
    }
}
