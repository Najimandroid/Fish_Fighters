#include "PreviewPanel.h"

#include "../EditorWidgets.h"

#include <algorithm>
#include <cmath>
#include <string>

using namespace EditorWidgets;

namespace
{
    const ImU32 WALK_COLOR = IM_COL32(40, 170, 190, 255);
    const ImU32 KNOCKBACK_COLOR = IM_COL32(255, 160, 115, 255);
    const ImU32 ATTACK_COLOR = IM_COL32(230, 70, 80, 255);

    const char* PLAYER_BASE_TEXTURE = "assets/images/textures/bases/fishBaseTEST.png"; // Stage::spawn_bases
}

void PreviewPanel::draw(EditorContext& ctx)
{
    const float deltaTime = std::min(m_clock.restart().asSeconds(), 0.25f); // The file dialog can block for a while

    // A new selection starts its animation from the first frame
    if (!(ctx.selection == m_lastSelection))
    {
        m_lastSelection = ctx.selection;
        m_frame = 0;
        m_frameTimer = 0.f;
    }
    if (m_isPlaying) m_frameTimer += deltaTime * m_speed;

    switch (ctx.selection.kind)
    {
    case ContentKind::UNIT:
    {
        ordered_json* unit = ctx.project.find_uid(DataFile::UNITS, ctx.selection.uid);
        if (!unit || !unit->contains("forms") || !(*unit)["forms"].is_array() || (*unit)["forms"].size() <= static_cast<size_t>(ctx.selection.form)) break;
        draw_animation(ctx, (*unit)["forms"][ctx.selection.form], DataFile::UNITS);
        break;
    }
    case ContentKind::ENEMY:
        if (ordered_json* enemy = ctx.project.find_uid(DataFile::ENEMIES, ctx.selection.uid)) draw_animation(ctx, *enemy, DataFile::ENEMIES);
        break;

    case ContentKind::STAGE:
        if (const ordered_json* stage = ctx.project.find_uid(DataFile::STAGES, ctx.selection.uid)) draw_stage(ctx, *stage);
        break;

    case ContentKind::BASE:
    case ContentKind::BACKGROUND:
        if (const sf::Texture* texture = ctx.textures.get(ctx.selection.name))
        {
            const ImVec2 source(static_cast<float>(texture->getSize().x), static_cast<float>(texture->getSize().y));
            image_in_box(texture, fit(source, ImGui::GetContentRegionAvail()), {}, true);
        }
        break;

    case ContentKind::CHAPTER:
        ImGui::TextDisabled("The chapter map is edited in the Properties panel.");
        break;

    default:
        ImGui::TextDisabled("Select a unit, an enemy or a stage to preview it.");
        break;
    }
}

// ==================
// Animation
// ==================
void PreviewPanel::step_animation(int frameCount, int knockbackFrame)
{
    // Same rules as BattleEntity::update_sprite
    while (m_frameTimer >= FRAME_TIME)
    {
        m_frameTimer -= FRAME_TIME;
        switch (m_animation)
        {
        case Animation::IDLE:
            m_frame = 0;
            break;
        case Animation::WALK:
            m_frame = m_frame < knockbackFrame - 1 ? m_frame + 1 : 0;
            break;
        case Animation::ATTACK:
            if (m_frame <= knockbackFrame) m_frame = knockbackFrame;
            m_frame = m_frame < frameCount - 1 ? m_frame + 1 : 0;
            break;
        case Animation::KNOCKBACK:
            m_frame = knockbackFrame;
            break;
        }
    }
    m_frame = std::clamp(m_frame, 0, std::max(0, frameCount - 1));
}

