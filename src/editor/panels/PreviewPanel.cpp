#include "PreviewPanel.h"

#include "../EditorWidgets.h"
#include "../ProjectQueries.h"
#include "../../data/StageData.h"
#include "../../uis/widgets/UIStatusIcons.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

using namespace EditorWidgets;

namespace
{
    const ImU32 WALK_COLOR = IM_COL32(40, 170, 190, 255);
    const ImU32 KNOCKBACK_COLOR = IM_COL32(255, 160, 115, 255);
    const ImU32 ATTACK_COLOR = IM_COL32(230, 70, 80, 255);

    // Same colors as the developer tools' battlefield overlays
    const ImU32 HITBOX_COLOR = IM_COL32(80, 160, 255, 255);
    const ImU32 RANGE_COLOR = IM_COL32(255, 215, 70, 255);
    const ImU32 DAMAGE_COLOR = IM_COL32(255, 80, 80, 255);

    const char* PLAYER_BASE_TEXTURE = "assets/images/textures/bases/fishBaseTEST.png"; // Stage::spawn_bases

    constexpr float SCENE_WORLD_X = 960.f;   // World x of the edited entity (lines the background up like a battle)

    // Draws copies of a background between two world x, placed like Stage::init_background
    // (a copy every width x BACKGROUND_SCALE from -width, the ground row on BATTLE_GROUND_Y)
    template<typename ToScreen>
    void draw_background_copies(ImDrawList* drawList, const sf::Texture& background, float groundRow, float worldLeft, float worldRight, ToScreen toScreen)
    {
        const float tileWidth = background.getSize().x * BACKGROUND_SCALE;
        const float tileHeight = background.getSize().y * BACKGROUND_SCALE;
        const float top = BATTLE_GROUND_Y - groundRow * BACKGROUND_SCALE;
        const int first = static_cast<int>(std::floor((worldLeft + tileWidth) / tileWidth)) - 1;
        const int last = static_cast<int>(std::floor((worldRight + tileWidth) / tileWidth));
        for (int i = first; i <= last; ++i)
        {
            const float x = -tileWidth + i * tileWidth;
            drawList->AddImage(texture_id(background), toScreen(x, top), toScreen(x + tileWidth, top + tileHeight));
        }
    }

    ImU32 with_alpha(ImU32 color, int alpha)
    {
        return (color & 0x00FFFFFF) | (static_cast<ImU32>(alpha) << IM_COL32_A_SHIFT);
    }

    bool overlaps(float aMin, float aMax, float bMin, float bMax)
    {
        return std::max(aMin, bMin) < std::min(aMax, bMax); // Like sf::FloatRect::findIntersection
    }

    int frame_width(const sf::Texture* sheet, int frameCount)
    {
        return sheet ? static_cast<int>(sheet->getSize().x) / std::max(1, frameCount) : 100;
    }

    float sprite_height(const sf::Texture* sheet)
    {
        return sheet ? static_cast<float>(sheet->getSize().y) : 100.f;
    }
}

// ==================
// Panel
// ==================
void PreviewPanel::draw(EditorContext& ctx)
{
    const float deltaTime = std::min(m_clock.restart().asSeconds(), 0.25f); // The file dialog can block for a while

    // A new selection starts its animation from the first frame
    if (!(ctx.selection == m_lastSelection))
    {
        m_lastSelection = ctx.selection;
        m_frame = 0;
        m_opponentFrame = 0;
        m_frameTimer = 0.f;
    }
    if (m_isPlaying) m_frameTimer += deltaTime * m_speed;

    switch (ctx.selection.kind)
    {
    case ContentKind::UNIT:
    {
        ordered_json* unit = ctx.project.find_uid(DataFile::UNITS, ctx.selection.uid);
        if (!unit || !unit->contains("forms") || !(*unit)["forms"].is_array() || (*unit)["forms"].size() <= static_cast<size_t>(ctx.selection.form)) break;
        draw_entity(ctx, (*unit)["forms"][ctx.selection.form], DataFile::UNITS, ctx.selection);
        break;
    }
    case ContentKind::ENEMY:
        if (ordered_json* enemy = ctx.project.find_uid(DataFile::ENEMIES, ctx.selection.uid)) draw_entity(ctx, *enemy, DataFile::ENEMIES, ctx.selection);
        break;

    case ContentKind::STAGE:
        if (const ordered_json* stage = ctx.project.find_uid(DataFile::STAGES, ctx.selection.uid)) draw_stage(ctx, *stage);
        break;

    case ContentKind::IMAGE:
        if (const sf::Texture* texture = ctx.textures.get(ctx.selection.name))
        {
            const ImVec2 source(static_cast<float>(texture->getSize().x), static_cast<float>(texture->getSize().y));
            image_in_box(texture, fit(source, ImGui::GetContentRegionAvail()), {}, true);
        }
        break;

    case ContentKind::CHAPTER:
        ImGui::TextDisabled("The chapter map is edited in the Properties panel (Map tab).");
        break;

    default:
        ImGui::TextDisabled("Select a unit, an enemy or a stage to preview it.");
        break;
    }
}

