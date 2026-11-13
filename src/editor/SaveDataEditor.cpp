#include "SaveDataEditor.h"
#include "EditorWidgets.h"

#include <algorithm>
#include <fstream>
#include <set>

namespace fs = std::filesystem;
using namespace EditorWidgets;

namespace
{
    const char* SAVE_FILE = "game_data/player.json";
    const int MAX_LEVEL = 30;
    const int MAX_EXTRA_LEVELS = 10;   // PlayerData::MAX_EXTRA_LEVELS
    const int LINEUP_SIZE = 10;

    // Sorted unique numbers of a JSON array
    std::set<int> number_set(const ordered_json& array)
    {
        std::set<int> numbers;
        if (array.is_array())
            for (const auto& value : array) if (value.is_number()) numbers.insert(value.get<int>());
        return numbers;
    }

    ordered_json to_array(const std::set<int>& numbers)
    {
        ordered_json array = ordered_json::array();
        for (int number : numbers) array.push_back(number);
        return array;
    }

    // A checkbox editing whether "number" is in a JSON array of numbers. True when it changed
    bool member_checkbox(const char* label, ordered_json& array, int number)
    {
        std::set<int> numbers = number_set(array);
        bool isMember = numbers.count(number) > 0;
        if (!ImGui::Checkbox(label, &isMember)) return false;
        if (isMember) numbers.insert(number); else numbers.erase(number);
        array = to_array(numbers);
        return true;
    }
}

void SaveDataEditor::open(const fs::path& projectFolder, const std::vector<GameBuilds::Build>& builds)
{
    m_files.clear();
    m_files.push_back({ "Project (starting save, copied by every build)", projectFolder / SAVE_FILE });
    for (const auto& build : builds)
        m_files.push_back({ build.name + " (progress of that build)", build.folder / SAVE_FILE });

    m_current = 0;
    std::string error;
    m_isLoaded = load(error);
    isOpen = true;
}

ordered_json SaveDataEditor::new_game()
{
    ordered_json save;
    save["money"]["shells"] = 0;
    save["progression"]["completedChapters"] = ordered_json::array();
    save["progression"]["completedStages"] = ordered_json::array();
    save["progression"]["currentChapter"] = 1;
    save["progression"]["seenEnemies"] = ordered_json::array();
    ordered_json equipped = ordered_json::array();
    for (int i = 0; i < LINEUP_SIZE; ++i) equipped.push_back(i == 0 ? 1 : -1);
    save["units"]["equipped"] = equipped;
    save["units"]["owned"]["1"] = { { "form", 0 }, { "level", 1 }, { "uid", 1 } };
    save["units"]["waitingToBeUnlocked"] = ordered_json::object();
    return save;
}

void SaveDataEditor::normalize()
{
    const ordered_json defaults = new_game();
    for (const char* group : { "money", "progression", "units" })
        if (!m_save.contains(group) || !m_save[group].is_object()) m_save[group] = ordered_json::object();
    for (const char* key : { "completedChapters", "completedStages", "seenEnemies" })
        if (!m_save["progression"].contains(key) || !m_save["progression"][key].is_array()) m_save["progression"][key] = ordered_json::array();
    if (!m_save["money"].contains("shells")) m_save["money"]["shells"] = 0;
    if (!m_save["progression"].contains("currentChapter")) m_save["progression"]["currentChapter"] = 1;
    if (!m_save["units"].contains("owned") || !m_save["units"]["owned"].is_object()) m_save["units"]["owned"] = ordered_json::object();
    if (!m_save["units"].contains("waitingToBeUnlocked") || !m_save["units"]["waitingToBeUnlocked"].is_object()) m_save["units"]["waitingToBeUnlocked"] = ordered_json::object();
    ordered_json& equipped = m_save["units"]["equipped"];
    if (!equipped.is_array()) equipped = defaults["units"]["equipped"];
    while (equipped.size() < LINEUP_SIZE) equipped.push_back(-1);
}

