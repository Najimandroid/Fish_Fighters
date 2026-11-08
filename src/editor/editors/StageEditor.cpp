#include "StageEditor.h"

#include "../EditorWidgets.h"
#include "../ProjectQueries.h"
#include "../../data/StageData.h"

#include <algorithm>
#include <cmath>
#include <string>

using namespace EditorWidgets;

namespace
{
    constexpr float DEFAULT_PLAYER_BASE_HEALTH = 350.f;   // StageData::playerBaseHealth when the stage doesn't set it
    const DataFile STAGES_FILE = DataFile::STAGES; // Every edit of this editor is in stages.json

    // The loader reserves this many waves: kept equal to the list
    void sync_wave_count(ordered_json& stage)
    {
        stage["numberOfDifferentEnemies"] = stage["enemies"].size();
    }

    // ===== Table cells (no label column) =====
    void cell_int(EditorContext& ctx, ordered_json& object, const char* key, int min, int max, float speed, const char* format, const char* tooltip)
    {
        ImGui::SetNextItemWidth(-FLT_MIN);
        ImGui::PushID(key);
        int value = read_int(object, key);
        if (ImGui::DragInt("##cell", &value, speed, min, max, format, ImGuiSliderFlags_AlwaysClamp)) object[key] = value;
        if (ImGui::IsItemDeactivatedAfterEdit()) ctx.commit(STAGES_FILE);
        if (tooltip) ImGui::SetItemTooltip("%s", tooltip);
        ImGui::PopID();
    }

    void cell_number(EditorContext& ctx, ordered_json& object, const char* key, float min, float max, float speed, const char* format, const char* tooltip)
    {
        ImGui::SetNextItemWidth(-FLT_MIN);
        ImGui::PushID(key);
        float value = static_cast<float>(read_number(object, key));
        if (ImGui::DragFloat("##cell", &value, speed, min, max, format, ImGuiSliderFlags_AlwaysClamp)) object[key] = number_value(value);
        if (ImGui::IsItemDeactivatedAfterEdit()) ctx.commit(STAGES_FILE);
        if (tooltip) ImGui::SetItemTooltip("%s", tooltip);
        ImGui::PopID();
    }

    void cell_flag(EditorContext& ctx, ordered_json& object, const char* key, const char* tooltip)
    {
        ImGui::PushID(key);
        bool value = object.contains(key) && (object[key].is_boolean() ? object[key].get<bool>() : read_int(object, key) != 0);
        if (ImGui::Checkbox("##cell", &value))
        {
            object[key] = value ? 1 : 0;
            ctx.commit(STAGES_FILE);
        }
        ImGui::SetItemTooltip("%s", tooltip);
        ImGui::PopID();
    }

    // HP or ATK multiplier: "magnification": [ hp, attack ]
    void cell_magnification(EditorContext& ctx, ordered_json& wave, int index, const char* tooltip)
    {
        if (!wave.contains("magnification") || !wave["magnification"].is_array() || wave["magnification"].size() != 2)
            wave["magnification"] = ordered_json::array({ 1, 1 });

        ImGui::SetNextItemWidth(-FLT_MIN);
        ImGui::PushID(index);
        float value = wave["magnification"][index].is_number() ? wave["magnification"][index].get<float>() : 1.f;
        if (ImGui::DragFloat("##cell", &value, 0.01f, 0.01f, 1000.f, "x%.2f", ImGuiSliderFlags_AlwaysClamp))
            wave["magnification"][index] = number_value(value);
        if (ImGui::IsItemDeactivatedAfterEdit()) ctx.commit(STAGES_FILE);
        ImGui::SetItemTooltip("%s", tooltip);
        ImGui::PopID();
    }