// ==================
// Scene entities
// ==================
bool PreviewPanel::load_entity(EditorContext& ctx, const Selection& source, SceneEntity& entity) const
{
    const ordered_json* data = nullptr;
    if (source.kind == ContentKind::UNIT)
    {
        const ordered_json* unit = ctx.project.find_uid(DataFile::UNITS, source.uid);
        if (unit && unit->contains("forms") && (*unit)["forms"].is_array() && (*unit)["forms"].size() > static_cast<size_t>(source.form))
            data = &(*unit)["forms"][source.form];
    }
    else if (source.kind == ContentKind::ENEMY) data = ctx.project.find_uid(DataFile::ENEMIES, source.uid);
    if (!data) return false;

    entity.data = data;
    entity.source = source;
    entity.isUnit = source.kind == ContentKind::UNIT;
    entity.sheet = ctx.textures.get(read_string(*data, "texture"));
    entity.name = read_string(*data, "name", "?");
    entity.frameCount = std::max(1, read_int(*data, "frameCount", 1));
    entity.knockbackFrame = std::clamp(read_int(*data, "knockbackFrameIndex", 1), 0, entity.frameCount - 1);
    return true;
}

PreviewPanel::Zones PreviewPanel::zones_of(const SceneEntity& entity)
{
    // BattleUnit / BattleEnemy constructors: half a frame (integer division), damageZone = [offset, width]
    const float half = static_cast<float>(frame_width(entity.sheet, entity.frameCount) / 2);
    Zones zones;
    zones.range = static_cast<float>(read_number(*entity.data, "attackRange"));

    float offset = 0.f;
    float width = zones.range;
    auto damageZone = entity.data->find("damageZone");
    if (damageZone != entity.data->end() && damageZone->is_array() && damageZone->size() == 2 && (*damageZone)[0].is_number() && (*damageZone)[1].is_number())
    {
        offset = (*damageZone)[0].get<float>();
        width = (*damageZone)[1].get<float>();
    }

    const float c = entity.center;
    if (entity.isUnit)
    {
        // Faces left: hitbox is the back (right) half, range and damage go left
        zones.hitboxMin = c;                       zones.hitboxMax = c + half;
        zones.rangeMin = c - zones.range;          zones.rangeMax = c + half;
        zones.damageMin = c - offset - width;      zones.damageMax = c - offset;
    }
    else
    {
        // Faces right: hitbox is the back (left) half, range and damage go right
        zones.hitboxMin = c - half;                zones.hitboxMax = c;
        zones.rangeMin = c - half;                 zones.rangeMax = c + zones.range;
        zones.damageMin = c + offset;              zones.damageMax = c + offset + width;
    }
    return zones;
}

void PreviewPanel::step_frame(int& frame, Animation animation, int frameCount, int knockbackFrame)
{
    // Same rules as BattleEntity::update_sprite
    switch (animation)
    {
    case Animation::IDLE:
        frame = 0;
        break;
    case Animation::WALK:
        frame = frame < knockbackFrame - 1 ? frame + 1 : 0;
        break;
    case Animation::ATTACK:
        if (frame <= knockbackFrame) frame = knockbackFrame;
        frame = frame < frameCount - 1 ? frame + 1 : 0;
        break;
    case Animation::KNOCKBACK:
        frame = knockbackFrame;
        break;
    }
    frame = std::clamp(frame, 0, std::max(0, frameCount - 1));
}

std::string PreviewPanel::background_path(EditorContext& ctx) const
{
    if (m_background == NO_BACKGROUND) return "";
    if (!m_background.empty()) return m_background;
    const auto& backgrounds = ctx.assets.list(ImageFolder::STAGE_BACKGROUNDS);
    return backgrounds.empty() ? "" : backgrounds.front();
}

