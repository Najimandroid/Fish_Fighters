#pragma once

#include "EditorPanel.h"
#include "../EditorContext.h"

#include <SFML/System/Clock.hpp>

/*
 * PreviewPanel
 * ------------
 * Shows the selected item as the game does:
 *  - Units and enemies: their animations, played with the same frame rules as BattleEntity
 *    (walk frames, knockback frame, attack frames), and the frame strip of the sheet
 *  - Stages: the battlefield (background, both bases, length)
 *  - Images: the image
 */
class PreviewPanel : public EditorPanel
{
public:
    const char* get_name() const override { return "Preview"; }
    void draw(EditorContext& ctx) override;

private:
    enum class Animation { IDLE, WALK, ATTACK, KNOCKBACK };

    void draw_animation(EditorContext& ctx, ordered_json& entity, DataFile file);
    void step_animation(int frameCount, int knockbackFrame);
    void draw_frame_strip(EditorContext& ctx, ordered_json& entity, DataFile file, const sf::Texture& sheet);
    void draw_stage(EditorContext& ctx, const ordered_json& stage);

    static constexpr float FRAME_TIME = 0.1f;   // BattleEntity::timeUntilNextFrame

    Animation m_animation = Animation::WALK;
    bool m_isPlaying = true;
    float m_speed = 1.f;
    float m_frameTimer = 0.f;
    int m_frame = 0;
    sf::Clock m_clock;
    Selection m_lastSelection;
};
