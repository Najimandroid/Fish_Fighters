#include "UIPlayerShellsInfo.h"
#include "../UISkin.h"
#include "../../core/DataLoader.h"

namespace
{
    const unsigned int DIGITS_SIZE = 52;
    const float ICON_SIZE = 62.f;
    const float PADDING = 18.f;
    const float SHINY_OFFSET_X = 364.f;   // The Shiny Scales pill: left of the shells pill
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
        m_text.setString(std::to_string(m_currency == Currency::SHELLS ? player->shells : player->shinyScales));

    const float textWidth = m_text.getLocalBounds().size.x;
    m_size = { std::max(MIN_PILL_WIDTH, textWidth + ICON_SIZE + 3.f * PADDING), PILL_HEIGHT };
    m_position = { 1920.f - (m_currency == Currency::SHELLS ? OFFSET_X : SHINY_OFFSET_X) - m_size.x, OFFSET_Y };

    // Digits on the right of the pill, vertically centered
    UISkin::set_text_anchor(m_text, { 1.f, 0.5f });
    m_text.setPosition({ m_position.x + m_size.x - PADDING, m_position.y + m_size.y / 2.f });
}

void UIPlayerShellsInfo::render(sf::RenderWindow& window)
{
    if (!m_isVisible) return;

    UISkin::draw_nine_slice(window, UISkin::PANEL_DARK, get_bounds(), 1.f);
    UISkin::draw_fitted(window, UISkin::texture(m_currency == Currency::SHELLS ? "icon_shell" : "icon_shiny_scale"),
        { { m_position.x + PADDING, m_position.y + (m_size.y - ICON_SIZE) / 2.f }, { ICON_SIZE, ICON_SIZE } });
    window.draw(m_text);
}