// ==================
// Unit / enemy preview
// ==================
void PreviewPanel::draw_entity(EditorContext& ctx, ordered_json& entity, DataFile file, const Selection& source)
{
    SceneEntity edited;
    load_entity(ctx, source, edited);
    if (!edited.sheet)
    {
        ImGui::TextColored(ImVec4(1.f, 0.45f, 0.4f, 1.f), "Missing sprite sheet: %s", read_string(entity, "texture").c_str());
        return;
    }

    // The opponent stands on the other side: an enemy in front of a unit, a unit in front of an enemy
    SceneEntity opponent;
    bool hasOpponent = m_opponent.kind != ContentKind::NONE && load_entity(ctx, m_opponent, opponent) && opponent.isUnit != edited.isUnit;
    if (m_opponent.kind != ContentKind::NONE && !hasOpponent) m_opponent = {};

    // ----- Animation (both entities on the same clock) -----
    while (m_frameTimer >= FRAME_TIME)
    {
        m_frameTimer -= FRAME_TIME;
        step_frame(m_frame, m_animation, edited.frameCount, edited.knockbackFrame);
        if (hasOpponent) step_frame(m_opponentFrame, m_animation, opponent.frameCount, opponent.knockbackFrame);
    }
    m_frame = std::clamp(m_frame, 0, edited.frameCount - 1);
    m_opponentFrame = std::clamp(m_opponentFrame, 0, opponent.frameCount - 1);

    edited.frame = m_frame;
    edited.center = 0.f;
    opponent.frame = m_opponentFrame;
    opponent.center = edited.isUnit ? -m_distance : m_distance;

    const float opponentRange = hasOpponent ? zones_of(opponent).range : 0.f;
    draw_toolbar(ctx, edited.isUnit, zones_of(edited).range, opponentRange, hasOpponent);

    // ----- Scene -----
    const float reserved = (hasOpponent ? 64.f : 26.f) + ImGui::GetFrameHeightWithSpacing() * 1.2f; // Readout and the Frames header
    const float height = std::clamp(ImGui::GetContentRegionAvail().y - reserved, 200.f, 560.f);
    draw_scene(ctx, edited, hasOpponent ? &opponent : nullptr, height);

    if (hasOpponent) draw_engagement(ctx, edited, opponent);
    else ImGui::TextDisabled("Frame size %d x %u. Add an opponent to tune the ranges against another entity.",
        frame_width(edited.sheet, edited.frameCount), edited.sheet->getSize().y);

    if (ImGui::CollapsingHeader("Frames"))
        draw_frame_strip(ctx, entity, file, *edited.sheet);
}

void PreviewPanel::draw_toolbar(EditorContext& ctx, bool isEditingUnit, float editedRange, float opponentRange, bool hasOpponent)
{
    // ----- Animation -----
    const char* names[] = { "Idle", "Walk", "Attack", "Knockback" };
    for (int i = 0; i < 4; ++i)
    {
        if (i > 0) ImGui::SameLine();
        if (ImGui::RadioButton(names[i], m_animation == static_cast<Animation>(i)))
        {
            m_animation = static_cast<Animation>(i);
            m_isPlaying = true;
        }
    }
    ImGui::SameLine(0.f, 16.f);
    if (ImGui::Button(m_isPlaying ? "Pause" : "Play", ImVec2(64.f, 0.f))) m_isPlaying = !m_isPlaying;
    ImGui::SameLine();
    ImGui::SetNextItemWidth(110.f);
    ImGui::SliderFloat("##speed", &m_speed, 0.1f, 3.f, "speed x%.1f");

    // ----- Background and overlays -----
    background_picker(ctx);
    ImGui::SameLine(0.f, 14.f);
    ImGui::Checkbox("Hitbox", &m_isShowingHitboxes);
    ImGui::SetItemTooltip("Blue: the body the enemy's attacks look for (the back half of the sprite)");
    ImGui::SameLine();
    ImGui::Checkbox("Range", &m_isShowingRanges);
    ImGui::SetItemTooltip("Yellow: it stops and attacks when a target's hitbox enters it");
    ImGui::SameLine();
    ImGui::Checkbox("Damage", &m_isShowingDamageZones);
    ImGui::SetItemTooltip("Red: where the attack actually hits");
    ImGui::SameLine();
    ImGui::Checkbox("Statuses", &m_isShowingStatusIcons);
    ImGui::SetItemTooltip("Every status icon under the entities, like in battle");

    // ----- Opponent -----
    opponent_picker(ctx, isEditingUnit);
    if (hasOpponent)
    {
        ImGui::SameLine();
        ImGui::SetNextItemWidth(130.f);
        ImGui::DragFloat("##distance", &m_distance, 1.f, 0.f, 3000.f, "distance %.0f", ImGuiSliderFlags_AlwaysClamp);
        ImGui::SetItemTooltip("Between the two centers, in game pixels. You can also drag the opponent in the scene.");
        ImGui::SameLine();
        if (ImGui::Button("My range")) m_distance = std::max(0.f, editedRange - 1.f);
        ImGui::SetItemTooltip("Puts it just inside this entity's attack range");
        ImGui::SameLine();
        if (ImGui::Button("Its range")) m_distance = std::max(0.f, opponentRange - 1.f);
        ImGui::SetItemTooltip("Puts it just inside the opponent's attack range");
    }
}

