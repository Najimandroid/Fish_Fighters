#include "UIBattleIcon.h"
#include <iostream>

/*
 * Initializes the button, loads the icon texture, sets up cooldown bar and price text.
 */
UIBattleIcon::UIBattleIcon(sf::Vector2f position, const std::string& iconTexturePath) :
    UIButtonElement({ 144.f * 1.15f, 108.f * 1.15f }, position, "0$"),
    m_sprite(m_texture)
{
    // Load icon texture
    set_texture(iconTexturePath);

    // Configure price text
    m_text.setCharacterSize(30);
    m_text.setFillColor(sf::Color::Yellow);
    m_text.setOutlineColor(sf::Color::Black);
    m_text.setOutlineThickness(1.5f);

    set_cost(m_cost);

    // Setup cooldown bar visuals
    float barWidth = m_shape.getSize().x - 15.f;
    float barHeight = m_shape.getSize().y * 0.1f;

    m_barBackground.setSize({ barWidth, barHeight });
    m_barBackground.setFillColor(sf::Color(0, 0, 0, 0));
    m_barBackground.setPosition({
        position.x + (m_shape.getSize().x - barWidth) / 2.f,
        position.y + m_shape.getSize().y - barHeight - 7.5f
        });

    m_barFill.setSize({ 0.f, barHeight });
    m_barFill.setFillColor(sf::Color(0, 255, 255, 0));
    m_barFill.setPosition(m_barBackground.getPosition());

    // Shape outline
    m_shape.setOutlineColor(sf::Color::Black);
    m_shape.setOutlineThickness(4.5f);

    // Setup black overlay filter
    m_blackFilter.setSize(m_shape.getSize());
    m_blackFilter.setPosition(m_shape.getPosition());
}

/*
 * Updates cooldown progress, updates overlay and price text
 */
void UIBattleIcon::update(float deltaTime)
{
    if (m_isOnCooldown)
    {
        m_currentCooldown += deltaTime;

        if (m_currentCooldown >= m_maxCooldown)
        {
            // Cooldown finished
            m_isOnCooldown = false;
            m_currentCooldown = 0.0f;

            // Hide overlays
            sf::Color filterColor = m_blackFilter.getFillColor(); filterColor.a = 0; m_blackFilter.setFillColor(filterColor);
            sf::Color bgColor = m_barBackground.getFillColor(); bgColor.a = 0; m_barBackground.setFillColor(bgColor);
            sf::Color fillColor = m_barFill.getFillColor(); fillColor.a = 0; m_barFill.setFillColor(fillColor);
        }
    }

    // Update price text
    std::string priceTagText = (m_isOnCooldown || m_uid < 0) ? "" : std::to_string(m_cost) + "$";
    m_text.setString(priceTagText);

    // Update cooldown bar
    float progress = m_isOnCooldown ? (m_currentCooldown / m_maxCooldown) : 0.f;
    progress = std::clamp(progress, 0.f, 1.f);

    float fullWidth = m_barBackground.getSize().x;
    sf::Vector2f size = m_barFill.getSize();
    size.x = fullWidth * progress;
    m_barFill.setSize(size);
}

/*
 * Draws the icon sprite, button, cooldown overlay, bar, and price text
 */
void UIBattleIcon::render(sf::RenderWindow& window)
{
    if (m_isVisible)
    {
        window.draw(m_shape);
        window.draw(m_sprite);
        window.draw(m_blackFilter);
        window.draw(m_barBackground);
        window.draw(m_barFill);
        window.draw(m_text);
    }
}

// Getters
int UIBattleIcon::get_uid() const { return m_uid; }
bool UIBattleIcon::get_if_on_cooldown() const { return m_isOnCooldown; }
int UIBattleIcon::get_cost() const { return m_cost; }

// Setters
void UIBattleIcon::set_uid(int uid) { m_uid = uid; }
void UIBattleIcon::set_max_cooldown(int cooldown) { m_maxCooldown = cooldown; }

/*
 * Sets the unit deployment cost and updates price text position
 */
void UIBattleIcon::set_cost(int cost)
{
    m_cost = cost;

    if (m_cost < 0)
    {
        m_text.setString("");
        m_text.setFillColor(sf::Color::Transparent);
    }
    else
    {
        m_text.setString(std::to_string(m_cost) + "$");
        m_text.setFillColor(sf::Color::Yellow);
    }

    // Adjust text position to bottom-right of the icon
    auto bounds = m_shape.getGlobalBounds();
    auto textBounds = m_text.getLocalBounds();
    m_text.setOrigin({
        textBounds.position.x + textBounds.size.x,
        textBounds.position.y + textBounds.size.y
        });

    float paddingX = 2.f;
    float paddingY = 2.f;

    m_text.setPosition({
        bounds.position.x + bounds.size.x - paddingX,
        bounds.position.y + bounds.size.y - paddingY
        });
}

/*
 * Begins cooldown and sets overlay and cooldown bar to visible
 */
void UIBattleIcon::start_cooldown()
{
    m_isOnCooldown = true;
    m_currentCooldown = 0.0f;

    sf::Color filterColor = m_blackFilter.getFillColor(); filterColor.a = 128; m_blackFilter.setFillColor(filterColor);
    sf::Color bgColor = m_barBackground.getFillColor(); bgColor.a = 255; m_barBackground.setFillColor(bgColor);
    sf::Color fillColor = m_barFill.getFillColor(); fillColor.a = 255; m_barFill.setFillColor(fillColor);
}

/*
 * Loads a texture from file and scales/centers it inside the icon
 */
void UIBattleIcon::set_texture(const std::string& texturePath)
{
    if (!m_texture.loadFromFile(texturePath))
        m_texture.loadFromFile("assets/images/textures/icons/placeholder.png");

    m_sprite.setTextureRect(sf::IntRect({ 0, 0 }, { static_cast<int>(m_texture.getSize().x), static_cast<int>(m_texture.getSize().y) }));

    // Scale sprite to fit button
    sf::Vector2f targetSize = m_shape.getSize();
    sf::Vector2u texSize = m_texture.getSize();
    float scaleX = targetSize.x / texSize.x;
    float scaleY = targetSize.y / texSize.y;
    float uniformScale = std::min(scaleX, scaleY);

    m_sprite.setScale({ uniformScale, uniformScale });

    // Center sprite inside the button
    m_sprite.setPosition(m_shape.getPosition());
}

/*
 * Darkens the icon when unavailable, e.g., insufficient cash
 */
void UIBattleIcon::set_darkened(bool isDarkened)
{
    sf::Color bgColor = m_barBackground.getFillColor();

    if (isDarkened)
    {
        bgColor.a = 128;
        m_text.setFillColor(sf::Color(128, 128, 0));
    }
    else
    {
        bgColor.a = 0;
        m_text.setFillColor(sf::Color::Yellow);
    }

    m_blackFilter.setFillColor(bgColor);
}
