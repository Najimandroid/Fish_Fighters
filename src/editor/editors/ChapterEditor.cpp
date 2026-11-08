#include "ChapterEditor.h"

#include "../EditorWidgets.h"
#include "../ProjectQueries.h"

#include <algorithm>
#include <cmath>

using namespace EditorWidgets;

namespace
{
    const DataFile CHAPTERS_FILE = DataFile::CHAPTERS;

    constexpr float MAP_SCALE = 2.f;        // UIChapterMap draws the map image x2: positions are in those world pixels
    constexpr float POINT_RADIUS = 30.f;    // UIChapterMap's stage circle radius (world pixels)

    int s_draggedPoint = -1;
    int s_selectedPoint = -1;
    int s_chapterUid = 0;                   // Chapter the point indices belong to
    ImVec2 s_newPointPosition;              // World position of a point added with a right click

    // Every stage not on this map yet
    std::vector<int> stages_not_on_map(const ProjectData& project, const ordered_json& points)
    {
        std::vector<int> available;
        for (int uid : project.sorted_uids(DataFile::STAGES))
        {
            const bool isOnMap = std::any_of(points.begin(), points.end(), [uid](const ordered_json& point) { return read_int(point, "UID") == uid; });
            if (!isOnMap) available.push_back(uid);
        }
        return available;
    }

    // Menu of stages to add at a position. True when one was added
    bool add_stage_menu(EditorContext& ctx, ordered_json& points, ImVec2 worldPosition)
    {
        const std::vector<int> available = stages_not_on_map(ctx.project, points);
        if (available.empty()) ImGui::TextDisabled("Every stage is already on this map");

        for (int uid : available)
        {
            const std::string label = std::to_string(uid) + "  " + ProjectQueries::stage_name(ctx.project, uid);
            if (ImGui::MenuItem(label.c_str()))
            {
                points.push_back({ { "UID", uid }, { "x", std::lround(worldPosition.x) }, { "y", std::lround(worldPosition.y) } });
                s_selectedPoint = static_cast<int>(points.size()) - 1;
                ctx.commit(CHAPTERS_FILE);
                return true;
            }
        }
        return false;
    }