void PreviewPanel::draw_animation(EditorContext& ctx, ordered_json& entity, DataFile file)
{
    const sf::Texture* sheet = ctx.textures.get(read_string(entity, "texture"));
    if (!sheet)
    {
        ImGui::TextColored(ImVec4(1.f, 0.45f, 0.4f, 1.f), "Missing sprite sheet: %s", read_string(entity, "texture").c_str());
        return;
    }

    const int frameCount = std::max(1, read_int(entity, "frameCount", 1));
    const int knockbackFrame = std::clamp(read_int(entity, "knockbackFrameIndex", 1), 0, frameCount - 1);
    step_animation(frameCount, knockbackFrame);

    // ----- Animation choice -----
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

    if (ImGui::Button(m_isPlaying ? "Pause" : "Play", ImVec2(70.f, 0.f))) m_isPlaying = !m_isPlaying;
    ImGui::SameLine();
    ImGui::SetNextItemWidth(140.f);
    ImGui::SliderFloat("Speed", &m_speed, 0.1f, 3.f, "x%.1f");
    ImGui::SameLine();
    ImGui::TextDisabled("Frame %d / %d", m_frame, frameCount - 1);

    // ----- Current frame -----
    const FrameUV uv = frame_uv(*sheet, m_frame, frameCount);
    const float height = std::clamp(ImGui::GetContentRegionAvail().y - 190.f, 120.f, 420.f);
    const ImVec2 box(ImGui::GetContentRegionAvail().x, height);
    const ImVec2 min = ImGui::GetCursorScreenPos();
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    drawList->AddRectFilled(min, ImVec2(min.x + box.x, min.y + box.y), IM_COL32(8, 16, 20, 255), 6.f);

    // Ground line, so the walk cycle reads like on the battlefield
    drawList->AddLine(ImVec2(min.x, min.y + box.y - 12.f), ImVec2(min.x + box.x, min.y + box.y - 12.f), IM_COL32(255, 255, 255, 30), 2.f);
    image_in_box(sheet, ImVec2(box.x, box.y - 12.f), uv);
    ImGui::Dummy(ImVec2(1.f, 12.f));

    ImGui::TextDisabled("Frame size %d x %u.  Walk 0-%d, knockback %d, attack %d-%d (%.1f s per frame).",
        static_cast<int>(sheet->getSize().x) / frameCount, sheet->getSize().y,
        knockbackFrame - 1, knockbackFrame, knockbackFrame + 1, frameCount - 1, FRAME_TIME);

    draw_frame_strip(ctx, entity, file, *sheet);
}

void PreviewPanel::draw_frame_strip(EditorContext& ctx, ordered_json& entity, DataFile file, const sf::Texture& sheet)
{
    const int frameCount = std::max(1, read_int(entity, "frameCount", 1));
    const int knockbackFrame = read_int(entity, "knockbackFrameIndex", 1);

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

    const float length = std::max(100.f, static_cast<float>(read_number(stage, "length", 960.0)));
    const float worldHeight = 1080.f;                      // The game's view height
    const ImVec2 worldSize(length + 400.f, worldHeight);   // A bit of room around the bases

    const float width = ImGui::GetContentRegionAvail().x;
    const float scale = width / worldSize.x;
    const ImVec2 size(width, worldSize.y * scale);
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    ImGui::Dummy(size);

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    drawList->PushClipRect(origin, ImVec2(origin.x + size.x, origin.y + size.y), true);
    drawList->AddRectFilled(origin, ImVec2(origin.x + size.x, origin.y + size.y), IM_COL32(8, 16, 20, 255));

    // Background repeated along the stage, at the view's height
    if (background)
    {
        const float tileHeight = size.y;
        const float tileWidth = background->getSize().x * (tileHeight / background->getSize().y);
        for (float x = origin.x; x < origin.x + size.x; x += tileWidth)
            drawList->AddImage(texture_id(*background), ImVec2(x, origin.y), ImVec2(x + tileWidth, origin.y + tileHeight));
    }

    // Bases at the middle of the view: enemy at 0 (left), player at the stage length (right)
    auto drawBase = [&](const sf::Texture* texture, float worldX)
        {
            if (!texture) return;
            const ImVec2 baseSize(texture->getSize().x * scale, texture->getSize().y * scale);
            const ImVec2 min(origin.x + (200.f + worldX) * scale - baseSize.x / 2.f, origin.y + size.y / 2.f - baseSize.y / 2.f);
            drawList->AddImage(texture_id(*texture), min, ImVec2(min.x + baseSize.x, min.y + baseSize.y));
        };
    drawBase(enemyBase, 0.f);
    drawBase(playerBase, length);

    // Length
    const float y = origin.y + size.y * 0.8f;
    const float left = origin.x + 200.f * scale;
    const float right = origin.x + (200.f + length) * scale;
    drawList->AddLine(ImVec2(left, y), ImVec2(right, y), IM_COL32(255, 160, 115, 255), 2.f);
    const std::string label = "length " + std::to_string(static_cast<int>(length));
    const ImVec2 labelSize = ImGui::CalcTextSize(label.c_str());
    drawList->AddText(ImVec2((left + right - labelSize.x) / 2.f, y - labelSize.y - 2.f), IM_COL32(255, 160, 115, 255), label.c_str());
    drawList->PopClipRect();

    ImGui::TextDisabled("Approximate battlefield: enemy base on the left, player base on the right.");
}