    // Enemy of a wave: icon and name, click to change it
    bool enemy_button(EditorContext& ctx, int uid)
    {
        const ImVec2 size(std::max(60.f, ImGui::GetContentRegionAvail().x), 36.f);
        const ImVec2 min = ImGui::GetCursorScreenPos();
        const bool clicked = ImGui::InvisibleButton("##enemy", size);

        ImDrawList* drawList = ImGui::GetWindowDrawList();
        if (ImGui::IsItemHovered()) drawList->AddRectFilled(min, ImVec2(min.x + size.x, min.y + size.y), ImGui::GetColorU32(ImGuiCol_HeaderHovered), 4.f);

        FrameUV uv;
        const sf::Texture* texture = enemy_texture(ctx, uid, uv);
        draw_image_in_rect(drawList, texture, ImVec2(min.x + 2.f, min.y + 2.f), ImVec2(min.x + 46.f, min.y + size.y - 2.f), uv);

        const bool exists = ctx.project.find_uid(DataFile::ENEMIES, uid) != nullptr;
        const std::string name = exists ? ProjectQueries::enemy_name(ctx.project, uid) : "Missing enemy " + std::to_string(uid);
        drawList->PushClipRect(min, ImVec2(min.x + size.x, min.y + size.y), true);
        drawList->AddText(ImVec2(min.x + 52.f, min.y + (size.y - ImGui::GetTextLineHeight()) / 2.f),
            exists ? ImGui::GetColorU32(ImGuiCol_Text) : IM_COL32(255, 115, 100, 255), name.c_str());
        drawList->PopClipRect();

        ImGui::SetItemTooltip("%s (#%d)\nClick to change the enemy", name.c_str(), uid);
        return clicked;
    }

