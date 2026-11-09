#include "UIBaseHealthInfo.h"
#include "../UISkin.h"
#include "../../core/stages/Stage.h"

#include <algorithm>

namespace
{
    const unsigned int TEXT_SIZE = 34;
    const float GAP_ABOVE_BASE = 12.f; // Between the top of the base image and the text

    // "current / max", centered just above the base image
    void place_health_text(sf::Text& text, const BattleBase& base)
    {
        text.setString(std::to_string(std::max(0, static_cast<int>(base.currentHealth)))
            + " / " + std::to_string(static_cast<int>(base.maxHealth)));

        const sf::FloatRect baseBounds = base.sprite.getGlobalBounds();
        UISkin::set_text_anchor(text, { 0.5f, 1.f });
        text.setPosition({ baseBounds.position.x + baseBounds.size.x / 2.f, baseBounds.position.y - GAP_ABOVE_BASE });
    }
}

/*
 * Initializes the texts with the outlined heavy font of the skin.
 */
UIBaseHealthInfo::UIBaseHealthInfo(std::shared_ptr<Stage> stage) :
    m_stage(stage),
    m_textEnemyBase(UISkin::heavy_font()),
    m_textUnitBase(UISkin::heavy_font())
{
    isWorldSpaceUi = true; // Render positions relative to world coordinates

    UISkin::style_text(m_textEnemyBase, TEXT_SIZE);
    UISkin::style_text(m_textUnitBase, TEXT_SIZE);
}

/*
 * Updates the displayed health values for both the player's base and the enemy's base.
 * Positions the text above each base in the game world.
 */
void UIBaseHealthInfo::update(float deltaTime)
{
    if (auto stage = m_stage.lock())
    {
        if (auto enemyBase = stage->get_enemy_base().lock())
            place_health_text(m_textEnemyBase, *enemyBase);

        if (auto unitBase = stage->get_unit_base().lock())
            place_health_text(m_textUnitBase, *unitBase);
    }
}

/*
 * Draws the health texts to the window if the UI element is visible.
 */
void UIBaseHealthInfo::render(sf::RenderWindow& window)
{
    if (!m_isVisible) return;

    window.draw(m_textEnemyBase); // Draw enemy base health
    window.draw(m_textUnitBase);  // Draw player base health
}
