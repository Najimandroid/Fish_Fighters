#include "UIUnitCard.h"
#include "../UISkin.h"

#include <iostream>

namespace
{
    // Layout of a card 300 px wide; everything scales with the width
    const float BASE_WIDTH = 300.f;
    const float PADDING = 14.f;
    const float NAME_BAND = 62.f;            // Name above the icon
    const float ICON_RATIO = 0.75f;          // Icons are 4:3
    const float BORDER = 9.f;                // Yellow border of the selected card
    const float PANEL_CORNERS = 0.9f;

    // Texts are rendered at their biggest size and scaled down (sharper than scaling up)
    const float TEXT_SCALE_MAX = 1.4f;
    const float NAME_SIZE = 30.f;
    const float INFO_SIZE = 30.f;
    const float BADGE_SIZE = 22.f;

    const sf::Color BADGE_TINT{ 72, 217, 230 };    // Turns the pale blue slot image teal: "in the lineup", not an alert
    const sf::Color LOCKED_TINT{ 0, 0, 0 };        // Silhouette
    const sf::Color SHADE{ 0, 0, 0, 120 };          // Over the cards that aren't selected

    unsigned int text_size(float baseSize) { return static_cast<unsigned int>(baseSize * TEXT_SCALE_MAX); }
}

UIUnitCard::UIUnitCard(const std::string& iconTexturePath) :
    UITextureElement({ BASE_WIDTH, BASE_WIDTH * HEIGHT_RATIO }, { 0.f, 0.f }, "assets/images/textures/icons/placeholder.png"),
    m_name(UISkin::heavy_font()),
    m_level(UISkin::heavy_font()),
    m_value(UISkin::heavy_font()),
    m_badge(UISkin::heavy_font())
{
    if (!m_texture.loadFromFile(iconTexturePath))
    {
        std::cerr << "Failed to load icon: " << iconTexturePath << ", using placeholder.png\n";
        if (!m_texture.loadFromFile("assets/images/textures/icons/placeholder.png"))
            std::cerr << "Failed to load placeholder.png\n";
    }
    m_texture.setSmooth(true);

    UISkin::style_text(m_name, text_size(NAME_SIZE));
    UISkin::style_text(m_level, text_size(INFO_SIZE), UISkin::NUMBER);
    UISkin::style_text(m_value, text_size(INFO_SIZE), UISkin::NUMBER);
    UISkin::style_text(m_badge, text_size(BADGE_SIZE));
}

void UIUnitCard::select(bool isSelected)
{
    m_isSelected = isSelected;
}

bool UIUnitCard::is_selected() const
{
    return m_isSelected;
}

void UIUnitCard::set_unit_name(const std::string& name)
{
    m_name.setString(name);
}

void UIUnitCard::set_size(sf::Vector2f size)
{
    m_size = { size.x, size.x * HEIGHT_RATIO };
}

void UIUnitCard::set_level_line(const std::string& text, sf::Color color)
{
    m_level.setString(text);
    UISkin::set_text_color(m_level, color);
}

void UIUnitCard::set_value_line(const std::string& text, sf::Color color, bool showsShell)
{
    m_value.setString(text);
    UISkin::set_text_color(m_value, color);
    m_showsShell = showsShell;
}

void UIUnitCard::set_locked(bool isLocked)
{
    m_isLocked = isLocked;
}

void UIUnitCard::set_badge(const std::string& text)
{
    m_badge.setString(text);
}

void UIUnitCard::update(float deltaTime)
{
    if (tweenX.progress() < 1.0f)
        m_position.x = tweenX.step(1);

    if (tweenWidth.progress() < 1.0f || tweenHeight.progress() < 1.0f)
    {
        const float width = tweenWidth.step(1);
        tweenHeight.step(1);
        set_size({ width, width });
    }
}

sf::FloatRect UIUnitCard::get_bounds() const
{
    return sf::FloatRect{ { m_position.x, m_position.y - m_size.y }, m_size };
}

