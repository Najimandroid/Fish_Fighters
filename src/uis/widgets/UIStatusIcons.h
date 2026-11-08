#pragma once

#include "UIElement.h"

#include <memory>

class Stage;
class DataLoader;
class BattleEntity;

/*
 * UIStatusIcons
 * -------------
 * World-space widget (like UIBaseHealthInfo) drawing, under every unit and enemy,
 * a horizontal row with one icon per active status, as in The Battle Cats.
 * Icons come from the status icons sheet (STATUS_ICONS_TEXTURE), by each status' iconIndex.
 * An icon blinks during the last second of its status.
 */
class UIStatusIcons : public UIElement
{
public:
    // Layout in world pixels (FishEditor's preview draws them the same way)
    static constexpr float ICON_SIZE = 36.f;      // Drawn size of one icon
    static constexpr float ICON_GAP = 2.f;        // Space between two icons
    static constexpr float OFFSET_BELOW = 6.f;    // Space between the entity's feet and the row

    UIStatusIcons(std::shared_ptr<Stage> stage, std::shared_ptr<DataLoader> dataLoader);

    void update(float deltaTime) override;
    void render(sf::RenderWindow& window) override;

private:
    void draw_row(sf::RenderWindow& window, const BattleEntity& entity);

    std::weak_ptr<Stage> m_stage;
    std::shared_ptr<DataLoader> m_dataLoader;   // Icon index of each status

    sf::Texture m_iconsTexture;
    bool m_isTextureLoaded = false;
    float m_time = 0.f;                         // Drives the blinking
};
