#include "UIPlayerShellsInfo.h"
#include "../UISkin.h"
#include "../../core/DataLoader.h"

#include <algorithm>
#include <cmath>

namespace
{
    const float ROLL_RATE = 7.f;          // How fast the digits roll to a new amount
    const unsigned int DIGITS_SIZE = 52;
    const float ICON_SIZE = 62.f;
    const float PADDING = 18.f;
    const float SHINY_OFFSET_X = 364.f;   // The Shiny Scales pill: left of the shells pill
    const float FISH_OFFSET_X = 704.f;    // The Fish Scales pill: left of it (40 px apart, like the others)
}

/*
 * Initializes the digits: yellow, outlined
 */
UIPlayerShellsInfo::UIPlayerShellsInfo(std::shared_ptr<DataLoader> dataLoader, Currency currency) :
    UITextElement({ 1.f, 1.f }, { 1.f, 1.f }),
    m_dataLoader(dataLoader),
    m_currency(currency)
{
    UISkin::style_text(m_text, DIGITS_SIZE, UISkin::NUMBER);
}

/*
 * Updates the shell count, and the pill around it (its size and position are the element's)
 */
void UIPlayerShellsInfo::update(float deltaTime)
{
    if (auto player = m_dataLoader->get_player_data().lock())
    {
        const int amount = m_currency == Currency::SHELLS ? player->shells
            : m_currency == Currency::SHINY_SCALES ? player->shinyScales : player->fishScales;
        const float target = static_cast<float>(amount - m_heldBack);

        // The digits roll to a new amount instead of jumping (spent: they flash red on the way down)
        if (m_shown < 0.f) m_shown = target;
        else if (m_shown != target)
        {
            if (target < m_shown && m_spent <= 0.f) m_spent = 1.f;
            m_shown += (target - m_shown) * std::min(1.f, deltaTime * ROLL_RATE);
            if (std::abs(target - m_shown) < 0.5f) m_shown = target;
        }
        m_text.setString(std::to_string(static_cast<int>(std::lround(m_shown))));
    }
    m_pop = std::max(0.f, m_pop - deltaTime * 5.f);
    m_spent = std::max(0.f, m_spent - deltaTime * 2.5f);
    const sf::Color red{ 255, 120, 110 };
    const auto mix = [this](std::uint8_t a, std::uint8_t b) { return static_cast<std::uint8_t>(a + (b - a) * m_spent); };
    UISkin::set_text_color(m_text, { mix(UISkin::NUMBER.r, red.r), mix(UISkin::NUMBER.g, red.g), mix(UISkin::NUMBER.b, red.b) });

    const float textWidth = m_text.getLocalBounds().size.x;
    m_size = { std::max(MIN_PILL_WIDTH, textWidth + ICON_SIZE + 3.f * PADDING + m_trailingRoom), PILL_HEIGHT };
    const float offset = m_currency == Currency::SHELLS ? OFFSET_X : m_currency == Currency::SHINY_SCALES ? SHINY_OFFSET_X : FISH_OFFSET_X;
    m_position = { 1920.f - offset - m_size.x, OFFSET_Y };

    // Digits on the right of the pill (left of a button there), vertically centered
    UISkin::set_text_anchor(m_text, { 1.f, 0.5f });
    m_text.setPosition({ m_position.x + m_size.x - PADDING - m_trailingRoom, m_position.y + m_size.y / 2.f });
}

void UIPlayerShellsInfo::set_tint(sf::Color tint)
{
    m_tint = tint;
}

void UIPlayerShellsInfo::set_trailing_room(float room)
{
    m_trailingRoom = std::max(0.f, room);
}

void UIPlayerShellsInfo::render(sf::RenderWindow& window)
{
    if (!m_isVisible) return;

    UISkin::draw_nine_slice(window, UISkin::PANEL_DARK, get_bounds(), 1.f, m_tint);
    const float iconSize = ICON_SIZE * (1.f + 0.3f * m_pop);   // Pops when something lands in it
    const char* icon = m_currency == Currency::SHELLS ? "icon_shell" : m_currency == Currency::SHINY_SCALES ? "icon_shiny_scale" : "icon_fish_scale";
    UISkin::draw_fitted(window, UISkin::texture(icon),
        { get_icon_center() - sf::Vector2f{ iconSize, iconSize } / 2.f, { iconSize, iconSize } });
    window.draw(m_text);
}

void UIPlayerShellsInfo::hold_back(int amount)
{
    m_heldBack += std::max(0, amount);
}

void UIPlayerShellsInfo::receive(int amount)
{
    m_heldBack = std::max(0, m_heldBack - std::max(0, amount));
    m_pop = 1.f;
}

sf::Vector2f UIPlayerShellsInfo::get_icon_center() const
{
    return { m_position.x + PADDING + ICON_SIZE / 2.f, m_position.y + m_size.y / 2.f };
}