    // ==================
    // Map
    // ==================
    void draw_map(EditorContext& ctx, ordered_json& chapter)
    {
        if (!chapter.contains("stages") || !chapter["stages"].is_array()) chapter["stages"] = ordered_json::array();
        ordered_json& points = chapter["stages"];

        const sf::Texture* map = ctx.textures.get(read_string(chapter, "mapTexture"));
        const ImVec2 worldSize = map
            ? ImVec2(map->getSize().x * MAP_SCALE, map->getSize().y * MAP_SCALE)
            : ImVec2(2560.f, 1440.f);

        const float width = std::max(200.f, ImGui::GetContentRegionAvail().x);
        const float scale = width / worldSize.x;
        const ImVec2 size(width, worldSize.y * scale);
        const ImVec2 origin = ImGui::GetCursorScreenPos();

        ImGui::InvisibleButton("##map", size, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
        const bool isHovered = ImGui::IsItemHovered();

        auto toScreen = [&](float x, float y) { return ImVec2(origin.x + x * scale, origin.y + y * scale); };
        auto toWorld = [&](ImVec2 screen)
            {
                return ImVec2(std::clamp((screen.x - origin.x) / scale, 0.f, worldSize.x), std::clamp((screen.y - origin.y) / scale, 0.f, worldSize.y));
            };

        ImDrawList* drawList = ImGui::GetWindowDrawList();
        drawList->AddRectFilled(origin, ImVec2(origin.x + size.x, origin.y + size.y), IM_COL32(8, 16, 20, 255));
        if (map) drawList->AddImage(texture_id(*map), origin, ImVec2(origin.x + size.x, origin.y + size.y));
        else drawList->AddText(ImVec2(origin.x + 10.f, origin.y + 10.f), IM_COL32(255, 115, 100, 255), "Missing map image");

        const float radius = std::max(9.f, POINT_RADIUS * scale);

        // Dotted path between consecutive stages, like the game
        for (size_t i = 1; i < points.size(); ++i)
        {
            const ImVec2 a = toScreen(static_cast<float>(read_number(points[i - 1], "x")), static_cast<float>(read_number(points[i - 1], "y")));
            const ImVec2 b = toScreen(static_cast<float>(read_number(points[i], "x")), static_cast<float>(read_number(points[i], "y")));
            const float length = std::hypot(b.x - a.x, b.y - a.y);
            const int dots = static_cast<int>(length / 10.f);
            for (int d = 1; d < dots; ++d)
            {
                const float t = static_cast<float>(d) / dots;
                drawList->AddCircleFilled(ImVec2(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t), 2.f, IM_COL32(255, 255, 255, 170));
            }
        }

        // Point under the mouse
        int hoveredPoint = -1;
        const ImVec2 mouse = ImGui::GetIO().MousePos;
        for (int i = static_cast<int>(points.size()) - 1; i >= 0 && isHovered; --i)
        {
            const ImVec2 center = toScreen(static_cast<float>(read_number(points[i], "x")), static_cast<float>(read_number(points[i], "y")));
            if (std::hypot(mouse.x - center.x, mouse.y - center.y) <= radius + 3.f) { hoveredPoint = i; break; }
        }

        // ----- Dragging -----
        if (ImGui::IsItemActivated() && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            s_draggedPoint = hoveredPoint;
            s_selectedPoint = hoveredPoint;
        }
        if (s_draggedPoint >= 0 && s_draggedPoint < static_cast<int>(points.size()) && ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left, 1.f))
        {
            const ImVec2 world = toWorld(mouse);
            points[s_draggedPoint]["x"] = std::lround(world.x);
            points[s_draggedPoint]["y"] = std::lround(world.y);
        }
        if (ImGui::IsItemDeactivated() && s_draggedPoint >= 0)
        {
            s_draggedPoint = -1;
            ctx.commit(CHAPTERS_FILE);
        }

        // Double click: open the stage
        if (hoveredPoint >= 0 && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
        {
            const int stageUid = read_int(points[hoveredPoint], "UID");
            ctx.defer([&ctx, stageUid]() { ctx.select({ ContentKind::STAGE, stageUid }); });
        }

        // ----- Right click: point menu, or add a stage here -----
        if (isHovered && ImGui::IsMouseReleased(ImGuiMouseButton_Right))
        {
            s_selectedPoint = hoveredPoint;
            s_newPointPosition = toWorld(mouse);
            ImGui::OpenPopup(hoveredPoint >= 0 ? "PointMenu" : "MapMenu");
        }

        // ----- Points -----
        for (size_t i = 0; i < points.size(); ++i)
        {
            const int stageUid = read_int(points[i], "UID");
            const ImVec2 center = toScreen(static_cast<float>(read_number(points[i], "x")), static_cast<float>(read_number(points[i], "y")));
            const bool exists = ctx.project.find_uid(DataFile::STAGES, stageUid) != nullptr;
            const bool isSelected = static_cast<int>(i) == s_selectedPoint;

            const ImU32 fill = !exists ? IM_COL32(200, 60, 60, 230) : isSelected ? IM_COL32(255, 160, 115, 240) : IM_COL32(26, 120, 135, 230);
            drawList->AddCircleFilled(center, radius, fill);
            drawList->AddCircle(center, radius, static_cast<int>(i) == hoveredPoint ? IM_COL32_WHITE : IM_COL32(255, 255, 255, 150), 0, 2.f);

            const std::string number = std::to_string(i + 1);
            const ImVec2 numberSize = ImGui::CalcTextSize(number.c_str());
            drawList->AddText(ImVec2(center.x - numberSize.x / 2.f, center.y - numberSize.y / 2.f), IM_COL32_WHITE, number.c_str());

            const std::string name = exists ? ProjectQueries::stage_name(ctx.project, stageUid) : "Missing stage " + std::to_string(stageUid);
            const ImVec2 nameSize = ImGui::CalcTextSize(name.c_str());
            const ImVec2 namePosition(center.x - nameSize.x / 2.f, center.y + radius + 2.f);
            drawList->AddRectFilled(ImVec2(namePosition.x - 4.f, namePosition.y), ImVec2(namePosition.x + nameSize.x + 4.f, namePosition.y + nameSize.y),
                IM_COL32(0, 0, 0, 150), 4.f);
            drawList->AddText(namePosition, IM_COL32_WHITE, name.c_str());
        }

        if (hoveredPoint >= 0 && s_draggedPoint < 0)
        {
            const ordered_json& point = points[hoveredPoint];
            ImGui::SetTooltip("%d. %s (stage #%d)\nx %d, y %d\nDrag to move, double click to open, right click for more",
                hoveredPoint + 1, ProjectQueries::stage_name(ctx.project, read_int(point, "UID")).c_str(), read_int(point, "UID"),
                read_int(point, "x"), read_int(point, "y"));
        }

        // ----- Menus -----
        int removedPoint = -1;
        if (ImGui::BeginPopup("PointMenu"))
        {
            if (s_selectedPoint >= 0 && s_selectedPoint < static_cast<int>(points.size()))
            {
                const int stageUid = read_int(points[s_selectedPoint], "UID");
                ImGui::TextDisabled("%s", ProjectQueries::stage_name(ctx.project, stageUid).c_str());
                if (ImGui::MenuItem("Open stage")) ctx.defer([&ctx, stageUid]() { ctx.select({ ContentKind::STAGE, stageUid }); });
                if (ImGui::MenuItem("Remove from the map")) removedPoint = s_selectedPoint;
            }
            ImGui::EndPopup();
        }
        if (ImGui::BeginPopup("MapMenu"))
        {
            ImGui::TextDisabled("Add a stage here");
            add_stage_menu(ctx, points, s_newPointPosition);
            ImGui::EndPopup();
        }

        if (removedPoint >= 0)
        {
            points.erase(static_cast<size_t>(removedPoint));
            s_selectedPoint = -1;
            ctx.commit(CHAPTERS_FILE);
        }
    }

    // ==================
    // Stage list (the order is the path on the map)
    // ==================
    void draw_point_list(EditorContext& ctx, ordered_json& chapter)
    {
        ordered_json& points = chapter["stages"];

        enum class Action { NONE, UP, DOWN, REMOVE };
        Action action = Action::NONE;
        size_t actionIndex = 0;

        const ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp;
        if (!points.empty() && ImGui::BeginTable("Points", 5, flags))
        {
            ImGui::TableSetupColumn("#", ImGuiTableColumnFlags_WidthFixed, 26.f);
            ImGui::TableSetupColumn("Stage", ImGuiTableColumnFlags_WidthStretch, 2.f);
            ImGui::TableSetupColumn("X", ImGuiTableColumnFlags_WidthStretch, 0.7f);
            ImGui::TableSetupColumn("Y", ImGuiTableColumnFlags_WidthStretch, 0.7f);
            ImGui::TableSetupColumn("##actions", ImGuiTableColumnFlags_WidthFixed, 90.f);
            ImGui::TableHeadersRow();

            for (size_t i = 0; i < points.size(); ++i)
            {
                ordered_json& point = points[i];
                const int stageUid = read_int(point, "UID");
                ImGui::PushID(static_cast<int>(i));
                ImGui::TableNextRow();
                if (static_cast<int>(i) == s_selectedPoint)
                    ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, IM_COL32(255, 160, 115, 50));

                ImGui::TableSetColumnIndex(0);
                ImGui::AlignTextToFramePadding();
                ImGui::Text("%zu", i + 1);

                ImGui::TableSetColumnIndex(1);
                const std::string label = std::to_string(stageUid) + "  " + ProjectQueries::stage_name(ctx.project, stageUid);
                if (ImGui::TextLink(label.c_str())) ctx.defer([&ctx, stageUid]() { ctx.select({ ContentKind::STAGE, stageUid }); });

                for (int axis = 0; axis < 2; ++axis)
                {
                    const char* key = axis == 0 ? "x" : "y";
                    ImGui::TableSetColumnIndex(2 + axis);
                    ImGui::SetNextItemWidth(-FLT_MIN);
                    int value = read_int(point, key);
                    if (ImGui::DragInt(key[0] == 'x' ? "##x" : "##y", &value, 1.f, -10000, 10000)) point[key] = value;
                    if (ImGui::IsItemActivated()) s_selectedPoint = static_cast<int>(i);
                    if (ImGui::IsItemDeactivatedAfterEdit()) ctx.commit(CHAPTERS_FILE);
                }

                ImGui::TableSetColumnIndex(4);
                if (ImGui::ArrowButton("##up", ImGuiDir_Up)) { action = Action::UP; actionIndex = i; }
                ImGui::SameLine(0.f, 2.f);
                if (ImGui::ArrowButton("##down", ImGuiDir_Down)) { action = Action::DOWN; actionIndex = i; }
                ImGui::SameLine(0.f, 2.f);
                if (ImGui::Button("X")) { action = Action::REMOVE; actionIndex = i; }
                ImGui::SetItemTooltip("Remove from the map (the stage itself stays)");

                ImGui::PopID();
            }
            ImGui::EndTable();
        }

        switch (action)
        {
        case Action::UP:
            if (actionIndex > 0) { std::swap(points[actionIndex], points[actionIndex - 1]); s_selectedPoint = static_cast<int>(actionIndex) - 1; }
            break;
        case Action::DOWN:
            if (actionIndex + 1 < points.size()) { std::swap(points[actionIndex], points[actionIndex + 1]); s_selectedPoint = static_cast<int>(actionIndex) + 1; }
            break;
        case Action::REMOVE:
            points.erase(actionIndex);
            s_selectedPoint = -1;
            break;
        default:
            break;
        }
        if (action != Action::NONE) ctx.commit(CHAPTERS_FILE);

        if (ImGui::Button("+ Add stage")) ImGui::OpenPopup("AddStage");
        if (ImGui::BeginPopup("AddStage"))
        {
            // New points go next to the last one (or the middle of the map)
            ImVec2 position(640.f, 720.f);
            if (!points.empty())
                position = ImVec2(static_cast<float>(read_number(points.back(), "x")) + 200.f, static_cast<float>(read_number(points.back(), "y")));
            add_stage_menu(ctx, points, position);
            ImGui::EndPopup();
        }
        ImGui::SameLine();
        ImGui::TextDisabled("Or right click on the map");
    }
}