bool PreviewPanel::background_picker(EditorContext& ctx)
{
    const std::string current = background_path(ctx);
    const sf::Texture* currentTexture = ctx.textures.get(current);

    // Button: thumbnail of the background, opens the list
    const ImVec2 size(76.f, ImGui::GetFrameHeight());
    const ImVec2 min = ImGui::GetCursorScreenPos();
    const ImVec2 max(min.x + size.x, min.y + size.y);
    if (ImGui::InvisibleButton("##backgroundButton", size)) ImGui::OpenPopup("BackgroundPicker");
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    drawList->AddRectFilled(min, max, ImGui::GetColorU32(ImGui::IsItemHovered() ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg), 4.f);
    if (currentTexture) drawList->AddImage(texture_id(*currentTexture), min, max);
    else drawList->AddText(ImVec2(min.x + 6.f, min.y + ImGui::GetStyle().FramePadding.y), IM_COL32(200, 210, 215, 255), "None");
    drawList->AddRect(min, max, IM_COL32(255, 255, 255, 90), 4.f);
    ImGui::SetItemTooltip("Background: %s\nClick to change it", current.empty() ? "none" : current.c_str());

    bool changed = false;
    if (ImGui::BeginPopup("BackgroundPicker"))
    {
        ImGui::TextDisabled("Stage backgrounds (%s)", AssetLibrary::folder(ImageFolder::STAGE_BACKGROUNDS));
        if (tile("##none", nullptr, {}, "None", current.empty(), ImVec2(128.f, 100.f)))
        {
            m_background = NO_BACKGROUND;
            changed = true;
            ImGui::CloseCurrentPopup();
        }
        const std::vector<std::string> images = ctx.assets.list(ImageFolder::STAGE_BACKGROUNDS);
        for (size_t i = 0; i < images.size(); ++i)
        {
            if ((i + 1) % 4 != 0) ImGui::SameLine(); // 4 tiles per row, "None" first
            ImGui::PushID(static_cast<int>(i));
            if (tile("##image", ctx.textures.get(images[i]), {}, images[i].substr(images[i].find_last_of('/') + 1), images[i] == current, ImVec2(128.f, 100.f)))
            {
                m_background = images[i];
                changed = true;
                ImGui::CloseCurrentPopup();
            }
            ImGui::PopID();
        }
        ImGui::EndPopup();
    }
    return changed;
}

bool PreviewPanel::opponent_picker(EditorContext& ctx, bool isEditingUnit)
{
    // Button: the opponent's icon and name
    std::string label = "Opponent: none";
    if (m_opponent.kind == ContentKind::UNIT) label = "Opponent: " + ProjectQueries::unit_name(ctx.project, m_opponent.uid, m_opponent.form);
    if (m_opponent.kind == ContentKind::ENEMY) label = "Opponent: " + ProjectQueries::enemy_name(ctx.project, m_opponent.uid);
    if (ImGui::Button((label + "...").c_str())) ImGui::OpenPopup("OpponentPicker");
    ImGui::SetItemTooltip("Puts another entity in front of this one, to compare ranges");

    if (m_opponent.kind != ContentKind::NONE)
    {
        ImGui::SameLine();
        if (ImGui::SmallButton("X")) m_opponent = {};
        ImGui::SetItemTooltip("Remove the opponent");
    }

    bool changed = false;
    if (!ImGui::BeginPopup("OpponentPicker")) return false;

    ImGui::TextDisabled(isEditingUnit ? "An enemy in front of this unit" : "A unit (any form) in front of this enemy");
    const ImVec2 tileSize(100.f, 96.f);
    int index = 0;
    auto pick = [&](const Selection& candidate, const sf::Texture* texture, FrameUV uv, const std::string& name)
        {
            if (index > 0 && index % 5 != 0) ImGui::SameLine();
            ImGui::PushID(index++);
            if (tile("##opponent", texture, uv, name, candidate == m_opponent, tileSize))
            {
                m_opponent = candidate;
                m_opponentFrame = 0;
                changed = true;
                ImGui::CloseCurrentPopup();
            }
            ImGui::PopID();
        };

    if (isEditingUnit)
    {
        for (int uid : ctx.project.sorted_uids(DataFile::ENEMIES))
        {
            FrameUV uv;
            const sf::Texture* texture = enemy_texture(ctx, uid, uv);
            pick({ ContentKind::ENEMY, uid }, texture, uv, ProjectQueries::enemy_name(ctx.project, uid));
        }
    }
    else
    {
        for (int uid : ctx.project.sorted_uids(DataFile::UNITS))
        {
            const ordered_json* unit = ctx.project.find_uid(DataFile::UNITS, uid);
            const size_t forms = unit->contains("forms") && (*unit)["forms"].is_array() ? (*unit)["forms"].size() : 0;
            for (size_t form = 0; form < forms; ++form)
            {
                FrameUV uv;
                const sf::Texture* texture = unit_texture(ctx, uid, static_cast<int>(form), uv);
                pick({ ContentKind::UNIT, uid, static_cast<int>(form) }, texture, uv, ProjectQueries::unit_name(ctx.project, uid, static_cast<int>(form)));
            }
        }
    }
    ImGui::EndPopup();
    return changed;
}