    // ==================
    // Waves
    // ==================
    void waves_table(EditorContext& ctx, ordered_json& stage)
    {
        if (!stage.contains("enemies") || !stage["enemies"].is_array()) stage["enemies"] = ordered_json::array();
        ordered_json& waves = stage["enemies"];

        enum class Action { NONE, UP, DOWN, COPY, REMOVE };
        Action action = Action::NONE;
        size_t actionIndex = 0;

        if (waves.empty()) ImGui::TextDisabled("No wave: the stage has no enemy.");

        // Fixed columns, scrolling sideways in a narrow panel (the enemy column stays visible).
        // A scrolling table needs its exact height: header + rows (enemy button height) + scrollbar
        const ImGuiStyle& style = ImGui::GetStyle();
        const float rowHeight = 36.f + style.CellPadding.y * 2.f + 1.f;
        const float tableHeight = ImGui::GetTextLineHeight() + style.CellPadding.y * 2.f + 1.f
            + rowHeight * waves.size() + style.ScrollbarSize + 6.f;

        const ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit
            | ImGuiTableFlags_ScrollX | ImGuiTableFlags_Resizable;
        if (!waves.empty() && ImGui::BeginTable("Waves", 11, flags, ImVec2(0.f, tableHeight)))
        {
            ImGui::TableSetupScrollFreeze(1, 1);
            ImGui::TableSetupColumn("Enemy", ImGuiTableColumnFlags_WidthFixed, 190.f);
            ImGui::TableSetupColumn("Amount", ImGuiTableColumnFlags_WidthFixed, 100.f);
            ImGui::TableSetupColumn("Every", ImGuiTableColumnFlags_WidthFixed, 80.f);
            ImGui::TableSetupColumn("Starts at", ImGuiTableColumnFlags_WidthFixed, 80.f);
            ImGui::TableSetupColumn("Base HP", ImGuiTableColumnFlags_WidthFixed, 80.f);
            ImGui::TableSetupColumn("HP", ImGuiTableColumnFlags_WidthFixed, 70.f);
            ImGui::TableSetupColumn("ATK", ImGuiTableColumnFlags_WidthFixed, 70.f);
            ImGui::TableSetupColumn("Layer", ImGuiTableColumnFlags_WidthFixed, 100.f);
            ImGui::TableSetupColumn("Boss", ImGuiTableColumnFlags_WidthFixed, 40.f);
            ImGui::TableSetupColumn("No limit", ImGuiTableColumnFlags_WidthFixed, 60.f);
            ImGui::TableSetupColumn("##actions", ImGuiTableColumnFlags_WidthFixed, 118.f);
            ImGui::TableHeadersRow();

            for (size_t i = 0; i < waves.size(); ++i)
            {
                ordered_json& wave = waves[i];
                ImGui::PushID(static_cast<int>(i));
                ImGui::TableNextRow();

                ImGui::TableSetColumnIndex(0);
                if (enemy_button(ctx, read_int(wave, "UID"))) ImGui::OpenPopup("PickEnemy");
                int enemyUid = read_int(wave, "UID");
                if (enemy_picker(ctx, "PickEnemy", enemyUid))
                {
                    wave["UID"] = enemyUid;
                    ctx.commit(STAGES_FILE);
                }

                ImGui::TableSetColumnIndex(1);
                cell_int(ctx, wave, "amount", -1, 100000, 0.1f, read_int(wave, "amount") == -1 ? "%d (infinite)" : "%d",
                    "How many spawn. -1 = infinite");
                ImGui::TableSetColumnIndex(2);
                cell_number(ctx, wave, "respawnTime", 0.f, 3600.f, 0.05f, "%.1f s", "Seconds between two spawns");
                ImGui::TableSetColumnIndex(3);
                cell_number(ctx, wave, "spawnStart", 0.f, 3600.f, 0.05f, "%.1f s", "Seconds after the battle starts before the first spawn");
                ImGui::TableSetColumnIndex(4);
                cell_number(ctx, wave, "baseHealth", 0.f, 100.f, 0.2f, "<= %.0f%%",
                    "Only spawns once the enemy base is at or below this % of its health (100 = from the start)");
                ImGui::TableSetColumnIndex(5);
                cell_magnification(ctx, wave, 0, "Health multiplier of this wave");
                ImGui::TableSetColumnIndex(6);
                cell_magnification(ctx, wave, 1, "Attack multiplier of this wave");
                ImGui::TableSetColumnIndex(7);
                cell_int(ctx, wave, "layer", -1, 50, 0.05f, read_int(wave, "layer") <= 0 ? "%d (random)" : "%d",
                    "Drawing depth (1-50). -1 or 0 = random");
                ImGui::TableSetColumnIndex(8);
                cell_flag(ctx, wave, "isBoss", "Boss: its arrival pushes every fish back (shockwave)");
                ImGui::TableSetColumnIndex(9);
                cell_flag(ctx, wave, "bypassEnemyLimit", "Spawns even when the stage's enemy limit is reached (bosses should, to avoid cheese)");

                ImGui::TableSetColumnIndex(10);
                if (ImGui::ArrowButton("##up", ImGuiDir_Up)) { action = Action::UP; actionIndex = i; }
                ImGui::SetItemTooltip("Move up");
                ImGui::SameLine(0.f, 2.f);
                if (ImGui::ArrowButton("##down", ImGuiDir_Down)) { action = Action::DOWN; actionIndex = i; }
                ImGui::SetItemTooltip("Move down");
                ImGui::SameLine(0.f, 2.f);
                if (ImGui::Button("+")) { action = Action::COPY; actionIndex = i; }
                ImGui::SetItemTooltip("Duplicate this wave");
                ImGui::SameLine(0.f, 2.f);
                if (ImGui::Button("X")) { action = Action::REMOVE; actionIndex = i; }
                ImGui::SetItemTooltip("Remove this wave");

                ImGui::PopID();
            }
            ImGui::EndTable();
        }

        // Structural changes once the rows are drawn (nothing refers to them anymore)
        switch (action)
        {
        case Action::UP:
            if (actionIndex > 0) std::swap(waves[actionIndex], waves[actionIndex - 1]);
            break;
        case Action::DOWN:
            if (actionIndex + 1 < waves.size()) std::swap(waves[actionIndex], waves[actionIndex + 1]);
            break;
        case Action::COPY:
        {
            ordered_json copy = waves[actionIndex];
            waves.insert(waves.begin() + static_cast<std::ptrdiff_t>(actionIndex) + 1, std::move(copy));
            break;
        }
        case Action::REMOVE:
            waves.erase(actionIndex);
            break;
        default:
            break;
        }
        if (action != Action::NONE)
        {
            sync_wave_count(stage);
            ctx.commit(STAGES_FILE);
        }

        // ----- Add a wave -----
        if (ImGui::Button("+ Add wave")) ImGui::OpenPopup("AddWave");
        int newEnemy = 0;
        if (enemy_picker(ctx, "AddWave", newEnemy))
        {
            waves.push_back({
                { "UID", newEnemy }, { "amount", 1 }, { "respawnTime", 5 }, { "spawnStart", 0 }, { "layer", -1 },
                { "baseHealth", 100 }, { "magnification", ordered_json::array({ 1, 1 }) }, { "isBoss", 0 }, { "bypassEnemyLimit", 0 }
                });
            sync_wave_count(stage);
            ctx.commit(STAGES_FILE);
        }
    }

