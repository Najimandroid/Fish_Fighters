#include "ContentPanel.h"

#include "../ProjectQueries.h"

#include "imgui_stdlib.h"

#include <algorithm>
#include <cctype>

using namespace EditorWidgets;
namespace fs = std::filesystem;

namespace
{
    DataFile file_of(ContentKind kind)
    {
        switch (kind)
        {
        case ContentKind::UNIT:    return DataFile::UNITS;
        case ContentKind::ENEMY:   return DataFile::ENEMIES;
        case ContentKind::STAGE:   return DataFile::STAGES;
        case ContentKind::CHAPTER: return DataFile::CHAPTERS;
        default:                   return DataFile::STATUSES;
        }
    }

    const char* kind_name(ContentKind kind)
    {
        switch (kind)
        {
        case ContentKind::UNIT:    return "unit";
        case ContentKind::ENEMY:   return "enemy";
        case ContentKind::STAGE:   return "stage";
        case ContentKind::CHAPTER: return "chapter";
        default:                   return "item";
        }
    }

    std::string lower(std::string text)
    {
        std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return text;
    }

    // Same item, whatever form is shown (switching forms mustn't scroll the list)
    bool same_item(const Selection& a, const Selection& b)
    {
        return a.kind == b.kind && a.uid == b.uid && a.name == b.name;
    }

    // "UID_4", or "UID_4_2" if that key is somehow taken
    std::string free_key(const ProjectData& project, DataFile file, const std::string& key)
    {
        std::string candidate = key;
        for (int i = 2; project.get(file).contains(candidate); ++i) candidate = key + "_" + std::to_string(i);
        return candidate;
    }

    std::string key_for(const ProjectData& project, DataFile file, int uid)
    {
        // chapters.json uses "Chapter_1" keys, the other files "UID_1" (the game only reads the "UID" values)
        return free_key(project, file, file == DataFile::CHAPTERS ? "Chapter_" + std::to_string(uid) : ProjectData::uid_key(uid));
    }

    // The preferred image of the folder if it exists, else the first one
    std::string default_image(EditorContext& ctx, ImageFolder folder, const std::string& preferredName)
    {
        const std::vector<std::string>& images = ctx.assets.list(folder);
        const std::string preferred = std::string(AssetLibrary::folder(folder)) + "/" + preferredName;
        if (std::find(images.begin(), images.end(), preferred) != images.end()) return preferred;
        return images.empty() ? preferred : images.front();
    }
}