void PreviewPanel::draw_scene(EditorContext& ctx, const SceneEntity& edited, const SceneEntity* opponent, float height)
{
    const float width = std::max(200.f, ImGui::GetContentRegionAvail().x);
    const Zones editedZones = zones_of(edited);
    const Zones opponentZones = opponent ? zones_of(*opponent) : Zones{};

    // ----- View: both sprites and their zones, standing near the bottom -----
    if (!m_isDraggingOpponent)
    {
        float minX = 1e9f, maxX = -1e9f, tallest = 0.f;
        auto include = [&](const SceneEntity& entity, const Zones& zones)
            {
                const float half = frame_width(entity.sheet, entity.frameCount) / 2.f;
                minX = std::min(minX, entity.center - half);
                maxX = std::max(maxX, entity.center + half);
                if (m_isShowingRanges) { minX = std::min(minX, zones.rangeMin); maxX = std::max(maxX, zones.rangeMax); }
                if (m_isShowingDamageZones) { minX = std::min(minX, zones.damageMin); maxX = std::max(maxX, zones.damageMax); }
                tallest = std::max(tallest, sprite_height(entity.sheet));
            };
        include(edited, editedZones);
        if (opponent) include(*opponent, opponentZones);

        const float worldWidth = std::max(320.f, maxX - minX + 120.f);
        const float worldHeight = tallest + 100.f + 90.f;  // Room for the labels above, status icons and the ruler below
        m_viewScale = std::min(width / worldWidth, height / worldHeight);
        m_viewLeft = (minX + maxX) / 2.f - width / m_viewScale / 2.f;
        m_viewTop = GROUND_Y + 90.f - height / m_viewScale;
    }
    const float scale = m_viewScale;

    const ImVec2 origin = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton("##scene", ImVec2(width, height));
    const bool isHovered = ImGui::IsItemHovered();
    const ImVec2 mouse = ImGui::GetIO().MousePos;

    auto toScreen = [&](float x, float y) { return ImVec2(origin.x + (x - m_viewLeft) * scale, origin.y + (y - m_viewTop) * scale); };
    const float worldRight = m_viewLeft + width / scale;

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    drawList->PushClipRect(origin, ImVec2(origin.x + width, origin.y + height), true);
    drawList->AddRectFilled(origin, ImVec2(origin.x + width, origin.y + height), IM_COL32(8, 16, 20, 255));

    // ----- Background, placed like Stage::init_background (ground line of the stages using it) -----
    const std::string backgroundPath = background_path(ctx);
    if (const sf::Texture* background = ctx.textures.get(backgroundPath))
    {
        const float groundRow = ProjectQueries::background_ground_row(ctx.project, backgroundPath, static_cast<float>(background->getSize().y));
        draw_background_copies(drawList, *background, groundRow, m_viewLeft + SCENE_WORLD_X, worldRight + SCENE_WORLD_X,
            [&](float x, float y) { return toScreen(x - SCENE_WORLD_X, y); });
    }

    const float tallest = std::max(sprite_height(edited.sheet), opponent ? sprite_height(opponent->sheet) : 0.f);
    const float zoneTop = GROUND_Y - tallest - 40.f;

    // ----- Zone fills (behind the sprites) -----
    auto fillZones = [&](const Zones& zones, int alpha)
        {
            if (m_isShowingRanges) drawList->AddRectFilled(toScreen(zones.rangeMin, zoneTop), toScreen(zones.rangeMax, GROUND_Y), with_alpha(RANGE_COLOR, alpha / 2));
            if (m_isShowingDamageZones) drawList->AddRectFilled(toScreen(zones.damageMin, zoneTop + 20.f), toScreen(zones.damageMax, GROUND_Y), with_alpha(DAMAGE_COLOR, alpha));
            if (m_isShowingHitboxes) drawList->AddRectFilled(toScreen(zones.hitboxMin, zoneTop + 40.f), toScreen(zones.hitboxMax, GROUND_Y), with_alpha(HITBOX_COLOR, alpha));
        };
    if (opponent) fillZones(opponentZones, 30);
    fillZones(editedZones, 55);

    // Ground line
    drawList->AddLine(toScreen(m_viewLeft, GROUND_Y), toScreen(worldRight, GROUND_Y), IM_COL32(255, 255, 255, 60), 1.f);

    // ----- Sprites (the edited one in front) -----
    auto drawSprite = [&](const SceneEntity& entity)
        {
            const FrameUV uv = frame_uv(*entity.sheet, entity.frame, entity.frameCount);
            const float half = static_cast<float>(frame_width(entity.sheet, entity.frameCount) / 2);
            const ImVec2 min = toScreen(entity.center - half, GROUND_Y - uv.size.y);
            const ImVec2 max = toScreen(entity.center - half + uv.size.x, GROUND_Y);
            drawList->AddImage(texture_id(*entity.sheet), min, max, uv.uv0, uv.uv1);
        };
    if (opponent && opponent->sheet) drawSprite(*opponent);
    drawSprite(edited);

    // ----- Zone outlines and labels -----
    auto outlineZones = [&](const Zones& zones, float labelY, int alpha, const std::string& owner)
        {
            auto outline = [&](float minX, float maxX, float top, ImU32 color, const std::string& label)
                {
                    const ImVec2 min = toScreen(minX, top);
                    const ImVec2 max = toScreen(maxX, GROUND_Y);
                    drawList->AddRect(min, max, with_alpha(color, alpha), 0.f, 0, 1.5f);
                    drawList->AddText(ImVec2(min.x + 3.f, toScreen(0.f, labelY).y), with_alpha(color, alpha), label.c_str());
                };
            char text[64];
            if (m_isShowingRanges)
            {
                std::snprintf(text, sizeof(text), "%srange %.0f", owner.c_str(), zones.range);
                outline(zones.rangeMin, zones.rangeMax, zoneTop, RANGE_COLOR, text);
            }
            if (m_isShowingDamageZones) outline(zones.damageMin, zones.damageMax, zoneTop + 20.f, DAMAGE_COLOR, "");
            if (m_isShowingHitboxes) outline(zones.hitboxMin, zones.hitboxMax, zoneTop + 40.f, HITBOX_COLOR, "");
        };
    if (opponent) outlineZones(opponentZones, zoneTop - 34.f, 150, "its ");
    outlineZones(editedZones, zoneTop - 18.f, 255, "");

    // ----- Status icons on the sprite's origin, as UIStatusIcons draws them -----
    if (m_isShowingStatusIcons)
    {
        std::vector<std::pair<const sf::Texture*, FrameUV>> icons;
        for (const auto& [key, status] : ctx.project.get(DataFile::STATUSES).items())
        {
            FrameUV uv;
            if (const sf::Texture* icon = status_icon(ctx, read_int(status, "iconIndex", -1), uv)) icons.push_back({ icon, uv });
        }
        auto drawIcons = [&](const SceneEntity& entity)
            {
                // Origin: bottom of the frame's center (fish: its bottom-left corner, grows right;
                // enemy: its bottom-right corner, grows left)
                const float size = UIStatusIcons::ICON_SIZE;
                const float step = entity.isUnit ? size + UIStatusIcons::ICON_GAP : -(size + UIStatusIcons::ICON_GAP);
                float x = entity.isUnit ? entity.center : entity.center - size;
                const float y = GROUND_Y - size;
                for (const auto& [texture, uv] : icons)
                {
                    drawList->AddImage(texture_id(*texture), toScreen(x, y), toScreen(x + size, y + size), uv.uv0, uv.uv1);
                    x += step;
                }
            };
        if (!icons.empty())
        {
            if (opponent) drawIcons(*opponent);
            drawIcons(edited);
        }
    }

    // ----- Ruler: distance from this entity's center -----
    const float rulerY = origin.y + height - 18.f;
    float step = 10.f;
    for (float candidate : { 10.f, 25.f, 50.f, 100.f, 200.f, 500.f, 1000.f })
    {
        step = candidate;
        if (candidate * scale >= 46.f) break;
    }
    drawList->AddRectFilled(ImVec2(origin.x, rulerY - 4.f), ImVec2(origin.x + width, origin.y + height), IM_COL32(8, 16, 20, 200));
    for (float x = std::floor(m_viewLeft / step) * step; x <= worldRight; x += step)
    {
        const float screenX = toScreen(x, 0.f).x;
        drawList->AddLine(ImVec2(screenX, rulerY - 4.f), ImVec2(screenX, rulerY + 2.f), IM_COL32(200, 210, 215, 160));
        drawList->AddText(ImVec2(screenX + 2.f, rulerY), IM_COL32(200, 210, 215, 200), std::to_string(static_cast<int>(std::abs(x))).c_str());
    }

    // Distance between the two centers
    if (opponent)
    {
        const float y = GROUND_Y + 60.f;
        const ImVec2 a = toScreen(edited.center, y);
        const ImVec2 b = toScreen(opponent->center, y);
        drawList->AddLine(a, b, IM_COL32_WHITE, 1.5f);
        drawList->AddCircleFilled(a, 3.f, IM_COL32_WHITE);
        drawList->AddCircleFilled(b, 3.f, IM_COL32_WHITE);
        const std::string label = std::to_string(static_cast<int>(m_distance));
        drawList->AddText(ImVec2((a.x + b.x) / 2.f - 10.f, a.y - ImGui::GetTextLineHeight() - 1.f), IM_COL32_WHITE, label.c_str());
    }
    drawList->PopClipRect();

    // ----- Dragging the opponent changes the distance -----
    if (opponent && opponent->sheet)
    {
        const float half = frame_width(opponent->sheet, opponent->frameCount) / 2.f;
        const ImVec2 min = toScreen(opponent->center - half, GROUND_Y - sprite_height(opponent->sheet));
        const ImVec2 max = toScreen(opponent->center + half, GROUND_Y);
        const bool isOverOpponent = mouse.x >= min.x && mouse.x <= max.x && mouse.y >= min.y && mouse.y <= max.y;

        if (ImGui::IsItemActivated() && isOverOpponent) m_isDraggingOpponent = true;
        if (m_isDraggingOpponent && ImGui::IsItemActive())
        {
            const float mouseWorldX = m_viewLeft + (mouse.x - origin.x) / scale;
            m_distance = std::max(0.f, edited.isUnit ? edited.center - mouseWorldX : mouseWorldX - edited.center);
        }
        if (!ImGui::IsItemActive()) m_isDraggingOpponent = false;
        if (isHovered && (isOverOpponent || m_isDraggingOpponent)) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
    }
}