bool SaveDataEditor::load(std::string& error)
{
    m_hasChanges = false;
    const fs::path& path = m_files[m_current].path;
    std::ifstream file(path);
    if (!file)
    {
        // No save yet: the game would start a new one
        m_save = new_game();
        error = "No save there yet: showing a new game";
        return true;
    }
    try
    {
        m_save = ordered_json::parse(file);
        normalize();
        return true;
    }
    catch (const std::exception& exception)
    {
        error = "Can't read " + path.string() + ": " + exception.what();
        return false;
    }
}

bool SaveDataEditor::write(const fs::path& path, std::string& error) const
{
    std::error_code code;
    fs::create_directories(path.parent_path(), code);
    const fs::path temporary = path.string() + ".tmp";
    {
        std::ofstream file(temporary, std::ios::trunc);
        if (!file) { error = "Can't write " + temporary.string(); return false; }
        file << m_save.dump(4);
    }
    fs::rename(temporary, path, code);
    if (code) { error = "Can't replace " + path.string() + ": " + code.message(); return false; }
    return true;
}

void SaveDataEditor::draw(EditorContext& ctx)
{
    if (!isOpen) return;

    ImGui::SetNextWindowSize(ImVec2(760.f, 720.f), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Save data", &isOpen))
    {
        ImGui::End();
        return;
    }

    // ----- Which save -----
    ImGui::SetNextItemWidth(-FLT_MIN);
    if (ImGui::BeginCombo("##file", m_files.empty() ? "" : m_files[m_current].label.c_str()))
    {
        for (int i = 0; i < static_cast<int>(m_files.size()); ++i)
        {
            if (ImGui::Selectable(m_files[i].label.c_str(), i == m_current) && i != m_current)
            {
                m_current = i;
                std::string error;
                m_isLoaded = load(error);
                if (!error.empty()) ctx.notify(error, !m_isLoaded);
            }
            ImGui::SetItemTooltip("%s", m_files[i].path.string().c_str());
        }
        ImGui::EndCombo();
    }
    ImGui::TextDisabled("%s", m_files.empty() ? "" : m_files[m_current].path.string().c_str());

    if (!m_isLoaded)
    {
        ImGui::TextColored(ImVec4(1.f, 0.45f, 0.4f, 1.f), "This save can't be read.");
        ImGui::End();
        return;
    }

    // ----- Actions -----
    ImGui::BeginDisabled(!m_hasChanges);
    if (ImGui::Button("Save"))
    {
        std::string error;
        if (write(m_files[m_current].path, error)) { m_hasChanges = false; ctx.notify("Saved " + m_files[m_current].path.string()); }
        else ctx.notify(error, true);
    }
    ImGui::EndDisabled();
    ImGui::SameLine();
    if (ImGui::Button("Reload"))
    {
        std::string error;
        m_isLoaded = load(error);
        if (!error.empty()) ctx.notify(error, !m_isLoaded);
    }
    ImGui::SameLine();
    if (ImGui::Button("Copy to every save"))
    {
        std::string error;
        int written = 0;
        for (const SaveFile& save : m_files)
        {
            if (write(save.path, error)) ++written;
            else ctx.notify(error, true);
        }
        m_hasChanges = false;
        ctx.notify("Copied to " + std::to_string(written) + " save(s)");
    }
    ImGui::SetItemTooltip("Writes this save to the project and to every build");
    ImGui::SameLine();
    if (ImGui::Button("Reset to a new game...")) ImGui::OpenPopup("Reset save");
    ImGui::SetItemTooltip("Like the game's first launch: the first fish, no shells, nothing cleared");

    if (ImGui::BeginPopupModal("Reset save", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Replace this save with a new game?\nYou still have to click Save (or Copy to every save) to write it.");
        if (ImGui::Button("Reset")) { m_save = new_game(); m_hasChanges = true; ImGui::CloseCurrentPopup(); }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
    if (m_hasChanges) { ImGui::SameLine(); ImGui::TextColored(ImVec4(1.f, 0.8f, 0.3f, 1.f), "Not saved"); }

    ImGui::Separator();
    if (ImGui::BeginTabBar("##saveTabs"))
    {
        if (ImGui::BeginTabItem("Progress")) { draw_progress(ctx); ImGui::EndTabItem(); }
        if (ImGui::BeginTabItem("Fish")) { draw_units(ctx); ImGui::EndTabItem(); }
        if (ImGui::BeginTabItem("Enemies met")) { draw_enemies(ctx); ImGui::EndTabItem(); }
        ImGui::EndTabBar();
    }

    ImGui::End();
}

void SaveDataEditor::draw_progress(EditorContext& ctx)
{
    if (begin_properties("##progress"))
    {
        property_label("Shells", "The currency of the Upgrade screen");
        int shells = read_int(m_save["money"], "shells");
        if (ImGui::DragInt("##shells", &shells, 10.f, 0, 1000000000)) { m_save["money"]["shells"] = std::max(0, shells); m_hasChanges = true; }

        property_label("Shiny Scales", "The Clam Room's currency (from the Shoal Power's milestones, the welcome gift, crafting)");
        int scales = m_save["money"].contains("shinyScales") ? read_int(m_save["money"], "shinyScales") : 0;
        if (ImGui::DragInt("##scales", &scales, 1.f, 0, 1000000)) { m_save["money"]["shinyScales"] = std::max(0, scales); m_hasChanges = true; }

        property_label("Fish Scales", "From selling fish in the Clam Storage: 10 craft a Shiny Scale");
        int fishScales = m_save["money"].contains("fishScales") ? read_int(m_save["money"], "fishScales") : 0;
        if (ImGui::DragInt("##fishScales", &fishScales, 1.f, 0, 1000000)) { m_save["money"]["fishScales"] = std::max(0, fishScales); m_hasChanges = true; }

        property_label("Current chapter", "The chapter the map opens on");
        int chapter = read_int(m_save["progression"], "currentChapter", 1);
        if (ImGui::InputInt("##chapter", &chapter)) { m_save["progression"]["currentChapter"] = std::max(1, chapter); m_hasChanges = true; }
        end_properties();
    }

    // Cleared chapters, then their stages in map order
    ordered_json& completedStages = m_save["progression"]["completedStages"];
    ordered_json& completedChapters = m_save["progression"]["completedChapters"];
    std::set<int> listedStages;

    for (int chapterUid : ctx.project.sorted_uids(DataFile::CHAPTERS))
    {
        const ordered_json* chapter = ctx.project.find_uid(DataFile::CHAPTERS, chapterUid);
        if (!chapter) continue;
        ImGui::PushID(chapterUid);
        section(("Chapter " + std::to_string(chapterUid) + " - " + read_string(*chapter, "name")).c_str());
        if (member_checkbox("Chapter cleared", completedChapters, chapterUid)) m_hasChanges = true;

        // Every stage up to this one cleared (or none) in one click
        if (chapter->contains("stages") && (*chapter)["stages"].is_array())
        {
            int index = 0;
            for (const auto& stageEntry : (*chapter)["stages"])
            {
                const int stageUid = read_int(stageEntry, "UID", -1);
                const ordered_json* stage = ctx.project.find_uid(DataFile::STAGES, stageUid);
                listedStages.insert(stageUid);
                ImGui::PushID(stageUid);
                const std::string label = std::to_string(++index) + ". " + (stage ? read_string(*stage, "stageName") : "Missing stage");
                if (member_checkbox(label.c_str(), completedStages, stageUid)) m_hasChanges = true;
                ImGui::SameLine();
                if (ImGui::SmallButton("Up to here"))
                {
                    std::set<int> cleared = number_set(completedStages);
                    for (const auto& previous : (*chapter)["stages"])
                    {
                        cleared.insert(read_int(previous, "UID", -1));
                        if (read_int(previous, "UID", -1) == stageUid) break;
                    }
                    completedStages = to_array(cleared);
                    m_hasChanges = true;
                }
                ImGui::SetItemTooltip("Clears every stage of the chapter up to this one");
                ImGui::PopID();
            }
        }
        ImGui::PopID();
    }

    // Stages in no chapter (still saved)
    bool hasOthers = false;
    for (int stageUid : ctx.project.sorted_uids(DataFile::STAGES))
    {
        if (listedStages.count(stageUid) || stageUid < 0) continue;
        if (!hasOthers) { section("Stages in no chapter"); hasOthers = true; }
        const ordered_json* stage = ctx.project.find_uid(DataFile::STAGES, stageUid);
        ImGui::PushID(stageUid + 100000);
        if (member_checkbox((stage ? read_string(*stage, "stageName") : std::to_string(stageUid)).c_str(), completedStages, stageUid)) m_hasChanges = true;
        ImGui::PopID();
    }
}

void SaveDataEditor::draw_units(EditorContext& ctx)
{
    ordered_json& owned = m_save["units"]["owned"];
    ordered_json& waiting = m_save["units"]["waitingToBeUnlocked"];
    ordered_json& equipped = m_save["units"]["equipped"];
    const std::vector<int> uids = ctx.project.sorted_uids(DataFile::UNITS);

    auto unitName = [&](int uid, int form)
        {
            const ordered_json* unit = ctx.project.find_uid(DataFile::UNITS, uid);
            if (!unit || !unit->contains("forms") || form >= static_cast<int>((*unit)["forms"].size())) return std::string("Unit ") + std::to_string(uid);
            return read_string((*unit)["forms"][form], "name");
        };

    if (ImGui::BeginTable("##units", 6, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_SizingStretchProp))
    {
        ImGui::TableSetupColumn("Fish", ImGuiTableColumnFlags_WidthStretch, 2.f);
        ImGui::TableSetupColumn("Owned", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("Level", ImGuiTableColumnFlags_WidthStretch, 1.f);
        ImGui::TableSetupColumn("Extra", ImGuiTableColumnFlags_WidthStretch, 0.7f);
        ImGui::TableSetupColumn("Form", ImGuiTableColumnFlags_WidthStretch, 1.f);
        ImGui::TableSetupColumn("To unlock", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableHeadersRow();

        for (int uid : uids)
        {
            const std::string key = std::to_string(uid);
            const ordered_json* unit = ctx.project.find_uid(DataFile::UNITS, uid);
            const int formCount = (unit && unit->contains("forms")) ? static_cast<int>((*unit)["forms"].size()) : 1;
            bool isOwned = owned.contains(key);
            ordered_json ownedUnit = isOwned ? owned[key] : ordered_json{ { "form", 0 }, { "level", 1 }, { "uid", uid } };

            ImGui::PushID(uid);
            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            unit_icon(ctx, uid, read_int(ownedUnit, "form"), ImVec2(48.f, 36.f));
            ImGui::SameLine();
            ImGui::AlignTextToFramePadding();
            ImGui::Text("%s  #%d", unitName(uid, read_int(ownedUnit, "form")).c_str(), uid);

            ImGui::TableSetColumnIndex(1);
            if (ImGui::Checkbox("##owned", &isOwned))
            {
                if (isOwned) { owned[key] = ownedUnit; waiting.erase(key); }
                else owned.erase(key);
                m_hasChanges = true;
            }

            ImGui::BeginDisabled(!isOwned);
            ImGui::TableSetColumnIndex(2);
            ImGui::SetNextItemWidth(-FLT_MIN);
            int level = read_int(ownedUnit, "level", 1);
            if (ImGui::SliderInt("##level", &level, 1, MAX_LEVEL) && isOwned) { owned[key]["level"] = level; m_hasChanges = true; }

            ImGui::TableSetColumnIndex(3);
            ImGui::SetNextItemWidth(-FLT_MIN);
            int extra = read_int(ownedUnit, "extraLevels", 0);
            if (ImGui::SliderInt("##extra", &extra, 0, MAX_EXTRA_LEVELS, "+%d") && isOwned) { owned[key]["extraLevels"] = extra; m_hasChanges = true; }
            ImGui::SetItemTooltip("Extra levels from the Clam Room: stronger, the price and the form don't change");

            ImGui::TableSetColumnIndex(4);
            ImGui::SetNextItemWidth(-FLT_MIN);
            int form = read_int(ownedUnit, "form");
            if (ImGui::SliderInt("##form", &form, 0, std::max(0, formCount - 1), form < formCount ? unitName(uid, form).c_str() : "%d") && isOwned)
            {
                owned[key]["form"] = form;
                m_hasChanges = true;
            }
            ImGui::EndDisabled();

            ImGui::TableSetColumnIndex(5);
            ImGui::BeginDisabled(isOwned);
            bool isWaiting = waiting.contains(key);
            if (ImGui::Checkbox("##waiting", &isWaiting))
            {
                if (isWaiting) waiting[key] = { { "uid", uid } };
                else waiting.erase(key);
                m_hasChanges = true;
            }
            ImGui::SetItemTooltip("Shown in the Upgrade screen, to unlock with shells");
            ImGui::EndDisabled();

            ImGui::PopID();
        }
        ImGui::EndTable();
    }

    // ----- Lineup -----
    section("Lineup");
    for (int slot = 0; slot < LINEUP_SIZE; ++slot)
    {
        ImGui::PushID(1000 + slot);
        const int uid = equipped[slot].is_number() ? equipped[slot].get<int>() : -1;
        const std::string current = uid < 0 ? "Empty" : unitName(uid, owned.contains(std::to_string(uid)) ? read_int(owned[std::to_string(uid)], "form") : 0);
        ImGui::SetNextItemWidth(170.f);
        if (ImGui::BeginCombo(("##slot" + std::to_string(slot)).c_str(), current.c_str()))
        {
            if (ImGui::Selectable("Empty", uid < 0)) { equipped[slot] = -1; m_hasChanges = true; }
            for (const auto& [key, unitSave] : owned.items())
            {
                const int ownedUid = std::stoi(key);
                if (ImGui::Selectable(unitName(ownedUid, read_int(unitSave, "form")).c_str(), ownedUid == uid))
                {
                    for (auto& other : equipped) if (other.is_number() && other.get<int>() == ownedUid) other = -1;  // Once in the lineup
                    equipped[slot] = ownedUid;
                    m_hasChanges = true;
                }
            }
            ImGui::EndCombo();
        }
        if (slot % 5 != 4) ImGui::SameLine();
        ImGui::PopID();
    }
    ImGui::TextDisabled("Only owned fish can be in the lineup; the game moves them to the first slots.");
}

void SaveDataEditor::draw_enemies(EditorContext& ctx)
{
    ImGui::TextDisabled("Enemies met in battle: the chapter map and the encyclopedia show the others in black.");
    ordered_json& seen = m_save["progression"]["seenEnemies"];

    if (ImGui::Button("Met them all"))
    {
        std::set<int> all;
        for (int uid : ctx.project.sorted_uids(DataFile::ENEMIES)) if (uid >= 0) all.insert(uid);
        seen = to_array(all);
        m_hasChanges = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Met none")) { seen = ordered_json::array(); m_hasChanges = true; }

    for (int uid : ctx.project.sorted_uids(DataFile::ENEMIES))
    {
        const ordered_json* enemy = ctx.project.find_uid(DataFile::ENEMIES, uid);
        if (!enemy) continue;
        ImGui::PushID(uid);
        enemy_icon(ctx, uid, ImVec2(48.f, 36.f));
        ImGui::SameLine();
        if (member_checkbox((read_string(*enemy, "name") + "  #" + std::to_string(uid)).c_str(), seen, uid)) m_hasChanges = true;
        ImGui::PopID();
    }
}
