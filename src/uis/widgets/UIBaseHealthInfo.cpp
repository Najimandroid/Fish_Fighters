#include "UIBaseHealthInfo.h"
#include "../../core/stages/Stage.h"

/*
 * Initializes the UI element with font and text styling.
 */
UIBaseHealthInfo::UIBaseHealthInfo(std::shared_ptr<Stage> stage) :
    m_stage(stage),
    m_textEnemyBase(m_font),
    m_textUnitBase(m_font)
{
    isWorldSpaceUi = true; // Render positions relative to world coordinates

    // Load font
    auto success = m_font.openFromFile("assets/fonts/MPLUSRounded1c-Medium.ttf");

    // Configure enemy base text appearance
    m_textEnemyBase.setCharacterSize(20);
    m_textEnemyBase.setFillColor(sf::Color::White);
    m_textEnemyBase.setOutlineColor(sf::Color::Black);
    m_textEnemyBase.setOutlineThickness(1.f);

    // Configure player base text appearance
    m_textUnitBase.setCharacterSize(20);
    m_textUnitBase.setFillColor(sf::Color::White);
    m_textUnitBase.setOutlineColor(sf::Color::Black);
    m_textUnitBase.setOutlineThickness(1.f);
}

/*
 * Updates the displayed health values for both the player's base and the enemy's base.
 * Positions the text above each base in the game world.
 */
void UIBaseHealthInfo::update(float deltaTime)
{
    if (auto stage = m_stage.lock())
    {
        // Enemy base health display
        m_textEnemyBase.setString(
            "Health: " + std::to_string(std::max(0, (int)stage->get_enemy_base().lock()->currentHealth))
            + " / " + std::to_string((int)stage->get_enemy_base().lock()->maxHealth)
        );

        auto boundsEnemy = m_textEnemyBase.getLocalBounds();
        m_textEnemyBase.setOrigin({
            boundsEnemy.position.x + boundsEnemy.size.x / 2.f, // center horizontally
            boundsEnemy.position.y
            });
        m_textEnemyBase.setPosition(
            stage->get_enemy_base().lock()->position + sf::Vector2f{ 100.f, -80.f } // offset above base
        );

        // Player unit base health display
        m_textUnitBase.setString(
            "Health: " + std::to_string(std::max(0, (int)stage->get_unit_base().lock()->currentHealth))
            + " / " + std::to_string((int)stage->get_unit_base().lock()->maxHealth)
        );

        auto boundsUnit = m_textUnitBase.getLocalBounds();
        m_textUnitBase.setOrigin({
            boundsUnit.position.x + boundsUnit.size.x / 2.f, // center horizontally
            boundsUnit.position.y
            });
        m_textUnitBase.setPosition(
            stage->get_unit_base().lock()->position + sf::Vector2f{ 100.f, -80.f } // offset above base
        );
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