void PreviewPanel::draw_engagement(EditorContext& ctx, const SceneEntity& edited, const SceneEntity& opponent)
{
    // Same tests as the states: attackRangeZone vs hitbox to start attacking, damageZone vs hitbox to hit
    auto line = [&](const SceneEntity& attacker, const SceneEntity& target)
        {
            const Zones attackerZones = zones_of(attacker);
            const Zones targetZones = zones_of(target);
            const bool isInRange = overlaps(attackerZones.rangeMin, attackerZones.rangeMax, targetZones.hitboxMin, targetZones.hitboxMax);
            const bool isHitting = overlaps(attackerZones.damageMin, attackerZones.damageMax, targetZones.hitboxMin, targetZones.hitboxMax);

            image_in_box(attacker.sheet, ImVec2(28.f, 22.f), frame_uv(*attacker.sheet, 0, attacker.frameCount));
            ImGui::SameLine();
            ImGui::Text("%s attacks under %.0f px:", attacker.name.c_str(), attackerZones.range);
            ImGui::SameLine();
            if (!isInRange) ImGui::TextDisabled("out of range");
            else if (isHitting) ImGui::TextColored(ImVec4(0.45f, 0.9f, 0.6f, 1.f), "in range, its attack hits");
            else ImGui::TextColored(ImVec4(1.f, 0.7f, 0.3f, 1.f), "in range, but its damage zone misses");
        };

    line(edited, opponent);
    if (opponent.sheet) line(opponent, edited);
}