namespace ChapterEditor
{
    void draw(EditorContext& ctx)
    {
        const int uid = ctx.selection.uid;
        ordered_json* chapter = ctx.project.find_uid(CHAPTERS_FILE, uid);
        if (!chapter)
        {
            ImGui::TextDisabled("Chapter %d doesn't exist anymore.", uid);
            return;
        }

        // Point indices belong to one chapter
        if (s_chapterUid != uid)
        {
            s_chapterUid = uid;
            s_selectedPoint = -1;
            s_draggedPoint = -1;
        }

        ImGui::Text("%s", ProjectQueries::chapter_name(ctx.project, uid).c_str());
        ImGui::TextDisabled("Chapter #%d   chapters.json", uid);
        ImGui::Spacing();

        // ----- Categories (the open tab stays when switching chapters) -----
        if (!ImGui::BeginTabBar("ChapterCategories")) return;

        if (ImGui::BeginTabItem("General"))
        {
            ImGui::PushID(uid);
            if (begin_properties("General"))
            {
                text_field(ctx, CHAPTERS_FILE, "Name", *chapter, "name");
                text_field(ctx, CHAPTERS_FILE, "Description", *chapter, "description", nullptr, true);
                end_properties();
            }
            ImGui::PopID();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Map"))
        {
            ImGui::PushID(uid);
            if (begin_properties("MapImage"))
            {
                image_field(ctx, CHAPTERS_FILE, "Map image", *chapter, "mapTexture", ImageFolder::MAPS,
                    "Drawn x2 in the game: stage positions are in those pixels.");
                end_properties();
            }

            section("Stages on the map");
            ImGui::TextDisabled("Drag the stages to move them. Right click to add or remove one. The numbers are the path order.");
            draw_map(ctx, *chapter);
            draw_point_list(ctx, *chapter);
            ImGui::PopID();
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
}