    // ==================
    // Timeline
    // ==================
    void timeline(EditorContext& ctx, const ordered_json& stage)
    {
        const ordered_json& waves = stage["enemies"];
        if (waves.empty()) return;

        // Long enough to show every finite wave (between 1 and 10 minutes)
        float duration = 60.f;
        for (const auto& wave : waves)
        {
            const int amount = read_int(wave, "amount");
            const float start = static_cast<float>(read_number(wave, "spawnStart"));
            const float every = static_cast<float>(read_number(wave, "respawnTime"));
            if (amount > 0) duration = std::max(duration, start + every * (amount - 1) + 5.f);
        }
        duration = std::min(duration, 600.f);

        const float labelWidth = 170.f;
        const float rowHeight = 26.f;
        const float width = ImGui::GetContentRegionAvail().x;
        const float trackWidth = std::max(100.f, width - labelWidth - 10.f);
        const ImVec2 origin = ImGui::GetCursorScreenPos();
        const float height = rowHeight * waves.size() + 22.f;
        ImGui::Dummy(ImVec2(width, height));

        ImDrawList* drawList = ImGui::GetWindowDrawList();
        const float trackX = origin.x + labelWidth;
        auto timeToX = [&](float time) { return trackX + time / duration * trackWidth; };

        // Seconds axis
        const float step = duration <= 60.f ? 5.f : duration <= 180.f ? 15.f : duration <= 360.f ? 30.f : 60.f;
        for (float time = 0.f; time <= duration + 0.01f; time += step)
        {
            const float x = timeToX(time);
            drawList->AddLine(ImVec2(x, origin.y + 16.f), ImVec2(x, origin.y + height), IM_COL32(255, 255, 255, 25));
            drawList->AddText(ImVec2(x + 2.f, origin.y), IM_COL32(170, 190, 200, 255), (std::to_string(static_cast<int>(time)) + "s").c_str());
        }

        for (size_t i = 0; i < waves.size(); ++i)
        {
            const ordered_json& wave = waves[i];
            const float y = origin.y + 20.f + rowHeight * i;
            const float centerY = y + rowHeight / 2.f;
            const int uid = read_int(wave, "UID");

            // Label: icon and name
            FrameUV uv;
            const sf::Texture* texture = enemy_texture(ctx, uid, uv);
            draw_image_in_rect(drawList, texture, ImVec2(origin.x, y + 2.f), ImVec2(origin.x + 30.f, y + rowHeight - 2.f), uv);
            drawList->PushClipRect(ImVec2(origin.x, y), ImVec2(trackX - 6.f, y + rowHeight), true);
            drawList->AddText(ImVec2(origin.x + 34.f, centerY - ImGui::GetTextLineHeight() / 2.f), IM_COL32_WHITE,
                (std::to_string(i + 1) + ". " + ProjectQueries::enemy_name(ctx.project, uid)).c_str());
            drawList->PopClipRect();

            const int amount = read_int(wave, "amount");
            const float start = static_cast<float>(read_number(wave, "spawnStart"));
            const float every = std::max(0.05f, static_cast<float>(read_number(wave, "respawnTime")));
            const bool isBoss = read_int(wave, "isBoss") != 0;
            const bool isConditional = read_number(wave, "baseHealth", 100.0) < 100.0;
            const ImU32 color = isBoss ? IM_COL32(255, 160, 115, 255) : isConditional ? IM_COL32(200, 170, 255, 255) : IM_COL32(70, 190, 210, 255);

            // First spawn at "spawnStart", then one every "respawnTime" (Stage::update)
            const float end = amount < 0 ? duration : std::min(duration, start + every * std::max(0, amount - 1));
            if (start <= duration)
            {
                drawList->AddLine(ImVec2(timeToX(start), centerY), ImVec2(timeToX(end), centerY), (color & 0x00FFFFFF) | 0x60000000, 2.f);
                int drawn = 0;
                for (float time = start; time <= end + 0.001f && drawn < 400; time += every, ++drawn)
                    drawList->AddCircleFilled(ImVec2(timeToX(time), centerY), isBoss ? 6.f : 3.5f, color);
                if (amount < 0) drawList->AddText(ImVec2(timeToX(end) - 14.f, y), color, ">>");
            }

            // Hover: spawn summary
            const ImVec2 rowMin(origin.x, y);
            const ImVec2 rowMax(origin.x + width, y + rowHeight);
            if (ImGui::IsMouseHoveringRect(rowMin, rowMax))
            {
                drawList->AddRect(rowMin, rowMax, IM_COL32(255, 255, 255, 40), 4.f);
                ImGui::SetTooltip("%s\n%s from %.1f s, every %.1f s%s%s",
                    ProjectQueries::enemy_name(ctx.project, uid).c_str(),
                    amount < 0 ? "Infinite" : (std::to_string(amount) + " spawn(s)").c_str(), start, every,
                    isConditional ? ("\nOnly once the enemy base is <= " + std::to_string(read_int(wave, "baseHealth")) + "% (the timer waits until then)").c_str() : "",
                    isBoss ? "\nBoss" : "");
            }
        }
    }