void PreviewPanel::draw_frame_strip(EditorContext& ctx, ordered_json& entity, DataFile file, const sf::Texture& sheet)
{
    const int frameCount = std::max(1, read_int(entity, "frameCount", 1));
    const int knockbackFrame = read_int(entity, "knockbackFrameIndex", 1);

    ImGui::TextDisabled("Walk 0-%d, knockback %d, attack %d-%d (%.1f s per frame). Click a frame to show it.",
        knockbackFrame - 1, knockbackFrame, knockbackFrame + 1, frameCount - 1, FRAME_TIME);

    const FrameUV first = frame_uv(sheet, 0, frameCount);
    const float thumbHeight = 64.f;
    const float thumbWidth = std::clamp(first.size.x / std::max(1.f, first.size.y) * thumbHeight, 32.f, 160.f);

    ImGui::BeginChild("FrameStrip", ImVec2(0.f, thumbHeight + 52.f), ImGuiChildFlags_Borders, ImGuiWindowFlags_HorizontalScrollbar);
    for (int frame = 0; frame < frameCount; ++frame)
    {
        if (frame > 0) ImGui::SameLine(0.f, 4.f);
        ImGui::PushID(frame);

        const ImVec2 min = ImGui::GetCursorScreenPos();
        if (ImGui::InvisibleButton("##frame", ImVec2(thumbWidth, thumbHeight + 18.f)))
        {
            m_frame = frame;
            m_isPlaying = false;
        }
        const bool isHovered = ImGui::IsItemHovered();

        ImDrawList* drawList = ImGui::GetWindowDrawList();
        const ImU32 color = frame < knockbackFrame ? WALK_COLOR : frame == knockbackFrame ? KNOCKBACK_COLOR : ATTACK_COLOR;
        const ImVec2 max(min.x + thumbWidth, min.y + thumbHeight);
        drawList->AddRectFilled(min, max, frame == m_frame ? IM_COL32(40, 70, 80, 255) : IM_COL32(8, 16, 20, 255), 4.f);
        draw_image_in_rect(drawList, &sheet, min, max, frame_uv(sheet, frame, frameCount));
        drawList->AddRectFilled(ImVec2(min.x, max.y + 2.f), ImVec2(max.x, max.y + 6.f), color, 2.f);
        if (frame == m_frame || isHovered) drawList->AddRect(min, max, IM_COL32_WHITE, 4.f);
        drawList->AddText(ImVec2(min.x + 3.f, max.y + 7.f), IM_COL32(200, 210, 215, 255), std::to_string(frame).c_str());

        // Right click: make it the knockback frame
        if (ImGui::BeginPopupContextItem("FrameMenu"))
        {
            ImGui::BeginDisabled(frame < 1 || frame >= frameCount);
            if (ImGui::MenuItem("Make it the knockback frame"))
            {
                entity["knockbackFrameIndex"] = frame;
                ctx.commit(file);
            }
            ImGui::EndDisabled();
            ImGui::EndPopup();
        }
        ImGui::SetItemTooltip("Frame %d: %s\nClick to show it, right click to make it the knockback frame", frame,
            frame < knockbackFrame ? "walk" : frame == knockbackFrame ? "knockback" : "attack");

        ImGui::PopID();
    }
    ImGui::EndChild();

    ImGui::BeginDisabled(m_frame < 1 || m_frame == knockbackFrame);
    if (ImGui::Button(("Make frame " + std::to_string(m_frame) + " the knockback frame").c_str()))
    {
        entity["knockbackFrameIndex"] = m_frame;
        ctx.commit(file);
    }
    ImGui::EndDisabled();
}

