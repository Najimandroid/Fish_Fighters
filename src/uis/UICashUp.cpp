#include "UICashUp.h"
#include "../core/stages/Stage.h"

/*
 * Sets up the button size, position, icon, text, and callback for handling upgrades.
 */
UICashUp::UICashUp(std::shared_ptr<Stage> stage) :
    UIButtonElement({ 300.f, 300.f }, { 0.f, 780.f }), // Initialize base UIButtonElement
    m_stage(stage),
    m_icon(m_texture) // Initialize sprite with texture
{
    m_position = { 0.f, 780.f };
    m_shape.setFillColor(sf::Color(0, 0, 0, 0)); // Transparent button shape

    // Load icon texture
    auto success = m_texture.loadFromFile("assets/images/textures/icons/uis/cash_up.png");
    m_icon.setTextureRect(sf::IntRect(
        { 0, 0 },
        { static_cast<int>(m_texture.getSize().x), static_cast<int>(m_texture.getSize().y) }
    ));
    // Scale sprite to match button size
    m_icon.setScale({
        m_shape.getSize().x / static_cast<float>(m_texture.getSize().x),
        m_shape.getSize().y / static_cast<float>(m_texture.getSize().y)
        });
    m_icon.setPosition(m_position);

    // Configure text
    m_text.setCharacterSize(30);
    m_text.setFillColor(sf::Color::Green);
    m_text.setOutlineColor(sf::Color::Black);
    m_text.setOutlineThickness(3.f);

    // Callback executed when button is clicked
    set_callback(
        [this]()
        {
            // Prevent upgrading beyond max level
            if (m_level >= m_maxLevel) return;

            // Lock stage reference
            if (auto stage = m_stage.lock())
            {
                // Attempt upgrade; if successful, increment level and increase cost
                if (stage->upgrade_cash(m_level, m_cost))
                {
                    m_level++;
                    m_cost *= 1.5f;
                }
            }
        }
    );
}

/*
 * Updates the button state and text each frame:
 * - Changes icon and text color based on affordability
 * - Updates the displayed text for level and cost
 */
void UICashUp::update(float deltaTime)
{
    // Check if upgrade is affordable or maxed
    if (m_stage.lock()->get_cash() >= m_cost || m_level == m_maxLevel)
    {
        // Active icon and green text
        m_texture.loadFromFile("assets/images/textures/icons/uis/cash_up.png");
        m_text.setFillColor(sf::Color::Green);
    }
    else
    {
        // Disabled icon and greyed-out text
        m_texture.loadFromFile("assets/images/textures/icons/uis/cash_up_disabled.png");
        m_text.setFillColor(sf::Color(135, 135, 135));
    }

    // Update text based on current level
    if (m_level >= m_maxLevel)
    {
        m_text.setString("\t-Cash Up-\nMAX LEVEL: " + std::to_string(m_level));
    }
    else
    {
        m_text.setString("\t-Cash Up-\nLevel: " + std::to_string(m_level) +
            " : " + std::to_string(m_cost) + "$");
    }

    // Center text horizontally above the button icon
    auto bounds = m_text.getLocalBounds();
    m_text.setOrigin({
        bounds.position.x + bounds.size.x / 2.f,
        bounds.position.y
        });
    m_text.setPosition({
        m_position.x + m_shape.getSize().x / 2.f - 33.f, // slightly adjusted to look centered
        m_position.y - 30.f // offset above the icon
        });
}

/*
 * Draws the button elements to the screen.
 * - Only renders if the button is visible
 */
void UICashUp::render(sf::RenderWindow& window)
{
    if (m_isVisible)
    {
        window.draw(m_shape); // button background (transparent)
        window.draw(m_icon);  // upgrade icon
        window.draw(m_text);  // level/cost text
    }
}