    // ==================
    // Rewards
    // ==================
    void rewards(EditorContext& ctx, ordered_json& stage)
    {
        if (!stage.contains("rewards") || !stage["rewards"].is_array()) stage["rewards"] = ordered_json::array();
        ordered_json& list = stage["rewards"];

        int removed = -1;
        if (list.empty()) ImGui::TextDisabled("No reward.");

        for (size_t i = 0; i < list.size(); ++i)
        {
            ordered_json& reward = list[i];
            ImGui::PushID(static_cast<int>(i));

            const bool isUnit = read_string(reward, "type") == "unit";
            if (isUnit)
            {
                const int uid = read_int(reward, "UID");
                unit_icon(ctx, uid, 0, ImVec2(56.f, 42.f));
                ImGui::SameLine();
                ImGui::BeginGroup();
                ImGui::Text("Unlocks a unit");
                if (ImGui::Button((ProjectQueries::unit_name(ctx.project, uid) + " (#" + std::to_string(uid) + ")...").c_str()))
                    ImGui::OpenPopup("PickUnit");
                ImGui::EndGroup();

                int picked = uid;
                if (unit_picker(ctx, "PickUnit", picked))
                {
                    reward["UID"] = picked;
                    ctx.commit(STAGES_FILE);
                }
            }
            else
            {
                ImGui::Dummy(ImVec2(56.f, 42.f));
                ImGui::SameLine();
                ImGui::BeginGroup();
                ImGui::Text("Shells");
                ImGui::SetNextItemWidth(200.f);
                int amount = read_int(reward, "amount");
                if (ImGui::DragInt("##amount", &amount, 1.f, 0, 100000000, "%d shells", ImGuiSliderFlags_AlwaysClamp)) reward["amount"] = amount;
                if (ImGui::IsItemDeactivatedAfterEdit()) ctx.commit(STAGES_FILE);
                ImGui::EndGroup();
            }

            ImGui::SameLine(0.f, 30.f);
            if (ImGui::SmallButton("Remove")) removed = static_cast<int>(i);
            ImGui::PopID();
        }

        if (removed >= 0)
        {
            list.erase(static_cast<size_t>(removed));
            ctx.commit(STAGES_FILE);
        }

        if (ImGui::Button("+ Shells")) { list.push_back({ { "type", "shells" }, { "amount", 100 } }); ctx.commit(STAGES_FILE); }
        ImGui::SameLine();
        if (ImGui::Button("+ Unit")) ImGui::OpenPopup("AddUnitReward");
        int newUnit = 0;
        if (unit_picker(ctx, "AddUnitReward", newUnit))
        {
            list.push_back({ { "type", "unit" }, { "UID", newUnit } });
            ctx.commit(STAGES_FILE);
        }
    }

    // Tabs (defined after StageEditor::draw)
    void general_tab(EditorContext& ctx, int uid, ordered_json& stage);
    void battlefield_tab(EditorContext& ctx, ordered_json& stage);
    void ground_line_editor(EditorContext& ctx, ordered_json& stage);
}