// ==================
// Panel
// ==================
void ContentPanel::draw(EditorContext& ctx)
{
    // Selected elsewhere (validation, "used by" links): open its tab and scroll to it
    if (!same_item(ctx.selection, m_lastSelection))
    {
        m_isFollowingSelection = ctx.selection.kind != ContentKind::NONE;
        m_lastSelection = ctx.selection;

        // An image selected elsewhere: show its folder
        if (ctx.selection.kind == ContentKind::IMAGE)
            if (const auto folder = AssetLibrary::folder_of(ctx.selection.name)) m_imageFolder = *folder;
    }

    ImGui::SetNextItemWidth(-FLT_MIN);
    ImGui::InputTextWithHint("##search", "Search by name or UID", &m_search);

    struct Tab { const char* label; ContentKind kind; };
    const Tab tabs[] = {
        { "Units", ContentKind::UNIT }, { "Enemies", ContentKind::ENEMY }, { "Stages", ContentKind::STAGE },
        { "Chapters", ContentKind::CHAPTER }, { "Statuses", ContentKind::STATUS }, { "Images", ContentKind::IMAGE }
    };

    if (ImGui::BeginTabBar("ContentTabs", ImGuiTabBarFlags_FittingPolicyScroll))
    {
        for (const Tab& tab : tabs)
        {
            ImGuiTabItemFlags flags = 0;
            if (m_isFollowingSelection && ctx.selection.kind == tab.kind && m_visibleTab != tab.kind) flags |= ImGuiTabItemFlags_SetSelected;

            if (!ImGui::BeginTabItem(tab.label, nullptr, flags)) continue;
            m_visibleTab = tab.kind;

            draw_toolbar(ctx, tab.kind);
            ImGui::BeginChild("List", ImVec2(0, 0), ImGuiChildFlags_None);
            switch (tab.kind)
            {
            case ContentKind::UNIT:       draw_units(ctx); break;
            case ContentKind::ENEMY:      draw_enemies(ctx); break;
            case ContentKind::STAGE:      draw_stages(ctx); break;
            case ContentKind::CHAPTER:    draw_chapters(ctx); break;
            case ContentKind::STATUS:     draw_statuses(ctx); break;
            case ContentKind::IMAGE:      draw_images(ctx); break;
            default: break;
            }
            ImGui::EndChild();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    draw_new_entity_modal(ctx);
    draw_delete_modal(ctx);
}

bool ContentPanel::row(const char* id, const sf::Texture* texture, FrameUV uv,
    const std::string& title, const std::string& subtitle, bool isSelected)
{
    const float height = 58.f;
    const ImVec2 min = ImGui::GetCursorScreenPos();
    const bool clicked = ImGui::Selectable(id, isSelected, ImGuiSelectableFlags_None, ImVec2(0.f, height));

    // Follow a selection made elsewhere
    if (isSelected && m_isFollowingSelection)
    {
        ImGui::SetScrollHereY(0.5f);
        m_isFollowingSelection = false;
    }

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    const ImVec2 iconMin(min.x + 2.f, min.y + 3.f);
    const ImVec2 iconMax(iconMin.x + 70.f, iconMin.y + height - 6.f);
    drawList->AddRectFilled(iconMin, iconMax, IM_COL32(8, 16, 20, 255), 4.f);
    draw_image_in_rect(drawList, texture, ImVec2(iconMin.x + 2.f, iconMin.y + 2.f), ImVec2(iconMax.x - 2.f, iconMax.y - 2.f), uv);

    const float textX = iconMax.x + 10.f;
    drawList->AddText(ImVec2(textX, min.y + 8.f), ImGui::GetColorU32(ImGuiCol_Text), title.c_str());
    drawList->AddText(ImVec2(textX, min.y + 8.f + ImGui::GetTextLineHeightWithSpacing()), ImGui::GetColorU32(ImGuiCol_TextDisabled), subtitle.c_str());
    return clicked;
}

bool ContentPanel::matches_search(const std::string& text, int uid) const
{
    if (m_search.empty()) return true;
    const std::string search = lower(m_search);
    return lower(text).find(search) != std::string::npos || std::to_string(uid) == m_search;
}

// ==================
// Lists
// ==================
void ContentPanel::draw_units(EditorContext& ctx)
{
    for (int uid : ctx.project.sorted_uids(DataFile::UNITS))
    {
        const std::string name = ProjectQueries::unit_name(ctx.project, uid);
        if (!matches_search(name, uid)) continue;

        const ordered_json& unit = *ctx.project.find_uid(DataFile::UNITS, uid);
        const size_t formCount = unit.contains("forms") && unit["forms"].is_array() ? unit["forms"].size() : 0;
        const int cost = formCount > 0 ? read_int(unit["forms"][0], "cost") : 0;

        FrameUV uv;
        const sf::Texture* texture = unit_texture(ctx, uid, 0, uv);
        const std::string subtitle = "#" + std::to_string(uid) + "   " + std::to_string(formCount) + (formCount == 1 ? " form" : " forms")
            + "   cost " + std::to_string(cost);

        ImGui::PushID(uid);
        const bool isSelected = ctx.selection.kind == ContentKind::UNIT && ctx.selection.uid == uid;
        if (row("##unit", texture, uv, name, subtitle, isSelected) && !isSelected)
        {
            ctx.select({ ContentKind::UNIT, uid, 0 });
            m_lastSelection = ctx.selection;
        }
        ImGui::PopID();
    }
}

void ContentPanel::draw_enemies(EditorContext& ctx)
{
    for (int uid : ctx.project.sorted_uids(DataFile::ENEMIES))
    {
        const std::string name = ProjectQueries::enemy_name(ctx.project, uid);
        if (!matches_search(name, uid)) continue;

        const ordered_json& enemy = *ctx.project.find_uid(DataFile::ENEMIES, uid);
        FrameUV uv;
        const sf::Texture* texture = enemy_texture(ctx, uid, uv);
        const std::string subtitle = "#" + std::to_string(uid) + "   " + std::to_string(read_int(enemy, "health")) + " HP   "
            + std::to_string(read_int(enemy, "attackPower")) + " ATK";

        ImGui::PushID(uid);
        const bool isSelected = ctx.selection.kind == ContentKind::ENEMY && ctx.selection.uid == uid;
        if (row("##enemy", texture, uv, name, subtitle, isSelected))
        {
            ctx.select({ ContentKind::ENEMY, uid });
            m_lastSelection = ctx.selection;
        }
        ImGui::PopID();
    }
}

void ContentPanel::draw_stages(EditorContext& ctx)
{
    for (int uid : ctx.project.sorted_uids(DataFile::STAGES))
    {
        const std::string name = ProjectQueries::stage_name(ctx.project, uid);
        if (!matches_search(name, uid)) continue;

        const ordered_json& stage = *ctx.project.find_uid(DataFile::STAGES, uid);
        const size_t waves = stage.contains("enemies") && stage["enemies"].is_array() ? stage["enemies"].size() : 0;
        const sf::Texture* texture = ctx.textures.get(read_string(stage, "baseTexture"));
        const std::string subtitle = "#" + std::to_string(uid) + "   " + std::to_string(waves) + (waves == 1 ? " wave" : " waves")
            + "   base " + std::to_string(read_int(stage, "baseHealth")) + " HP";

        ImGui::PushID(uid);
        const bool isSelected = ctx.selection.kind == ContentKind::STAGE && ctx.selection.uid == uid;
        if (row("##stage", texture, {}, name, subtitle, isSelected))
        {
            ctx.select({ ContentKind::STAGE, uid });
            m_lastSelection = ctx.selection;
        }
        ImGui::PopID();
    }
}

void ContentPanel::draw_chapters(EditorContext& ctx)
{
    for (int uid : ctx.project.sorted_uids(DataFile::CHAPTERS))
    {
        const std::string name = ProjectQueries::chapter_name(ctx.project, uid);
        if (!matches_search(name, uid)) continue;

        const ordered_json& chapter = *ctx.project.find_uid(DataFile::CHAPTERS, uid);
        const size_t stages = chapter.contains("stages") && chapter["stages"].is_array() ? chapter["stages"].size() : 0;
        const sf::Texture* texture = ctx.textures.get(read_string(chapter, "mapTexture"));
        const std::string subtitle = "#" + std::to_string(uid) + "   " + std::to_string(stages) + (stages == 1 ? " stage" : " stages");

        ImGui::PushID(uid);
        const bool isSelected = ctx.selection.kind == ContentKind::CHAPTER && ctx.selection.uid == uid;
        if (row("##chapter", texture, {}, name, subtitle, isSelected))
        {
            ctx.select({ ContentKind::CHAPTER, uid });
            m_lastSelection = ctx.selection;
        }
        ImGui::PopID();
    }
}

void ContentPanel::draw_images(EditorContext& ctx)
{
    const ImageFolder folder = m_imageFolder;
    const ContentKind kind = ContentKind::IMAGE;
    const std::vector<std::string> images = ctx.assets.list(folder);
    if (images.empty()) ImGui::TextDisabled("No image in %s", AssetLibrary::folder(folder));

    for (const std::string& path : images)
    {
        const std::string name = path.substr(path.find_last_of('/') + 1);
        if (!matches_search(name, -100000)) continue;

        const sf::Texture* texture = ctx.textures.get(path);
        const size_t uses = ProjectQueries::uses_of_image(ctx.project, path).size();
        std::string subtitle = texture ? std::to_string(texture->getSize().x) + " x " + std::to_string(texture->getSize().y) : "can't be read";
        subtitle += uses == 0 ? "   unused" : "   used " + std::to_string(uses) + (uses == 1 ? " time" : " times");

        ImGui::PushID(path.c_str());
        const bool isSelected = ctx.selection.kind == kind && ctx.selection.name == path;
        if (row("##image", texture, {}, name, subtitle, isSelected))
        {
            ctx.select({ kind, 0, 0, path });
            m_lastSelection = ctx.selection;
        }
        ImGui::PopID();
    }
}

void ContentPanel::draw_statuses(EditorContext& ctx)
{
    ImGui::TextDisabled("Every status of the game is defined here (they can't be added or removed).");
    for (const auto& [key, status] : ctx.project.get(DataFile::STATUSES).items())
    {
        const std::string name = read_string(status, "name", key);
        if (!matches_search(name + " " + key, -100000)) continue;

        const std::string subtitle = key + "   stronger: " + read_string(status, "strongest", "?") + " value";
        FrameUV uv;
        const sf::Texture* icon = status_icon(ctx, read_int(status, "iconIndex", -1), uv);

        ImGui::PushID(key.c_str());
        const bool isSelected = ctx.selection.kind == ContentKind::STATUS && ctx.selection.name == key;
        if (row("##status", icon, uv, name, subtitle, isSelected))
        {
            ctx.select({ ContentKind::STATUS, 0, 0, key });
            m_lastSelection = ctx.selection;
        }
        ImGui::PopID();
    }
}

// ==================
// Toolbar
// ==================
void ContentPanel::draw_toolbar(EditorContext& ctx, ContentKind kind)
{
    const bool isSelectedHere = ctx.selection.kind == kind && ctx.project.find_uid(file_of(kind), ctx.selection.uid);

    switch (kind)
    {
    case ContentKind::UNIT:
    case ContentKind::ENEMY:
        if (ImGui::Button("+ New from sprite sheet...")) open_new_entity(kind);
        ImGui::SetItemTooltip("Creates a %s from a .png sprite sheet (frames side by side)", kind_name(kind));
        break;

    case ContentKind::STAGE:
        if (ImGui::Button("+ New stage")) create_stage(ctx);
        break;

    case ContentKind::CHAPTER:
        if (ImGui::Button("+ New chapter")) create_chapter(ctx);
        break;

    case ContentKind::IMAGE:
    {
        // Folder picker: bases, stage backgrounds, chapter maps, menu backgrounds, sprite sheets, unit icons
        ImGui::SetNextItemWidth(std::max(120.f, ImGui::GetContentRegionAvail().x - 150.f));
        if (ImGui::BeginCombo("##folder", AssetLibrary::display_name(m_imageFolder)))
        {
            for (int i = 0; i < static_cast<int>(ImageFolder::COUNT); ++i)
            {
                const ImageFolder option = static_cast<ImageFolder>(i);
                const std::string label = std::string(AssetLibrary::display_name(option)) + "  (" + std::to_string(ctx.assets.list(option).size()) + ")";
                if (ImGui::Selectable(label.c_str(), option == m_imageFolder)) m_imageFolder = option;
                ImGui::SetItemTooltip("%s", AssetLibrary::folder(option));
            }
            ImGui::EndCombo();
        }
        ImGui::SameLine();

        const ImageFolder folder = m_imageFolder;
        const ImGuiID importId = ImGui::GetID("ImportImage");
        if (ImGui::Button("+ Import...")) ctx.request_png_dialog(importId, std::string("Import into ") + AssetLibrary::folder(folder));
        ImGui::SetItemTooltip("Copies a .png into %s", AssetLibrary::folder(folder));

        if (auto picked = ctx.take_dialog_result(importId))
        {
            std::string error;
            if (auto imported = ctx.assets.import(folder, *picked, error))
            {
                ctx.textures.forget(*imported);
                ctx.select({ kind, 0, 0, *imported });
                ctx.notify("Imported " + *imported + ". Pick it in a stage or chapter to use it.");
            }
            else ctx.notify(error, true);
        }
        return; // Images aren't deleted from the editor (the files stay on disk)
    }

    default:
        return;
    }

    ImGui::SameLine();
    ImGui::BeginDisabled(!isSelectedHere);
    if (ImGui::Button("Duplicate")) duplicate(ctx, ctx.selection);
    ImGui::SameLine();
    if (ImGui::Button("Delete"))
    {
        m_toDelete = ctx.selection;
        m_isDeleteRequested = true;
    }
    ImGui::EndDisabled();
}

void ContentPanel::create_stage(EditorContext& ctx)
{
    const int uid = ctx.project.next_free_uid(DataFile::STAGES);
    ordered_json stage = {
        { "UID", uid },
        { "stageName", "New stage" },
        { "enemiesLimit", 10 },
        { "unitsLimit", 10 },
        { "baseHealth", 1000 },
        { "length", 1600 },
        { "baseTexture", default_image(ctx, ImageFolder::BASES, "defaultBase.png") },
        { "backgroundTexture", default_image(ctx, ImageFolder::STAGE_BACKGROUNDS, "backgroundTEST.png") },
        { "numberOfDifferentEnemies", 0 },
        { "enemies", ordered_json::array() },
        { "rewards", ordered_json::array({ { { "type", "shells" }, { "amount", 100 } } }) }
    };

    ctx.project.get(DataFile::STAGES)[key_for(ctx.project, DataFile::STAGES, uid)] = std::move(stage);
    ctx.commit(DataFile::STAGES);
    ctx.select({ ContentKind::STAGE, uid });
    m_lastSelection = ctx.selection;
    ctx.notify("Created stage " + std::to_string(uid) + ". Add it to a chapter map to make it playable.");
}

void ContentPanel::create_chapter(EditorContext& ctx)
{
    const int uid = ctx.project.next_free_uid(DataFile::CHAPTERS);
    ordered_json chapter = {
        { "UID", uid },
        { "name", "New chapter" },
        { "description", "" },
        { "mapTexture", default_image(ctx, ImageFolder::MAPS, "chapter_map.png") },
        { "stages", ordered_json::array() }
    };

    ctx.project.get(DataFile::CHAPTERS)[key_for(ctx.project, DataFile::CHAPTERS, uid)] = std::move(chapter);
    ctx.commit(DataFile::CHAPTERS);
    ctx.select({ ContentKind::CHAPTER, uid });
    m_lastSelection = ctx.selection;
    ctx.notify("Created chapter " + std::to_string(uid));
}

void ContentPanel::duplicate(EditorContext& ctx, const Selection& item)
{
    const DataFile file = file_of(item.kind);
    const ordered_json* source = ctx.project.find_uid(file, item.uid);
    if (!source) return;

    ordered_json copy = *source; // Copied before inserting: the insertion can move the entries
    const int uid = ctx.project.next_free_uid(file);
    copy["UID"] = uid;

    auto rename = [](ordered_json& object, const char* key)
        {
            if (object.contains(key) && object[key].is_string()) object[key] = object[key].get<std::string>() + " (copy)";
        };

    switch (item.kind)
    {
    case ContentKind::UNIT:
        if (copy.contains("forms") && copy["forms"].is_array())
        {
            for (auto& form : copy["forms"]) rename(form, "name");

            // The deploy icons follow the UID (icon_<UID>_<form>.png)
            for (size_t form = 0; form < copy["forms"].size(); ++form)
            {
                std::error_code error;
                const fs::path from = ctx.project.resolve(AssetLibrary::unit_icon_path(item.uid, static_cast<int>(form)));
                const fs::path to = ctx.project.resolve(AssetLibrary::unit_icon_path(uid, static_cast<int>(form)));
                if (fs::exists(from) && !fs::exists(to)) fs::copy_file(from, to, error);
                ctx.textures.forget(AssetLibrary::unit_icon_path(uid, static_cast<int>(form)));
            }
            ctx.assets.refresh();
        }
        break;
    case ContentKind::ENEMY:   rename(copy, "name"); break;
    case ContentKind::STAGE:   rename(copy, "stageName"); break;
    case ContentKind::CHAPTER: rename(copy, "name"); break;
    default: break;
    }

    ctx.project.get(file)[key_for(ctx.project, file, uid)] = std::move(copy);
    ctx.commit(file);
    ctx.select({ item.kind, uid });
    m_lastSelection = ctx.selection;
    ctx.notify(std::string("Duplicated as ") + kind_name(item.kind) + " " + std::to_string(uid));
}

// ==================
// New unit / enemy
// ==================
void ContentPanel::open_new_entity(ContentKind kind)
{
    m_new = NewEntity{};
    m_new.kind = kind;
    m_new.name = kind == ContentKind::UNIT ? "New fish" : "New enemy";
    m_isNewEntityRequested = true;
}

void ContentPanel::draw_sheet_slices(const sf::Texture& sheet, int frameCount, int knockbackFrame, float width)
{
    const ImVec2 sheetSize(static_cast<float>(sheet.getSize().x), static_cast<float>(sheet.getSize().y));
    const ImVec2 size = fit(sheetSize, ImVec2(width, 180.f));
    const ImVec2 min = ImGui::GetCursorScreenPos();
    ImGui::Dummy(ImVec2(width, size.y));

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    drawList->AddRectFilled(min, ImVec2(min.x + width, min.y + size.y), IM_COL32(8, 16, 20, 255), 4.f);
    drawList->AddImage(texture_id(sheet), min, ImVec2(min.x + size.x, min.y + size.y));

    // Colour of each frame: walk / knockback / attack, like the game reads them
    const int count = std::max(1, frameCount);
    const float frameWidth = static_cast<float>(static_cast<int>(sheet.getSize().x) / count) * (size.x / sheetSize.x);
    for (int frame = 0; frame < count; ++frame)
    {
        const ImVec2 frameMin(min.x + frame * frameWidth, min.y);
        const ImVec2 frameMax(frameMin.x + frameWidth, min.y + size.y);
        const ImU32 color = frame < knockbackFrame ? IM_COL32(40, 170, 190, 50)
            : frame == knockbackFrame ? IM_COL32(255, 160, 115, 90) : IM_COL32(230, 70, 80, 60);
        drawList->AddRectFilled(frameMin, frameMax, color);
        drawList->AddRect(frameMin, frameMax, IM_COL32(255, 255, 255, 90));
        if (frameWidth > 18.f) drawList->AddText(ImVec2(frameMin.x + 3.f, frameMin.y + 2.f), IM_COL32_WHITE, std::to_string(frame).c_str());
    }
}

void ContentPanel::draw_new_entity_modal(EditorContext& ctx)
{
    if (m_isNewEntityRequested)
    {
        ImGui::OpenPopup("###NewEntity");
        m_isNewEntityRequested = false;
    }

    const bool isUnit = m_new.kind == ContentKind::UNIT;
    const char* title = isUnit ? "New unit###NewEntity" : "New enemy###NewEntity";
    if (!ImGui::BeginPopupModal(title, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) return;

    const float width = 720.f;

    ImGui::SeparatorText("Name");
    ImGui::SetNextItemWidth(width);
    ImGui::InputText("##name", &m_new.name);

    // ----- Sprite sheet -----
    ImGui::SeparatorText("Sprite sheet");
    const ImGuiID sheetDialog = ImGui::GetID("PickSheet");
    if (ImGui::Button("Pick a .png...")) ctx.request_png_dialog(sheetDialog, "Pick the sprite sheet");
    ImGui::SameLine();
    ImGui::TextDisabled("%s", m_new.sheetPath.empty() ? "Frames side by side, idle first" : m_new.sheetPath.filename().string().c_str());

    if (auto picked = ctx.take_dialog_result(sheetDialog))
    {
        m_new.sheetPath = *picked;
        m_new.isSheetLoaded = m_new.sheet.loadFromFile(*picked);
        m_new.error = m_new.isSheetLoaded ? "" : "Can't read this image";
        if (m_new.isSheetLoaded)
        {
            // Square frames as a first guess, the slices below show if it's right
            const sf::Vector2u size = m_new.sheet.getSize();
            m_new.frameCount = std::max(1, static_cast<int>(size.x / std::max(1u, size.y)));
            m_new.knockbackFrame = std::clamp(m_new.frameCount / 2, 1, std::max(1, m_new.frameCount - 1));
        }
    }

    if (m_new.isSheetLoaded)
    {
        draw_sheet_slices(m_new.sheet, m_new.frameCount, m_new.knockbackFrame, width);

        ImGui::SetNextItemWidth(200.f);
        if (ImGui::DragInt("Frames", &m_new.frameCount, 0.1f, 1, 256, "%d", ImGuiSliderFlags_AlwaysClamp))
            m_new.knockbackFrame = std::clamp(m_new.knockbackFrame, 1, std::max(1, m_new.frameCount - 1));
        ImGui::SameLine(0.f, 30.f);
        ImGui::SetNextItemWidth(200.f);
        ImGui::DragInt("Knockback frame", &m_new.knockbackFrame, 0.1f, 1, std::max(1, m_new.frameCount - 1), "%d", ImGuiSliderFlags_AlwaysClamp);

        ImGui::TextColored(ImVec4(0.35f, 0.75f, 0.85f, 1.f), "Walk: 0-%d", m_new.knockbackFrame - 1);
        ImGui::SameLine(0.f, 20.f);
        ImGui::TextColored(ImVec4(1.f, 0.63f, 0.45f, 1.f), "Knockback: %d", m_new.knockbackFrame);
        ImGui::SameLine(0.f, 20.f);
        if (m_new.knockbackFrame < m_new.frameCount - 1)
            ImGui::TextColored(ImVec4(0.95f, 0.4f, 0.45f, 1.f), "Attack: %d-%d", m_new.knockbackFrame + 1, m_new.frameCount - 1);
        else
            ImGui::TextDisabled("No attack frame");
    }

    // ----- Deploy icon (units) -----
    if (isUnit)
    {
        ImGui::SeparatorText("Deploy icon");
        const ImGuiID iconDialog = ImGui::GetID("PickIcon");
        if (auto picked = ctx.take_dialog_result(iconDialog)) m_new.iconPath = *picked;

        if (m_new.iconPath.empty())
        {
            ImGui::Checkbox("Make it from the first frame", &m_new.isMakingIcon);
            ImGui::SameLine();
        }
        else
        {
            ImGui::Text("Icon: %s", m_new.iconPath.filename().string().c_str());
            ImGui::SameLine();
            if (ImGui::SmallButton("Clear")) m_new.iconPath.clear();
            ImGui::SameLine();
        }
        if (ImGui::Button("Use an icon file...")) ctx.request_png_dialog(iconDialog, "Pick the deploy icon (200 x 150)");
    }

    if (!m_new.error.empty()) ImGui::TextColored(ImVec4(1.f, 0.45f, 0.4f, 1.f), "%s", m_new.error.c_str());

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::BeginDisabled(!m_new.isSheetLoaded || m_new.name.empty());
    if (ImGui::Button("Create", ImVec2(140.f, 0.f)) && create_entity(ctx)) ImGui::CloseCurrentPopup();
    ImGui::EndDisabled();
    ImGui::SameLine();
    if (ImGui::Button("Cancel", ImVec2(140.f, 0.f))) ImGui::CloseCurrentPopup();

    ImGui::EndPopup();
}

bool ContentPanel::create_entity(EditorContext& ctx)
{
    const bool isUnit = m_new.kind == ContentKind::UNIT;
    const DataFile file = isUnit ? DataFile::UNITS : DataFile::ENEMIES;

    std::string error;
    const auto sheet = ctx.assets.import(isUnit ? ImageFolder::UNIT_SHEETS : ImageFolder::ENEMY_SHEETS, m_new.sheetPath, error);
    if (!sheet)
    {
        m_new.error = error;
        return false;
    }

    const int uid = ctx.project.next_free_uid(file);
    const ordered_json damageZone = ordered_json::array({ 0, 25 });

    // Same key order as the existing files
    ordered_json entity;
    if (isUnit)
    {
        ordered_json form = {
            { "form", 0 }, { "minLevel", 0 }, { "maxLevel", 9 },
            { "name", m_new.name }, { "description", "" },
            { "cost", 50 }, { "cooldown", 2 },
            { "health", 10 },
            { "attackPower", 5 }, { "attackRange", 20 }, { "attackType", 1 }, { "attackFrequency", 1.5 },
            { "foreswing", 0.3 }, { "backswing", 0.3 },
            { "damageZone", damageZone },
            { "movementSpeed", 8 },
            { "knockbackCount", 2 },
            { "texture", *sheet }, { "frameCount", m_new.frameCount }, { "knockbackFrameIndex", m_new.knockbackFrame }
        };
        entity = { { "UID", uid }, { "baseUpgradeCost", 100 }, { "forms", ordered_json::array({ form }) } };
    }
    else
    {
        entity = {
            { "UID", uid },
            { "name", m_new.name }, { "description", "" },
            { "health", 100 },
            { "attackPower", 10 }, { "attackRange", 20 }, { "attackFrequency", 2 }, { "attackType", 1 },
            { "foreswing", 0.3 }, { "backswing", 0.3 },
            { "damageZone", damageZone },
            { "types", ordered_json::array() },
            { "movementSpeed", 8 },
            { "knockbackCount", 1 },
            { "texture", *sheet }, { "frameCount", m_new.frameCount }, { "knockbackFrameIndex", m_new.knockbackFrame }
        };
    }

    ctx.project.get(file)[key_for(ctx.project, file, uid)] = std::move(entity);
    ctx.commit(file);

    // Deploy icon: the picked file, or one made from the first frame
    if (isUnit)
    {
        const std::string iconPath = AssetLibrary::unit_icon_path(uid, 0);
        bool isIconWritten = true;
        if (!m_new.iconPath.empty()) isIconWritten = ctx.assets.replace(iconPath, m_new.iconPath, error);
        else if (m_new.isMakingIcon)
        {
            const auto icon = AssetLibrary::make_icon(m_new.sheet, m_new.frameCount);
            isIconWritten = icon && ctx.assets.replace(iconPath, *icon, error);
        }
        ctx.textures.forget(iconPath);
        if (!isIconWritten) ctx.notify("The unit was created, but its icon couldn't be written: " + error, true);
    }

    ctx.textures.forget(*sheet);
    ctx.select({ m_new.kind, uid, 0 });
    m_lastSelection = ctx.selection;
    ctx.notify(std::string("Created ") + kind_name(m_new.kind) + " " + std::to_string(uid) + " '" + m_new.name + "'");
    return true;
}

// ==================
// Delete
// ==================
void ContentPanel::draw_delete_modal(EditorContext& ctx)
{
    if (m_isDeleteRequested)
    {
        ImGui::OpenPopup("Delete?");
        m_isDeleteRequested = false;
    }
    if (!ImGui::BeginPopupModal("Delete?", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) return;

    std::string name;
    std::vector<Reference> uses;
    switch (m_toDelete.kind)
    {
    case ContentKind::UNIT:    name = ProjectQueries::unit_name(ctx.project, m_toDelete.uid); uses = ProjectQueries::uses_of_unit(ctx.project, m_toDelete.uid); break;
    case ContentKind::ENEMY:   name = ProjectQueries::enemy_name(ctx.project, m_toDelete.uid); uses = ProjectQueries::uses_of_enemy(ctx.project, m_toDelete.uid); break;
    case ContentKind::STAGE:   name = ProjectQueries::stage_name(ctx.project, m_toDelete.uid); uses = ProjectQueries::uses_of_stage(ctx.project, m_toDelete.uid); break;
    case ContentKind::CHAPTER: name = ProjectQueries::chapter_name(ctx.project, m_toDelete.uid); break;
    default: break;
    }

    ImGui::Text("Delete %s %d '%s'?", kind_name(m_toDelete.kind), m_toDelete.uid, name.c_str());
    if (!uses.empty())
    {
        ImGui::TextColored(ImVec4(1.f, 0.8f, 0.4f, 1.f), "It's still used by:");
        for (const Reference& use : uses) ImGui::BulletText("%s", use.label.c_str());
        ImGui::TextDisabled("They will show errors until they're changed.");
    }
    ImGui::TextDisabled("Edit > Undo (Ctrl+Z) brings it back.");

    ImGui::Spacing();
    if (ImGui::Button("Delete", ImVec2(120.f, 0.f)))
    {
        const Selection target = m_toDelete;
        ctx.defer([&ctx, target]()
            {
                const DataFile file = file_of(target.kind);
                const std::string key = ctx.project.find_key(file, target.uid);
                if (key.empty()) return;

                ctx.project.get(file).erase(key);
                ctx.commit(file);
                if (ctx.selection.kind == target.kind && ctx.selection.uid == target.uid) ctx.select({});
                ctx.notify(std::string("Deleted ") + kind_name(target.kind) + " " + std::to_string(target.uid));
            });
        ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel", ImVec2(120.f, 0.f))) ImGui::CloseCurrentPopup();
    ImGui::EndPopup();
}
