#include "EditorWidgets.h"

#include "ProjectQueries.h"

#include "../data/AbilityData.h"
#include "../data/EntityType.h"
#include "../core/combat/DamageCalculator.h"
#include "../core/combat/AbilityGlossary.h"
#include "../core/entities/status/StatusEffect.h"
#include "../data/BattlefieldLayout.h"

#include "imgui_stdlib.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace
{
    const ImVec4 MISSING_COLOR(1.f, 0.45f, 0.4f, 1.f);

    // Player-facing names of the abilities (the JSON name is shown in the tooltip)
    const char* ability_label(AbilityType type)
    {
        switch (type)
        {
        case AbilityType::STRONG_AGAINST:    return "Strong against";
        case AbilityType::RESISTANT_AGAINST: return "Resistant to";
        case AbilityType::PARALYZE_ATTACK:   return "Paralyze on hit";
        case AbilityType::SLOW_ATTACK:       return "Slow on hit";
        case AbilityType::WEAKEN_ATTACK:     return "Weaken on hit";
        default:                             return "None";
        }
    }

    std::string ability_help(AbilityType type)
    {
        char text[160];
        switch (type)
        {
        case AbilityType::STRONG_AGAINST:
            std::snprintf(text, sizeof(text), "Deals x%.1f damage to the target types.", DamageCalculator::STRONG_AGAINST_MULTIPLIER);
            return text;
        case AbilityType::RESISTANT_AGAINST:
            std::snprintf(text, sizeof(text), "Takes x%.1f damage from the target types.", DamageCalculator::RESISTANT_MULTIPLIER);
            return text;
        case AbilityType::PARALYZE_ATTACK:  return "Hits can paralyze the target: it stops until the status ends (it can still be knocked back).";
        case AbilityType::SLOW_ATTACK:      return "Hits can slow the target. (The effect isn't implemented in the game yet.)";
        case AbilityType::WEAKEN_ATTACK:    return "Hits can weaken the target: its attack is multiplied by the value.";
        default:                            return "";
        }
    }

    // statuses.json key of the status an on-hit ability inflicts (nullptr for passive abilities)
    const char* inflicted_status(AbilityType type)
    {
        switch (type)
        {
        case AbilityType::PARALYZE_ATTACK: return to_string(StatusType::PARALYZED);
        case AbilityType::SLOW_ATTACK:     return to_string(StatusType::SLOW);
        case AbilityType::WEAKEN_ATTACK:   return to_string(StatusType::WEAKEN);
        default:                           return nullptr;
        }
    }

    AbilityType ability_from_name(const std::string& name)
    {
        for (int i = static_cast<int>(AbilityType::NONE) + 1; i < static_cast<int>(AbilityType::COUNT); ++i)
        {
            if (name == to_string(static_cast<AbilityType>(i))) return static_cast<AbilityType>(i);
        }
        return AbilityType::NONE;
    }

    // Type numbers of a "types" / "targetTypes" list, without the -1 placeholders
    std::vector<int> read_type_list(const ordered_json& object, const char* key)
    {
        std::vector<int> types;
        auto it = object.find(key);
        if (it == object.end() || !it->is_array()) return types;
        for (const auto& value : *it)
        {
            if (value.is_number_integer() && value.get<int>() >= 0) types.push_back(value.get<int>());
        }
        return types;
    }

    // One checkbox per enemy type, after its icon. True when the list changed
    bool enemy_type_checkboxes(EditorContext& ctx, std::vector<int>& types)
    {
        bool changed = false;
        for (EntityType enemyType : ENEMY_TYPES)
        {
            const int type = static_cast<int>(enemyType);
            bool hasType = std::find(types.begin(), types.end(), type) != types.end();
            ImGui::PushID(type);
            EditorWidgets::FrameUV uv;
            if (const sf::Texture* icon = EditorWidgets::type_icon(ctx, enemyType, uv))
            {
                EditorWidgets::image_in_box(icon, ImVec2(ImGui::GetFrameHeight(), ImGui::GetFrameHeight()), uv);
                ImGui::SameLine(0.f, 4.f);
            }
            if (ImGui::Checkbox(to_string(static_cast<EntityType>(type)), &hasType))
            {
                if (hasType) types.push_back(type);
                else types.erase(std::remove(types.begin(), types.end(), type), types.end());
                std::sort(types.begin(), types.end());
                changed = true;
            }
            ImGui::PopID();
            ImGui::SameLine();
        }
        ImGui::NewLine();
        return changed;
    }

    // Commits when the last widget finished being edited
    void commit_when_done(EditorContext& ctx, DataFile file)
    {
        if (ImGui::IsItemDeactivatedAfterEdit()) ctx.commit(file);
    }
}

namespace EditorWidgets
{
    // ==================
    // JSON values
    // ==================
    int read_int(const ordered_json& object, const char* key, int fallback)
    {
        auto it = object.find(key);
        if (it == object.end() || !it->is_number()) return fallback;
        return static_cast<int>(std::lround(it->get<double>()));
    }

