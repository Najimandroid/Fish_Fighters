#pragma once

#include "UIElement.h"

#include <memory>

class Stage;
class DataLoader;
class BattleEntity;

/*
 * UIStatusIcons
 * -------------
 * World-space widget (like UIBaseHealthInfo) drawing, at the feet of every unit and enemy,
 * a horizontal row with one icon per active status, as in The Battle Cats.
 * Icons come from the status icons sheet (STATUS_ICONS_TEXTURE), by each status' iconIndex.
 * An icon blinks during the last second of its status.
 *
 * The Stage draws each row right after the entity's sprite (Stage::set_entity_overlay, set by UIManager), so the
 * rows are layered like the entities: one standing in front covers the icons of one behind it.
 *
 * The row sits on the sprite's origin (bottom of the frame's center): the fish's bottom-left
 * corner, so it grows to the right over the fish, and the enemy's bottom-right corner,
 * so it grows to the left over the enemy.
 */
class UIStatusIcons : public UIElement
{
public:
    // Layout in world pixels (FishEditor's preview draws them the same way)
    static constexpr float ICON_SIZE = 36.f;      // Drawn size of one icon
    static constexpr float ICON_GAP = 2.f;        // Space between two icons

    UIStatusIcons(std::shared_ptr<Stage> stage, std::shared_ptr<DataLoader> dataLoader);

    void update(float deltaTime) override;
    void render(sf::RenderWindow& window) override {}   // The Stage draws the rows (draw_row)

    // The row of one entity. isGrowingRight: units (row starts at the origin and goes right), enemies go left
    void draw_row(sf::RenderWindow& window, const BattleEntity& entity, bool isGrowingRight);

private:

    std::weak_ptr<Stage> m_stage;
    std::shared_ptr<DataLoader> m_dataLoader;   // Icon index of each status

    sf::Texture m_iconsTexture;
    bool m_isTextureLoaded = false;
    float m_time = 0.f;                         // Drives the blinking
};
