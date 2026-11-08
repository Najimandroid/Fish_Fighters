#pragma once

#include "EditorPanel.h"
#include "../EditorContext.h"

#include <SFML/System/Clock.hpp>

#include <string>

/*
 * PreviewPanel
 * ------------
 * Shows the selected item as the game does:
 *  - Units and enemies: standing on a stage background (any of them, or none), with their
 *    animations played by BattleEntity's frame rules, their hitbox, attack range and damage
 *    zone drawn like the game computes them, and optionally an opponent in front of them
 *    (drag it to change the distance) to tune ranges against another entity
 *  - Stages: the battlefield (background, both bases, length)
 *  - Images: the image
 */
class PreviewPanel : public EditorPanel
{
public:
    const char* get_name() const override { return "Preview"; }
    void draw(EditorContext& ctx) override;

    // Entity standing in front of the edited one (FishEditor --opponent enemy:2)
    void set_opponent(const Selection& opponent) { m_opponent = opponent; }

private:
    enum class Animation { IDLE, WALK, ATTACK, KNOCKBACK };

    // A unit or enemy placed in the scene
    struct SceneEntity
    {
        const ordered_json* data = nullptr;
        const sf::Texture* sheet = nullptr;
        std::string name;
        Selection source;          // What it is (UNIT with its form, or ENEMY)
        bool isUnit = true;        // Units face left (toward the enemy base), enemies face right
        int frameCount = 1;
        int knockbackFrame = 1;
        int frame = 0;
        float center = 0.f;        // World x of the sprite's center
    };

    // Its combat zones in world x, as BattleUnit / BattleEnemy compute them
    struct Zones
    {
        float hitboxMin = 0.f, hitboxMax = 0.f;
        float rangeMin = 0.f, rangeMax = 0.f;
        float damageMin = 0.f, damageMax = 0.f;
        float range = 0.f;
    };

    void draw_entity(EditorContext& ctx, ordered_json& entity, DataFile file, const Selection& source);
    void draw_toolbar(EditorContext& ctx, bool isEditingUnit, float editedRange, float opponentRange, bool hasOpponent);
    bool opponent_picker(EditorContext& ctx, bool isEditingUnit);
    bool background_picker(EditorContext& ctx);
    void draw_scene(EditorContext& ctx, const SceneEntity& edited, const SceneEntity* opponent, float height);
    void draw_engagement(EditorContext& ctx, const SceneEntity& edited, const SceneEntity& opponent);
    void draw_frame_strip(EditorContext& ctx, ordered_json& entity, DataFile file, const sf::Texture& sheet);
    void draw_stage(EditorContext& ctx, const ordered_json& stage);

    bool load_entity(EditorContext& ctx, const Selection& source, SceneEntity& entity) const;
    std::string background_path(EditorContext& ctx) const;
    static Zones zones_of(const SceneEntity& entity);
    static void step_frame(int& frame, Animation animation, int frameCount, int knockbackFrame);

    static constexpr float FRAME_TIME = 0.1f;    // BattleEntity::timeUntilNextFrame
    static constexpr float GROUND_Y = 720.f;     // Entities stand at 1080 * 2/3 (layer 0)
    static constexpr const char* NO_BACKGROUND = "<none>";

    // ----- Animation -----
    Animation m_animation = Animation::WALK;
    bool m_isPlaying = true;
    float m_speed = 1.f;
    float m_frameTimer = 0.f;
    int m_frame = 0;
    int m_opponentFrame = 0;
    sf::Clock m_clock;
    Selection m_lastSelection;

    // ----- Scene -----
    std::string m_background;                    // "" = the first stage background, NO_BACKGROUND = plain dark
    bool m_isShowingHitboxes = true;
    bool m_isShowingRanges = true;
    bool m_isShowingDamageZones = true;
    bool m_isShowingStatusIcons = false;         // Every status icon under the entities, as in battle

    Selection m_opponent;                        // UNIT (with its form) or ENEMY in front, NONE = alone
    float m_distance = 120.f;                    // Between the two centers, in world pixels
    bool m_isDraggingOpponent = false;
    float m_viewLeft = 0.f, m_viewTop = 0.f, m_viewScale = 1.f;   // Kept while dragging, so the view doesn't move
};