// ==================
// Stage
// ==================
void PreviewPanel::draw_stage(EditorContext& ctx, const ordered_json& stage)
{
    const sf::Texture* background = ctx.textures.get(read_string(stage, "backgroundTexture"));
    const sf::Texture* enemyBase = ctx.textures.get(read_string(stage, "baseTexture"));
    const sf::Texture* playerBase = ctx.textures.get(PLAYER_BASE_TEXTURE);

    // The whole stage in world coordinates, the camera's height at the default zoom (y 54 to 1026)
    const float length = std::max(100.f, static_cast<float>(read_number(stage, "length", 960.0)));
    const float worldLeft = -300.f;
    const float worldRight = length + 500.f;
    const float worldTop = 54.f;
    const float worldBottom = 1026.f;

    const float width = std::max(200.f, ImGui::GetContentRegionAvail().x);
    const float scale = width / (worldRight - worldLeft);
    const ImVec2 size(width, (worldBottom - worldTop) * scale);
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    ImGui::Dummy(size);
    auto toScreen = [&](float x, float y) { return ImVec2(origin.x + (x - worldLeft) * scale, origin.y + (y - worldTop) * scale); };

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    drawList->PushClipRect(origin, ImVec2(origin.x + size.x, origin.y + size.y), true);
    drawList->AddRectFilled(origin, ImVec2(origin.x + size.x, origin.y + size.y), IM_COL32(8, 16, 20, 255));

    // Background copies with the stage's ground line, like Stage::init_background
    if (background)
    {
        const auto row = stage.find("backgroundGroundRow");
        const float groundRow = background_ground_row(row != stage.end() && row->is_number() ? row->get<float>() : -1.f,
            static_cast<float>(background->getSize().y));
        draw_background_copies(drawList, *background, groundRow, worldLeft, worldRight, toScreen);
    }

    // Bases standing on the ground: enemy at x 0 (left), player at the stage length (right), like Stage::spawn_bases
    auto drawBase = [&](const sf::Texture* texture, float worldX)
        {
            if (!texture) return;
            const float height = static_cast<float>(texture->getSize().y);
            drawList->AddImage(texture_id(*texture), toScreen(worldX, BATTLE_GROUND_Y - height),
                toScreen(worldX + texture->getSize().x, BATTLE_GROUND_Y));
        };
    drawBase(enemyBase, 0.f);
    drawBase(playerBase, length);

    // Ground line and length
    drawList->AddLine(toScreen(worldLeft, BATTLE_GROUND_Y), toScreen(worldRight, BATTLE_GROUND_Y), IM_COL32(255, 80, 80, 150), 1.f);
    const ImVec2 left = toScreen(0.f, BATTLE_GROUND_Y + 160.f);
    const ImVec2 right = toScreen(length, BATTLE_GROUND_Y + 160.f);
    drawList->AddLine(left, right, IM_COL32(255, 160, 115, 255), 2.f);
    const std::string label = "length " + std::to_string(static_cast<int>(length));
    const ImVec2 labelSize = ImGui::CalcTextSize(label.c_str());
    drawList->AddText(ImVec2((left.x + right.x - labelSize.x) / 2.f, left.y - labelSize.y - 2.f), IM_COL32(255, 160, 115, 255), label.c_str());
    drawList->PopClipRect();

    ImGui::TextDisabled("The whole stage at the default camera height. Red: the ground the units and bases stand on.");
}
