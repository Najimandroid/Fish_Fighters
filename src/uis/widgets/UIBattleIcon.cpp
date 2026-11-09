#include "UIBattleIcon.h"
#include "../UISkin.h"

#include <algorithm>
#include <cstdint>

namespace
{
    const float FRAME_SCALE = 1.f;                       // Corners of the slot images on screen
    const unsigned int COST_SIZE = 32;
    const sf::Vector2f COST_PADDING{ 12.f, 10.f };       // From the bottom right corner
    const float BAR_HEIGHT = 14.f;
    const float BAR_MARGIN = 14.f;                       // From the sides and the bottom
    const sf::Color BAR_BACKGROUND{ 20, 30, 40, 220 };
    const std::uint8_t SHADE_ALPHA_COOLDOWN = 150;
    const std::uint8_t SHADE_ALPHA_DARKENED = 110;
}

/*
 * Initializes the button and loads the icon texture.
 */
UIBattleIcon::UIBattleIcon(sf::Vector2f position, const std::string& iconTexturePath) :
    UIButtonElement({ WIDTH, HEIGHT }, position, "")
{
    // Load icon texture
    set_texture(iconTexturePath);

    // Price text: yellow outlined digits
    UISkin::style_text(m_text, COST_SIZE, UISkin::NUMBER);
    set_cost(m_cost);
}

/*
 * Updates cooldown progress and price text
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
        }
    }

    // Update price text (hidden while the cooldown bar is shown)
    std::string priceTagText = (m_isOnCooldown || m_uid < 0) ? "" : std::to_string(m_cost) + "$";
    m_text.setString(priceTagText);
    UISkin::set_text_color(m_text, m_isDarkened ? UISkin::NUMBER_DISABLED : UISkin::NUMBER);
}

/*
 * Draws the slot, the unit icon inside it, the shade and cooldown bar, the frame over the icon edges, and the price
 */
void UIBattleIcon::render(sf::RenderWindow& window)
{
    if (!m_isVisible) return;

    const sf::FloatRect rect = get_visual_rect();
    const float scale = rect.size.x / WIDTH;
    const sf::FloatRect inner = UISkin::slot_inner(rect, FRAME_SCALE * scale);

    const bool isEmpty = m_uid < 0;
    UISkin::draw_nine_slice(window, UISkin::SLOT, rect, FRAME_SCALE * scale, isEmpty ? UISkin::EMPTY_SLOT_TINT : sf::Color::White);

    if (!isEmpty)
    {
        // Unit icon cropped to the shape of the slot, filling it
        UISkin::draw_cropped(window, m_texture, inner);

        // Shade: can't be deployed (cooldown or not enough cash)
        if (m_isOnCooldown || m_isDarkened)
        {
            sf::RectangleShape shade(inner.size);
            shade.setPosition(inner.position);
            sf::Color shadeColor = UISkin::SHADE;
            shadeColor.a = m_isOnCooldown ? SHADE_ALPHA_COOLDOWN : SHADE_ALPHA_DARKENED;
            shade.setFillColor(shadeColor);
            window.draw(shade);
        }

        // Cooldown bar at the bottom of the slot
        if (m_isOnCooldown)
        {
            const float progress = std::clamp(m_currentCooldown / m_maxCooldown, 0.f, 1.f);
            const sf::Vector2f barSize{ rect.size.x - 2.f * BAR_MARGIN * scale, BAR_HEIGHT * scale };
            const sf::Vector2f barPosition{ rect.position.x + BAR_MARGIN * scale, rect.position.y + rect.size.y - (BAR_MARGIN * scale) - barSize.y };

            sf::RectangleShape barBackground(barSize);
            barBackground.setPosition(barPosition);
            barBackground.setFillColor(BAR_BACKGROUND);
            barBackground.setOutlineThickness(2.f * scale);
            barBackground.setOutlineColor(UISkin::OUTLINE);
            window.draw(barBackground);

            sf::RectangleShape barFill({ barSize.x * progress, barSize.y });
            barFill.setPosition(barPosition);
            barFill.setFillColor(UISkin::COOLDOWN);
            window.draw(barFill);
        }
    }

    // Frame over the icon edges (hides its corners under the rounded border)
    UISkin::draw_nine_slice(window, UISkin::SLOT_FRAME, rect, FRAME_SCALE * scale);

    // Price at the bottom right
    UISkin::set_text_anchor(m_text, { 1.f, 1.f });
    m_text.setScale({ scale, scale });
    m_text.setPosition(rect.position + rect.size - COST_PADDING * scale);
    window.draw(m_text);
}

// Getters
int UIBattleIcon::get_uid() const { return m_uid; }
bool UIBattleIcon::get_if_on_cooldown() const { return m_isOnCooldown; }
int UIBattleIcon::get_cost() const { return m_cost; }

// Setters
void UIBattleIcon::set_uid(int uid) { m_uid = uid; }
void UIBattleIcon::set_max_cooldown(float cooldown) { m_maxCooldown = cooldown; }

/*
 * Sets the unit deployment cost (a negative cost hides the price)
 */
void UIBattleIcon::set_cost(int cost)
{
    m_cost = cost;
    m_text.setString(m_cost < 0 ? "" : std::to_string(m_cost) + "$");
}

/*
 * Begins the cooldown: the slot is shaded and the bar fills up
 */
void UIBattleIcon::start_cooldown()
{
    m_isOnCooldown = true;
    m_currentCooldown = 0.0f;
}

/*
 * Loads a texture from file (placeholder if missing). render() crops and scales it into the slot
 */
void UIBattleIcon::set_texture(const std::string& texturePath)
{
    if (!m_texture.loadFromFile(texturePath))
        m_texture.loadFromFile("assets/images/textures/icons/placeholder.png");
    m_texture.setSmooth(true);
}

/*
 * Darkens the icon when unavailable, e.g., insufficient cash
 */
void UIBattleIcon::set_darkened(bool isDarkened)
{
    m_isDarkened = isDarkened;
}