namespace StageEditor
{
    void draw(EditorContext& ctx)
    {
        const int uid = ctx.selection.uid;
        ordered_json* stage = ctx.project.find_uid(STAGES_FILE, uid);
        if (!stage)
        {
            ImGui::TextDisabled("Stage %d doesn't exist anymore.", uid);
            return;
        }
        // ----- Header -----
        ImGui::PushID(uid);
        image_in_box(ctx.textures.get(read_string(*stage, "baseTexture")), ImVec2(72.f, 72.f), {}, true);
        ImGui::SameLine();
        ImGui::BeginGroup();
        ImGui::Text("%s", ProjectQueries::stage_name(ctx.project, uid).c_str());
        const size_t waveCount = stage->contains("enemies") && (*stage)["enemies"].is_array() ? (*stage)["enemies"].size() : 0;
        ImGui::TextDisabled("Stage #%d   %zu wave(s)   stages.json", uid, waveCount);
        ImGui::EndGroup();
        ImGui::PopID();
        ImGui::Spacing();

        // ----- Categories (the open tab stays when switching stages) -----
        if (ImGui::BeginTabBar("StageCategories"))
        {
            auto tab = [&](const char* name, auto drawContent)
                {
                    if (!ImGui::BeginTabItem(name)) return;
                    ImGui::PushID(uid);
                    drawContent();
                    ImGui::PopID();
                    ImGui::EndTabItem();
                };
            tab("General", [&]() { general_tab(ctx, uid, *stage); });
            tab("Battlefield", [&]() { battlefield_tab(ctx, *stage); });
            tab("Waves", [&]()
                {
                    section("Enemy waves");
                    waves_table(ctx, *stage);
                    section("Timeline");
                    ImGui::TextDisabled("When each wave spawns. Purple: waits for the enemy base health. Orange: boss.");
                    timeline(ctx, *stage);
                });
            tab("Rewards", [&]() { rewards(ctx, *stage); });
            ImGui::EndTabBar();
        }
    }
}

namespace
{
    // ==================
    // Tabs
    // ==================
    void general_tab(EditorContext& ctx, int uid, ordered_json& stage)
    {
        if (begin_properties("General"))
        {
            text_field(ctx, STAGES_FILE, "Name", stage, "stageName");
            int_field(ctx, STAGES_FILE, "Enemy limit", stage, "enemiesLimit", 1, 1000, "Maximum enemies on the field at once.", 0.1f);
            int_field(ctx, STAGES_FILE, "Unit limit", stage, "unitsLimit", 1, 1000, "Maximum fish the player can have on the field.", 0.1f);
            end_properties();
        }

        section("On the chapter maps");
        const auto chapters = ProjectQueries::uses_of_stage(ctx.project, uid);
        if (chapters.empty()) ImGui::TextColored(ImVec4(1.f, 0.8f, 0.4f, 1.f), "Not on any chapter map yet: add it in a chapter's Map tab to make it playable.");
        for (size_t i = 0; i < chapters.size(); ++i)
        {
            ImGui::PushID(static_cast<int>(i));
            if (ImGui::TextLink(chapters[i].label.c_str())) ctx.defer([&ctx, target = chapters[i].target]() { ctx.select(target); });
            ImGui::PopID();
        }
    }

    void battlefield_tab(EditorContext& ctx, ordered_json& stageObject)
    {
        ordered_json* stage = &stageObject;

        section("Bases and length");
        if (begin_properties("Rules"))
        {
            int_field(ctx, STAGES_FILE, "Enemy base health", *stage, "baseHealth", 1, 1000000000, "Destroy it to win.");

            // Optional: without it the player base has the default health
            property_label("Player base health", "Optional. Without it, the player's base has 350 HP.");
            bool hasCustomHealth = stage->contains("playerBaseHealth");
            if (ImGui::Checkbox("##customPlayerBase", &hasCustomHealth))
            {
                if (hasCustomHealth) (*stage)["playerBaseHealth"] = DEFAULT_PLAYER_BASE_HEALTH;
                else stage->erase("playerBaseHealth");
                ctx.commit(STAGES_FILE);
            }
            ImGui::SameLine();
            if (hasCustomHealth)
            {
                ImGui::SetNextItemWidth(-FLT_MIN);
                int health = read_int(*stage, "playerBaseHealth", static_cast<int>(DEFAULT_PLAYER_BASE_HEALTH));
                if (ImGui::DragInt("##playerBaseHealth", &health, 1.f, 1, 1000000000, "%d", ImGuiSliderFlags_AlwaysClamp))
                    (*stage)["playerBaseHealth"] = health;
                if (ImGui::IsItemDeactivatedAfterEdit()) ctx.commit(STAGES_FILE);
            }
            else ImGui::TextDisabled("Default (%.0f)", DEFAULT_PLAYER_BASE_HEALTH);

            number_field(ctx, STAGES_FILE, "Length", *stage, "length", 100.f, 100000.f, "Distance between the enemy base (left) and the player base (right).", 5.f, "%.0f");
            end_properties();
        }

        section("Images");
        if (begin_properties("Images"))
        {
            image_field(ctx, STAGES_FILE, "Enemy base", *stage, "baseTexture", ImageFolder::BASES);
            image_field(ctx, STAGES_FILE, "Background", *stage, "backgroundTexture", ImageFolder::STAGE_BACKGROUNDS,
                "Any size. Repeated along the stage, drawn x1.5 (Stage::init_background).");
            end_properties();
        }

        section("Ground line");
        ground_line_editor(ctx, *stage);
    }