    double read_number(const ordered_json& object, const char* key, double fallback)
    {
        auto it = object.find(key);
        return (it != object.end() && it->is_number()) ? it->get<double>() : fallback;
    }

    std::string read_string(const ordered_json& object, const char* key, const std::string& fallback)
    {
        auto it = object.find(key);
        return (it != object.end() && it->is_string()) ? it->get<std::string>() : fallback;
    }

    ordered_json number_value(double value)
    {
        // Floats come from ImGui as 0.30000001: rounding keeps the files readable
        const double rounded = std::round(value * 10000.0) / 10000.0;
        if (rounded == std::floor(rounded) && std::abs(rounded) < 1e12) return static_cast<long long>(rounded);
        return rounded;
    }

    // ==================
    // Layout
    // ==================
    bool begin_properties(const char* id)
    {
        if (!ImGui::BeginTable(id, 2, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_PadOuterX)) return false;
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed, 190.f);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
        return true;
    }

    void end_properties()
    {
        ImGui::EndTable();
    }

    void property_label(const char* label, const char* help)
    {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(label);
        if (help)
        {
            ImGui::SameLine();
            help_marker(help);
        }
        ImGui::TableSetColumnIndex(1);
        ImGui::SetNextItemWidth(-FLT_MIN);
    }

    void section(const char* title)
    {
        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 0.62f, 0.45f, 1.f));
        ImGui::SeparatorText(title);
        ImGui::PopStyleColor();
    }

    void help_marker(const char* text)
    {
        ImGui::TextDisabled("(?)");
        if (ImGui::BeginItemTooltip())
        {
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 28.f);
            ImGui::TextUnformatted(text);
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
    }

    void badge(const char* text, ImU32 color)
    {
        const ImVec2 padding(6.f, 1.f);
        const ImVec2 textSize = ImGui::CalcTextSize(text);
        const ImVec2 min = ImGui::GetCursorScreenPos();
        const ImVec2 max(min.x + textSize.x + padding.x * 2, min.y + textSize.y + padding.y * 2);

        ImDrawList* drawList = ImGui::GetWindowDrawList();
        drawList->AddRectFilled(min, max, color, 6.f);
        drawList->AddText(ImVec2(min.x + padding.x, min.y + padding.y), IM_COL32_WHITE, text);
        ImGui::Dummy(ImVec2(max.x - min.x, max.y - min.y));
    }

    // ==================
    // Property fields
    // ==================
    bool int_field(EditorContext& ctx, DataFile file, const char* label, ordered_json& object, const char* key,
        int min, int max, const char* help, float speed, const char* format)
    {
        property_label(label, help);
        ImGui::PushID(key);
        int value = read_int(object, key);
        const bool changed = ImGui::DragInt("##value", &value, speed, min, max, format, ImGuiSliderFlags_AlwaysClamp);
        if (changed) object[key] = value;
        commit_when_done(ctx, file);
        ImGui::PopID();
        return changed;
    }

    bool number_field(EditorContext& ctx, DataFile file, const char* label, ordered_json& object, const char* key,
        float min, float max, const char* help, float speed, const char* format)
    {
        property_label(label, help);
        ImGui::PushID(key);
        float value = static_cast<float>(read_number(object, key));
        const bool changed = ImGui::DragFloat("##value", &value, speed, min, max, format, ImGuiSliderFlags_AlwaysClamp);
        if (changed) object[key] = number_value(value);
        commit_when_done(ctx, file);
        ImGui::PopID();
        return changed;
    }

    bool text_field(EditorContext& ctx, DataFile file, const char* label, ordered_json& object, const char* key,
        const char* help, bool isMultiline)
    {
        property_label(label, help);
        ImGui::PushID(key);
        std::string value = read_string(object, key);
        const bool changed = isMultiline
            ? ImGui::InputTextMultiline("##value", &value, ImVec2(-FLT_MIN, ImGui::GetTextLineHeight() * 4.5f))
            : ImGui::InputText("##value", &value);
        if (changed) object[key] = value;
        commit_when_done(ctx, file);
        ImGui::PopID();
        return changed;
    }

    bool flag_field(EditorContext& ctx, DataFile file, const char* label, ordered_json& object, const char* key, const char* help)
    {
        property_label(label, help);
        ImGui::PushID(key);
        bool value = object.contains(key) && (object[key].is_boolean() ? object[key].get<bool>() : read_int(object, key) != 0);
        const bool changed = ImGui::Checkbox("##value", &value);
        if (changed)
        {
            object[key] = value ? 1 : 0;
            ctx.commit(file);
        }
        ImGui::PopID();
        return changed;
    }

    bool attack_type_field(EditorContext& ctx, DataFile file, ordered_json& object)
    {
        property_label("Attack type", "Single: hits the first target in the damage zone. Area: hits everything in it.");
        ImGui::PushID("attackType");
        int type = read_int(object, "attackType", 1);
        const char* names[] = { "Single target", "Area" };
        int index = std::clamp(type - 1, 0, 1);
        const bool changed = ImGui::Combo("##value", &index, names, 2);
        if (changed)
        {
            object["attackType"] = index + 1;
            ctx.commit(file);
        }
        ImGui::PopID();
        return changed;
    }

    bool pair_field(EditorContext& ctx, DataFile file, const char* label, ordered_json& object, const char* key,
        float defaultFirst, float defaultSecond, const char* help)
    {
        property_label(label, help);
        ImGui::PushID(key);
        float values[2] = { defaultFirst, defaultSecond };
        auto it = object.find(key);
        if (it != object.end() && it->is_array() && it->size() == 2 && (*it)[0].is_number() && (*it)[1].is_number())
        {
            values[0] = (*it)[0].get<float>();
            values[1] = (*it)[1].get<float>();
        }
        const bool changed = ImGui::DragFloat2("##value", values, 0.5f, -10000.f, 10000.f, "%.1f");
        if (changed) object[key] = ordered_json::array({ number_value(values[0]), number_value(values[1]) });
        commit_when_done(ctx, file);
        ImGui::PopID();
        return changed;
    }

    bool scale_field(EditorContext& ctx, DataFile file, const char* label, ordered_json& object, const char* key,
        float min, float max, const char* help)
    {
        return optional_number_field(ctx, file, label, object, key, 1.f, min, max, 100.f, "%.0f %%", 1.f, help);
    }

    bool optional_number_field(EditorContext& ctx, DataFile file, const char* label, ordered_json& object, const char* key,
        float defaultValue, float min, float max, float displayFactor, const char* format, float speed, const char* help)
    {
        property_label(label, help);
        ImGui::PushID(key);

        const bool isSet = object.contains(key);
        float shown = static_cast<float>(read_number(object, key, defaultValue)) * displayFactor;
        const float resetWidth = ImGui::CalcTextSize("Reset").x + ImGui::GetStyle().FramePadding.x * 2.f;
        ImGui::SetNextItemWidth(std::max(60.f, ImGui::GetContentRegionAvail().x - resetWidth - ImGui::GetStyle().ItemSpacing.x));
        bool changed = ImGui::DragFloat("##value", &shown, speed, min * displayFactor, max * displayFactor, format, ImGuiSliderFlags_AlwaysClamp);
        if (changed)
        {
            // The default value isn't written: no key in the JSON
            const float value = shown / displayFactor;
            if (std::abs(value - defaultValue) < 0.005f) object.erase(key);
            else object[key] = number_value(value);
        }
        commit_when_done(ctx, file);

        ImGui::SameLine();
        ImGui::BeginDisabled(!isSet);
        if (ImGui::Button("Reset"))
        {
            object.erase(key);
            ctx.commit(file);
            changed = true;
        }
        ImGui::EndDisabled();

        ImGui::PopID();
        return changed;
    }

    bool image_field(EditorContext& ctx, DataFile file, const char* label, ordered_json& object, const char* key,
        ImageFolder folder, const char* help)
    {
        property_label(label, help);
        ImGui::PushID(key);

        std::string path = read_string(object, key);
        const sf::Texture* texture = ctx.textures.get(path);
        bool changed = false;

        image_in_box(texture, ImVec2(112.f, 72.f), {}, true);
        ImGui::SameLine();
        ImGui::BeginGroup();
        {
            const std::string fileName = path.substr(path.find_last_of('/') + 1);
            if (texture)
            {
                ImGui::TextUnformatted(fileName.c_str());
                ImGui::TextDisabled("%u x %u", texture->getSize().x, texture->getSize().y);
            }
            else
            {
                ImGui::TextColored(MISSING_COLOR, path.empty() ? "No image" : "Missing image");
                ImGui::TextDisabled("%s", path.c_str());
            }

            if (ImGui::Button("Change...")) ImGui::OpenPopup("ImagePicker");
            ImGui::SetItemTooltip("Pick one of the %s", AssetLibrary::display_name(folder));
            ImGui::SameLine();
            const ImGuiID importId = ImGui::GetID("Import");
            if (ImGui::Button("Import...")) ctx.request_png_dialog(importId, std::string("Import an image into ") + AssetLibrary::folder(folder));
            ImGui::SetItemTooltip("Copies a .png from your computer into %s", AssetLibrary::folder(folder));

            if (auto picked = ctx.take_dialog_result(importId))
            {
                std::string error;
                if (auto imported = ctx.assets.import(folder, *picked, error))
                {
                    ctx.textures.forget(*imported);
                    object[key] = *imported;
                    ctx.commit(file);
                    ctx.notify("Imported " + *imported);
                    changed = true;
                }
                else ctx.notify(error, true);
            }
        }
        ImGui::EndGroup();

        if (image_picker(ctx, "ImagePicker", folder, path))
        {
            object[key] = path;
            ctx.commit(file);
            changed = true;
        }

        ImGui::PopID();
        return changed;
    }

    // ==================
    // Images
    // ==================
    ImTextureID texture_id(const sf::Texture& texture)
    {
        return static_cast<ImTextureID>(texture.getNativeHandle());
    }

    ImVec2 fit(ImVec2 source, ImVec2 box)
    {
        if (source.x <= 0.f || source.y <= 0.f) return box;
        const float scale = std::min(box.x / source.x, box.y / source.y);
        return ImVec2(source.x * scale, source.y * scale);
    }

    FrameUV frame_uv(const sf::Texture& texture, int frame, int frameCount)
    {
        const float textureWidth = static_cast<float>(texture.getSize().x);
        const int count = std::max(1, frameCount);
        const float frameWidth = static_cast<float>(static_cast<int>(texture.getSize().x) / count); // Integer division, like the game

        FrameUV uv;
        uv.uv0 = ImVec2(frameWidth * frame / textureWidth, 0.f);
        uv.uv1 = ImVec2(frameWidth * (frame + 1) / textureWidth, 1.f);
        uv.size = ImVec2(frameWidth, static_cast<float>(texture.getSize().y));
        return uv;
    }

    void draw_image_in_rect(ImDrawList* drawList, const sf::Texture* texture, ImVec2 min, ImVec2 max, FrameUV uv)
    {
        const ImVec2 box(max.x - min.x, max.y - min.y);
        if (!texture)
        {
            // Crossed box: the image is missing
            const ImU32 color = IM_COL32(255, 115, 100, 160);
            drawList->AddRect(min, max, color, 4.f);
            drawList->AddLine(min, max, color);
            drawList->AddLine(ImVec2(min.x, max.y), ImVec2(max.x, min.y), color);
            return;
        }

        const ImVec2 source = (uv.size.x > 1.f || uv.size.y > 1.f) ? uv.size
            : ImVec2(static_cast<float>(texture->getSize().x), static_cast<float>(texture->getSize().y));
        const ImVec2 size = fit(source, box);
        const ImVec2 imageMin(min.x + (box.x - size.x) / 2.f, min.y + (box.y - size.y) / 2.f);
        drawList->AddImage(texture_id(*texture), imageMin, ImVec2(imageMin.x + size.x, imageMin.y + size.y), uv.uv0, uv.uv1);
    }

    void image_in_box(const sf::Texture* texture, ImVec2 box, FrameUV uv, bool hasBorder)
    {
        const ImVec2 min = ImGui::GetCursorScreenPos();
        const ImVec2 max(min.x + box.x, min.y + box.y);
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        if (hasBorder) drawList->AddRectFilled(min, max, IM_COL32(8, 16, 20, 255), 4.f);
        draw_image_in_rect(drawList, texture, min, max, uv);
        ImGui::Dummy(box);
    }

    void frame_in_box(const sf::Texture* texture, int frame, int frameCount, ImVec2 box)
    {
        image_in_box(texture, box, texture ? frame_uv(*texture, frame, frameCount) : FrameUV{});
    }

    bool tile(const char* id, const sf::Texture* texture, FrameUV uv, const std::string& caption, bool isSelected, ImVec2 size)
    {
        const ImVec2 min = ImGui::GetCursorScreenPos();
        const bool clicked = ImGui::InvisibleButton(id, size);
        const ImVec2 max(min.x + size.x, min.y + size.y);

        ImDrawList* drawList = ImGui::GetWindowDrawList();
        const ImU32 background = isSelected ? ImGui::GetColorU32(ImGuiCol_HeaderActive)
            : ImGui::IsItemHovered() ? ImGui::GetColorU32(ImGuiCol_HeaderHovered) : IM_COL32(8, 16, 20, 255);
        drawList->AddRectFilled(min, max, background, 6.f);

        const float captionHeight = ImGui::GetTextLineHeight() + 4.f;
        draw_image_in_rect(drawList, texture, ImVec2(min.x + 4.f, min.y + 4.f), ImVec2(max.x - 4.f, max.y - captionHeight), uv);

        const ImVec2 textSize = ImGui::CalcTextSize(caption.c_str());
        const ImVec2 textPosition(min.x + std::max(4.f, (size.x - textSize.x) / 2.f), max.y - captionHeight + 1.f);
        drawList->PushClipRect(ImVec2(min.x + 2.f, min.y), ImVec2(max.x - 2.f, max.y), true);
        drawList->AddText(textPosition, IM_COL32_WHITE, caption.c_str());
        drawList->PopClipRect();

        if (textSize.x > size.x - 8.f) ImGui::SetItemTooltip("%s", caption.c_str());
        return clicked;
    }

    // ==================
    // Project images
    // ==================
    const sf::Texture* unit_texture(EditorContext& ctx, int uid, int form, FrameUV& uv)
    {
        uv = {};
        if (const sf::Texture* icon = ctx.textures.get(AssetLibrary::unit_icon_path(uid, form))) return icon;

        // No icon yet: the first frame of the sprite sheet
        const ordered_json* unit = ctx.project.find_uid(DataFile::UNITS, uid);
        if (!unit || !unit->contains("forms") || !(*unit)["forms"].is_array() || (*unit)["forms"].size() <= static_cast<size_t>(form)) return nullptr;

        const ordered_json& formData = (*unit)["forms"][form];
        const sf::Texture* sheet = ctx.textures.get(read_string(formData, "texture"));
        if (sheet) uv = frame_uv(*sheet, 0, read_int(formData, "frameCount", 1));
        return sheet;
    }

    const sf::Texture* enemy_texture(EditorContext& ctx, int uid, FrameUV& uv)
    {
        uv = {};
        const ordered_json* enemy = ctx.project.find_uid(DataFile::ENEMIES, uid);
        if (!enemy) return nullptr;

        const sf::Texture* sheet = ctx.textures.get(read_string(*enemy, "texture"));
        if (sheet) uv = frame_uv(*sheet, 0, read_int(*enemy, "frameCount", 1));
        return sheet;
    }

    void unit_icon(EditorContext& ctx, int uid, int form, ImVec2 box)
    {
        FrameUV uv;
        const sf::Texture* texture = unit_texture(ctx, uid, form, uv);
        image_in_box(texture, box, uv);
    }

    void enemy_icon(EditorContext& ctx, int uid, ImVec2 box)
    {
        FrameUV uv;
        const sf::Texture* texture = enemy_texture(ctx, uid, uv);
        image_in_box(texture, box, uv);
    }

    // ==================
    // Status icons
    // ==================
    const sf::Texture* status_icon(EditorContext& ctx, int iconIndex, FrameUV& uv)
    {
        uv = {};
        const sf::Texture* sheet = ctx.textures.get(STATUS_ICONS_TEXTURE);
        if (!sheet || iconIndex < 0) return nullptr;

        const int columns = std::max(1, static_cast<int>(sheet->getSize().x) / STATUS_ICON_CELL_SIZE);
        const float cell = static_cast<float>(STATUS_ICON_CELL_SIZE);
        const ImVec2 min((iconIndex % columns) * cell, (iconIndex / columns) * cell);
        uv.uv0 = ImVec2(min.x / sheet->getSize().x, min.y / sheet->getSize().y);
        uv.uv1 = ImVec2((min.x + cell) / sheet->getSize().x, (min.y + cell) / sheet->getSize().y);
        uv.size = ImVec2(cell, cell);
        return sheet;
    }

    int status_icon_index(const ProjectData& project, const std::string& statusKey)
    {
        const ordered_json& statuses = project.get(DataFile::STATUSES);
        auto it = statuses.find(statusKey);
        return it != statuses.end() ? read_int(*it, "iconIndex", -1) : -1;
    }

    namespace
    {
        // A cell of a sheet of square icons, read left to right then top to bottom
        const sf::Texture* sheet_cell(EditorContext& ctx, const char* sheetPath, int index, FrameUV& uv)
        {
            uv = {};
            const sf::Texture* sheet = ctx.textures.get(sheetPath);
            if (!sheet || index < 0) return nullptr;

            const float cell = static_cast<float>(AbilityGlossary::ICON_CELL_SIZE);
            const int columns = std::max(1, static_cast<int>(sheet->getSize().x) / AbilityGlossary::ICON_CELL_SIZE);
            const ImVec2 min((index % columns) * cell, (index / columns) * cell);
            uv.uv0 = ImVec2(min.x / sheet->getSize().x, min.y / sheet->getSize().y);
            uv.uv1 = ImVec2((min.x + cell) / sheet->getSize().x, (min.y + cell) / sheet->getSize().y);
            uv.size = ImVec2(cell, cell);
            return sheet;
        }
    }

    const sf::Texture* type_icon(EditorContext& ctx, EntityType type, FrameUV& uv)
    {
        return sheet_cell(ctx, AbilityGlossary::TYPE_ICONS_TEXTURE, AbilityGlossary::type_icon_index(type), uv);
    }

    const sf::Texture* ability_icon(EditorContext& ctx, AbilityType type, FrameUV& uv)
    {
        return sheet_cell(ctx, AbilityGlossary::ABILITY_ICONS_TEXTURE, AbilityGlossary::ability_icon_index(type), uv);
    }

    int status_icon_count(EditorContext& ctx)
    {
        const sf::Texture* sheet = ctx.textures.get(STATUS_ICONS_TEXTURE);
        if (!sheet) return 0;
        return static_cast<int>(sheet->getSize().x / STATUS_ICON_CELL_SIZE) * static_cast<int>(sheet->getSize().y / STATUS_ICON_CELL_SIZE);
    }

    // ==================
    // Pickers
    // ==================
    namespace
    {
        // Lays the tiles out in rows of about 600 px (pickers are popups, they size to their content)
        void tile_grid_next(int index, float tileWidth)
        {
            const int columns = std::max(1, static_cast<int>(600.f / (tileWidth + ImGui::GetStyle().ItemSpacing.x)));
            if (index % columns != 0) ImGui::SameLine();
        }
    }

    bool enemy_picker(EditorContext& ctx, const char* popupId, int& uid)
    {
        bool picked = false;
        if (!ImGui::BeginPopup(popupId)) return false;

        ImGui::TextDisabled("Pick an enemy");
        int index = 0;
        for (int enemyUid : ctx.project.sorted_uids(DataFile::ENEMIES))
        {
            FrameUV uv;
            const sf::Texture* texture = enemy_texture(ctx, enemyUid, uv);
            tile_grid_next(index++, 100.f);
            ImGui::PushID(enemyUid);
            if (tile("##enemy", texture, uv, ProjectQueries::enemy_name(ctx.project, enemyUid), enemyUid == uid, ImVec2(100.f, 96.f)))
            {
                uid = enemyUid;
                picked = true;
                ImGui::CloseCurrentPopup();
            }
            ImGui::PopID();
        }
        ImGui::EndPopup();
        return picked;
    }

    bool unit_picker(EditorContext& ctx, const char* popupId, int& uid)
    {
        bool picked = false;
        if (!ImGui::BeginPopup(popupId)) return false;

        ImGui::TextDisabled("Pick a unit");
        int index = 0;
        for (int unitUid : ctx.project.sorted_uids(DataFile::UNITS))
        {
            FrameUV uv;
            const sf::Texture* texture = unit_texture(ctx, unitUid, 0, uv);
            tile_grid_next(index++, 100.f);
            ImGui::PushID(unitUid);
            if (tile("##unit", texture, uv, ProjectQueries::unit_name(ctx.project, unitUid), unitUid == uid, ImVec2(100.f, 96.f)))
            {
                uid = unitUid;
                picked = true;
                ImGui::CloseCurrentPopup();
            }
            ImGui::PopID();
        }
        ImGui::EndPopup();
        return picked;
    }

    bool image_picker(EditorContext& ctx, const char* popupId, ImageFolder folder, std::string& path)
    {
        bool picked = false;
        if (!ImGui::BeginPopup(popupId)) return false;

        ImGui::TextDisabled("%s  (%s)", AssetLibrary::display_name(folder), AssetLibrary::folder(folder));
        const std::vector<std::string> images = ctx.assets.list(folder); // Copy: the list can be refreshed while drawing
        if (images.empty()) ImGui::TextDisabled("No image yet: use Import...");

        int index = 0;
        for (const std::string& image : images)
        {
            tile_grid_next(index++, 128.f);
            ImGui::PushID(image.c_str());
            if (tile("##image", ctx.textures.get(image), {}, image.substr(image.find_last_of('/') + 1), image == path, ImVec2(128.f, 100.f)))
            {
                path = image;
                picked = true;
                ImGui::CloseCurrentPopup();
            }
            ImGui::PopID();
        }
        ImGui::EndPopup();
        return picked;
    }

    // ==================
    // Types and abilities
    // ==================
    void types_field(EditorContext& ctx, DataFile file, ordered_json& object)
    {
        property_label("Types", "Abilities target types: a fish strong against SHELLED deals more damage to this enemy.\n"
            "FISH is hidden: every unit has it.");
        ImGui::PushID("types");
        std::vector<int> types = read_type_list(object, "types");
        if (enemy_type_checkboxes(ctx, types))
        {
            object["types"] = types;
            ctx.commit(file);
        }
        ImGui::PopID();
    }

    void abilities_field(EditorContext& ctx, DataFile file, ordered_json& object, bool isEnemy)
    {
        ImGui::PushID("abilities");
        if (!object.contains("abilities") || !object["abilities"].is_array()) object["abilities"] = ordered_json::array();
        ordered_json& abilities = object["abilities"];

        if (abilities.empty()) ImGui::TextDisabled("No ability.");

        int removedIndex = -1;
        const ImGuiTableFlags flags = ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp;
        if (!abilities.empty() && ImGui::BeginTable("Abilities", 6, flags))
        {
            ImGui::TableSetupColumn("Ability", ImGuiTableColumnFlags_WidthStretch, 1.4f);
            ImGui::TableSetupColumn("Targets", ImGuiTableColumnFlags_WidthStretch, 1.6f);
            ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch, 0.8f);
            ImGui::TableSetupColumn("Chance %", ImGuiTableColumnFlags_WidthStretch, 0.8f);
            ImGui::TableSetupColumn("Duration (s)", ImGuiTableColumnFlags_WidthStretch, 0.8f);
            ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 28.f);
            ImGui::TableHeadersRow();

            for (size_t i = 0; i < abilities.size(); ++i)
            {
                ordered_json& ability = abilities[i];
                ImGui::PushID(static_cast<int>(i));
                ImGui::TableNextRow();

                // ----- Type, after its icon (the one the game shows) -----
                ImGui::TableSetColumnIndex(0);
                AbilityType type = ability_from_name(read_string(ability, "type"));
                {
                    FrameUV uv;
                    if (const sf::Texture* icon = ability_icon(ctx, type, uv))
                    {
                        image_in_box(icon, ImVec2(ImGui::GetFrameHeight(), ImGui::GetFrameHeight()), uv);
                        if (const char* status = inflicted_status(type)) ImGui::SetItemTooltip("Inflicts %s", status);
                        ImGui::SameLine(0.f, 4.f);
                    }
                }
                ImGui::SetNextItemWidth(-FLT_MIN);
                if (ImGui::BeginCombo("##type", ability_label(type)))
                {
                    for (int t = static_cast<int>(AbilityType::NONE) + 1; t < static_cast<int>(AbilityType::COUNT); ++t)
                    {
                        const AbilityType option = static_cast<AbilityType>(t);
                        if (ImGui::Selectable(ability_label(option), option == type) && option != type)
                        {
                            ability["type"] = to_string(option);
                            if (is_on_hit(option))
                            {
                                if (!ability.contains("chance")) ability["chance"] = 100;
                                if (!ability.contains("duration")) ability["duration"] = 3;
                                if (option != AbilityType::PARALYZE_ATTACK && !ability.contains("value")) ability["value"] = 0.5;
                            }
                            else
                            {
                                ability.erase("value");
                                ability.erase("chance");
                                ability.erase("duration");
                            }
                            ctx.commit(file);
                            type = option;
                        }
                        ImGui::SetItemTooltip("%s\n(%s in the JSON)", ability_help(option).c_str(), to_string(option));
                    }
                    ImGui::EndCombo();
                }
                ImGui::SetItemTooltip("%s", ability_help(type).c_str());

                // ----- Targets -----
                ImGui::TableSetColumnIndex(1);
                if (isEnemy)
                {
                    ImGui::AlignTextToFramePadding();
                    ImGui::TextDisabled("Every fish");
                }
                else
                {
                    std::vector<int> targets = read_type_list(ability, "targetTypes");
                    if (enemy_type_checkboxes(ctx, targets))
                    {
                        ability["targetTypes"] = targets;
                        ctx.commit(file);
                    }
                    if (targets.empty()) ImGui::TextColored(MISSING_COLOR, "Pick at least one type");
                }

                // ----- Value, chance, duration (on hit only) -----
                auto numberCell = [&](int column, const char* key, float min, float max, float speed, const char* format, const char* tooltip)
                    {
                        ImGui::TableSetColumnIndex(column);
                        ImGui::SetNextItemWidth(-FLT_MIN);
                        ImGui::PushID(key);
                        float value = static_cast<float>(read_number(ability, key));
                        if (ImGui::DragFloat("##number", &value, speed, min, max, format, ImGuiSliderFlags_AlwaysClamp))
                            ability[key] = number_value(value);
                        commit_when_done(ctx, file);
                        ImGui::SetItemTooltip("%s", tooltip);
                        ImGui::PopID();
                    };

                if (is_on_hit(type))
                {
                    if (type != AbilityType::PARALYZE_ATTACK)
                        numberCell(2, "value", 0.01f, 10.f, 0.01f, "x%.2f", "Strength of the status (WEAKEN: the target's attack is multiplied by it)");
                    numberCell(3, "chance", 0.1f, 100.f, 0.5f, "%.1f%%", "Chance for each hit to inflict the status");
                    numberCell(4, "duration", 0.1f, 600.f, 0.1f, "%.1f s", "How long the status lasts. Hits while it's active add their duration");
                }

                // ----- Remove -----
                ImGui::TableSetColumnIndex(5);
                if (ImGui::SmallButton("X")) removedIndex = static_cast<int>(i);
                ImGui::SetItemTooltip("Remove this ability");

                ImGui::PopID();
            }
            ImGui::EndTable();
        }

        if (removedIndex >= 0)
        {
            abilities.erase(static_cast<size_t>(removedIndex));
            ctx.commit(file);
        }

        if (ImGui::Button("+ Add ability"))
        {
            if (isEnemy)
                abilities.push_back({ { "type", "WEAKEN_ATTACK" }, { "value", 0.5 }, { "chance", 100 }, { "duration", 3 } });
            else
                abilities.push_back({ { "type", "STRONG_AGAINST" }, { "targetTypes", ordered_json::array({ 0 }) } });
            ctx.commit(file);
        }

        ImGui::PopID();
    }

    // ==================
    // Backgrounds
    // ==================
    const sf::Texture* background_texture(EditorContext& ctx, const std::string& backgroundPath)
    {
        return ctx.textures.get_seamless(backgroundPath, ProjectQueries::background_seamless_overlap(ctx.project, backgroundPath));
    }

    void ground_line_field(EditorContext& ctx, const std::string& backgroundPath)
    {
        const sf::Texture* background = ctx.textures.get(backgroundPath);
        if (!background)
        {
            ImGui::TextDisabled("Pick a background to place its ground line.");
            return;
        }

        const DataFile file = DataFile::BACKGROUNDS;
        ordered_json& backgrounds = ctx.project.get(file);
        const float imageHeight = static_cast<float>(background->getSize().y);
        const bool isCustom = backgrounds.contains(backgroundPath) && backgrounds[backgroundPath].contains("groundRow");
        float row = ProjectQueries::background_ground_row(ctx.project, backgroundPath, imageHeight);

        auto setRow = [&](float value) { backgrounds[backgroundPath]["groundRow"] = std::lround(value); };
        auto clearRow = [&]()
            {
                backgrounds[backgroundPath].erase("groundRow");
                if (backgrounds[backgroundPath].empty()) backgrounds.erase(backgroundPath);
            };

        ImGui::PushID(backgroundPath.c_str());

        // ----- Value -----
        if (begin_properties("Ground"))
        {
            property_label("Ground line", "The row of the background the units stand on (front layer), saved in backgrounds.json.\n"
                "Every stage using this background uses it.\nWithout it: the same place as the 1280 x 1120 template (55% of the height).");
            bool hasCustomRow = isCustom;
            if (ImGui::Checkbox("##customGround", &hasCustomRow))
            {
                if (hasCustomRow) setRow(row);
                else clearRow();
                ctx.commit(file);
            }
            ImGui::SetItemTooltip("Set the ground line of this background");
            ImGui::SameLine();
            if (hasCustomRow)
            {
                ImGui::SetNextItemWidth(-FLT_MIN);
                if (ImGui::DragFloat("##groundRow", &row, 1.f, 0.f, imageHeight, "row %.0f", ImGuiSliderFlags_AlwaysClamp)) setRow(row);
                if (ImGui::IsItemDeactivatedAfterEdit()) ctx.commit(file);
            }
            else ImGui::TextDisabled("Template ratio: row %.0f of %.0f", row, imageHeight);

            property_label("Used by");
            const std::vector<Reference> stages = ProjectQueries::stages_using_background(ctx.project, backgroundPath);
            if (stages.empty()) ImGui::TextDisabled("No stage yet");
            for (size_t i = 0; i < stages.size(); ++i)
            {
                ImGui::PushID(static_cast<int>(i));
                if (ImGui::TextLink(stages[i].label.c_str())) ctx.defer([&ctx, target = stages[i].target]() { ctx.select(target); });
                ImGui::PopID();
            }
            end_properties();
        }

        // ----- Image with the line (drag it) -----
        const ImVec2 imageSize(static_cast<float>(background->getSize().x), imageHeight);
        const ImVec2 size = fit(imageSize, ImVec2(std::min(ImGui::GetContentRegionAvail().x, 640.f), 420.f));
        const float scale = size.x / imageSize.x;
        const ImVec2 origin = ImGui::GetCursorScreenPos();
        ImGui::InvisibleButton("##groundImage", size);

        if (ImGui::IsItemActive())
        {
            row = std::clamp((ImGui::GetIO().MousePos.y - origin.y) / scale, 0.f, imageHeight);
            setRow(row);
        }
        if (ImGui::IsItemDeactivated()) ctx.commit(file);
        if (ImGui::IsItemHovered()) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
        ImGui::SetItemTooltip("Click or drag to place the ground line");

        ImDrawList* drawList = ImGui::GetWindowDrawList();
        drawList->AddImage(texture_id(*background), origin, ImVec2(origin.x + size.x, origin.y + size.y));
        auto rowY = [&](float imageRow) { return origin.y + std::clamp(imageRow, 0.f, imageHeight) * scale; };
        auto worldRow = [&](float worldY) { return row + (worldY - BATTLE_GROUND_Y) / BACKGROUND_SCALE; };

        // What the camera shows (default zoom, and zoomed out at most): world y 54-1026 and -270-1350
        auto band = [&](float worldTop, float worldBottom, ImU32 color)
            {
                drawList->AddRect(ImVec2(origin.x + 1.f, rowY(worldRow(worldTop))), ImVec2(origin.x + size.x - 1.f, rowY(worldRow(worldBottom))), color, 0.f, 0, 1.5f);
            };
        band(-270.f, 1350.f, IM_COL32(255, 255, 255, 70));
        band(54.f, 1026.f, IM_COL32(255, 215, 70, 200));

        // Layers: front (the line) to back (the bases), MAX_BATTLE_LAYER world px higher
        const float backRow = worldRow(BACK_LAYER_Y);
        drawList->AddRectFilled(ImVec2(origin.x, rowY(backRow)), ImVec2(origin.x + size.x, rowY(row)), IM_COL32(255, 80, 80, 60));
        drawList->AddLine(ImVec2(origin.x, rowY(backRow)), ImVec2(origin.x + size.x, rowY(backRow)), IM_COL32(255, 160, 115, 255), 1.5f);
        drawList->AddLine(ImVec2(origin.x, rowY(row)), ImVec2(origin.x + size.x, rowY(row)), IM_COL32(255, 80, 80, 255), 2.f);

        ImGui::TextDisabled("Red: the front layer. Orange: the last layer, where the bases stand. Units stand in between.");
        ImGui::TextDisabled("Yellow: what the camera shows at the default zoom. White: zoomed out at most.");
        ImGui::PopID();
    }
}