void UIUnitCard::render(sf::RenderWindow& window)
{
    if (!m_isVisible) return;

    const sf::FloatRect card = get_bounds();
    const float scale = card.size.x / BASE_WIDTH;
    const float textScale = scale / TEXT_SCALE_MAX;
    const float left = card.position.x;
    const float right = left + card.size.x;
    const float top = card.position.y;
    const float bottom = top + card.size.y;

    // ----- Card -----
    if (m_isSelected)
    {
        const float border = BORDER * scale;
        const sf::FloatRect glow{ card.position - sf::Vector2f{ border, border }, card.size + sf::Vector2f{ border, border } * 2.f };
        UISkin::draw_nine_slice(window, UISkin::BUTTON_YELLOW, glow, 0.7f * scale);
    }
    UISkin::draw_nine_slice(window, UISkin::PANEL_DARK, card, PANEL_CORNERS * scale);

    // ----- Name -----
    m_name.setScale({ textScale, textScale });
    UISkin::set_text_anchor(m_name, { 0.5f, 0.5f });
    m_name.setPosition({ left + card.size.x / 2.f, top + NAME_BAND * scale / 2.f + 4.f * scale });
    window.draw(m_name);

    // ----- Icon in a slot -----
    const float padding = PADDING * scale;
    const float iconWidth = card.size.x - 2.f * padding;
    const sf::FloatRect slot{ { left + padding, top + NAME_BAND * scale }, { iconWidth, iconWidth * ICON_RATIO } };
    UISkin::draw_nine_slice(window, UISkin::SLOT, slot, PANEL_CORNERS * scale);
    UISkin::draw_cropped(window, m_texture, UISkin::slot_inner(slot, PANEL_CORNERS * scale), m_isLocked ? LOCKED_TINT : sf::Color::White);
    UISkin::draw_nine_slice(window, UISkin::SLOT_FRAME, slot, PANEL_CORNERS * scale);

    // Badge in the top right corner of the icon
    if (!m_badge.getString().isEmpty())
    {
        m_badge.setScale({ textScale, textScale });
        const sf::FloatRect textBounds = m_badge.getGlobalBounds();
        const sf::Vector2f badgeSize{ textBounds.size.x + 24.f * scale, 38.f * scale };
        const sf::FloatRect badge{ { slot.position.x + slot.size.x - badgeSize.x - 10.f * scale, slot.position.y + 10.f * scale }, badgeSize };
        UISkin::draw_nine_slice(window, UISkin::SLOT, badge, 0.35f * scale, BADGE_TINT);
        UISkin::set_text_anchor(m_badge, { 0.5f, 0.5f });
        m_badge.setPosition(badge.position + badge.size / 2.f);
        window.draw(m_badge);
    }

    // ----- Info lines: level on the left, value on the right -----
    const float lineY = (slot.position.y + slot.size.y + bottom) / 2.f;

    // Both on one row: shrunk together when they would touch ("LEVEL 12" and a 4-digit price)
    m_level.setScale({ textScale, textScale });
    m_value.setScale({ textScale, textScale });
    const float iconRoom = m_showsShell ? 48.f * scale : 0.f;
    const float room = (right - left) - 52.f * scale;
    const float needed = m_level.getGlobalBounds().size.x + m_value.getGlobalBounds().size.x + iconRoom;
    const float fit = needed > room ? room / needed : 1.f;

    m_level.setScale({ textScale * fit, textScale * fit });
    UISkin::set_text_anchor(m_level, { 0.f, 0.5f });
    m_level.setPosition({ left + 18.f * scale, lineY });
    window.draw(m_level);

    m_value.setScale({ textScale * fit, textScale * fit });
    UISkin::set_text_anchor(m_value, { 1.f, 0.5f });
    m_value.setPosition({ right - 18.f * scale, lineY });
    window.draw(m_value);

    if (m_showsShell)
    {
        const float iconSize = 36.f * scale * fit;
        const float valueLeft = m_value.getGlobalBounds().position.x;
        UISkin::draw_fitted(window, UISkin::texture("icon_shell"),
            { { valueLeft - iconSize - 6.f * scale, lineY - iconSize / 2.f }, { iconSize, iconSize } });
    }

    // ----- Shade over the cards that aren't selected -----
    if (!m_isSelected)
        UISkin::draw_nine_slice(window, UISkin::PANEL_DARK, card, PANEL_CORNERS * scale, SHADE);
}