    // Where the units stand on the background ("backgroundGroundRow", optional)
    void ground_line_editor(EditorContext& ctx, ordered_json& stage)
    {
        const sf::Texture* background = ctx.textures.get(read_string(stage, "backgroundTexture"));
        if (!background)
        {
            ImGui::TextDisabled("Pick a background to place its ground line.");
            return;
        }

        const float imageHeight = static_cast<float>(background->getSize().y);
        const bool isCustom = stage.contains("backgroundGroundRow");
        float row = background_ground_row(isCustom ? static_cast<float>(read_number(stage, "backgroundGroundRow")) : -1.f, imageHeight);

        // ----- Value -----
        if (begin_properties("Ground"))
        {
            property_label("Ground line", "The row of the background image the units stand on.\n"
                "Without it, the same place as the 1280 x 1120 template (55% of the height).\nDrag the line on the image below.");
            bool hasCustomRow = isCustom;
            if (ImGui::Checkbox("##customGround", &hasCustomRow))
            {
                if (hasCustomRow) stage["backgroundGroundRow"] = std::lround(row);
                else stage.erase("backgroundGroundRow");
                ctx.commit(STAGES_FILE);
            }
            ImGui::SetItemTooltip("Set the ground line of this stage");
            ImGui::SameLine();
            if (hasCustomRow)
            {
                ImGui::SetNextItemWidth(-FLT_MIN);
                if (ImGui::DragFloat("##groundRow", &row, 1.f, 0.f, imageHeight, "row %.0f", ImGuiSliderFlags_AlwaysClamp))
                    stage["backgroundGroundRow"] = std::lround(row);
                if (ImGui::IsItemDeactivatedAfterEdit()) ctx.commit(STAGES_FILE);
            }
            else ImGui::TextDisabled("Template ratio: row %.0f of %.0f", row, imageHeight);
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
            stage["backgroundGroundRow"] = std::lround(row);
        }
        if (ImGui::IsItemDeactivated()) ctx.commit(STAGES_FILE);
        if (ImGui::IsItemHovered()) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
        ImGui::SetItemTooltip("Click or drag to place the ground line");

        ImDrawList* drawList = ImGui::GetWindowDrawList();
        drawList->AddImage(texture_id(*background), origin, ImVec2(origin.x + size.x, origin.y + size.y));
        auto rowY = [&](float imageRow) { return origin.y + std::clamp(imageRow, 0.f, imageHeight) * scale; };

        // What the camera shows (default zoom, and zoomed out at most): world y 54-1026 and -270-1350
        auto band = [&](float worldTop, float worldBottom, ImU32 color)
            {
                const float top = row + (worldTop - BATTLE_GROUND_Y) / BACKGROUND_SCALE;
                const float bottom = row + (worldBottom - BATTLE_GROUND_Y) / BACKGROUND_SCALE;
                drawList->AddRect(ImVec2(origin.x + 1.f, rowY(top)), ImVec2(origin.x + size.x - 1.f, rowY(bottom)), color, 0.f, 0, 1.5f);
            };
        band(-270.f, 1350.f, IM_COL32(255, 255, 255, 70));
        band(54.f, 1026.f, IM_COL32(255, 215, 70, 200));

        // Units stand from the line (layer 0) up to 50 world px higher (layer 50)
        const float layerRows = 50.f / BACKGROUND_SCALE;
        drawList->AddRectFilled(ImVec2(origin.x, rowY(row - layerRows)), ImVec2(origin.x + size.x, rowY(row)), IM_COL32(255, 80, 80, 60));
        drawList->AddLine(ImVec2(origin.x, rowY(row)), ImVec2(origin.x + size.x, rowY(row)), IM_COL32(255, 80, 80, 255), 2.f);

        ImGui::TextDisabled("Red: where the units stand (their layer moves them up to the top of the band).");
        ImGui::TextDisabled("Yellow: what the camera shows at the default zoom. White: zoomed out at most.");
    }
}
