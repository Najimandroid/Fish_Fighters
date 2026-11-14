#include "UIRoundButton.h"
#include "../UISkin.h"

namespace
{
    const float ICON_RATIO = 0.62f;      // Icon size compared to the button
    const float RING_THICKNESS = 7.f;
    const sf::Color RING_COLOR{ 72, 224, 255 };      // Cyan: "on", seen on blue water and on sand
    const unsigned int LABEL_SIZE = 30;
}

UIRoundButton::UIRoundButton(float diameter, sf::Vector2f position, const std::string& iconName) :
    UIButtonElement({ diameter, diameter }, position, ""),
    m_button(UISkin::texture("button_round")),
    m_icon(UISkin::texture(iconName)),
    m_label(UISkin::heavy_font())
{
    UISkin::style_text(m_label, LABEL_SIZE);
    m_ring.setRadius(diameter / 2.f + 2.f); // Just around the button
    m_ring.setOrigin({ m_ring.getRadius(), m_ring.getRadius() });
    m_ring.setFillColor(sf::Color::Transparent);
    m_ring.setOutlineThickness(RING_THICKNESS);
    m_ring.setOutlineColor(RING_COLOR);

    set_icon(iconName);
}

void UIRoundButton::set_icon(const std::string& iconName)
{
    const sf::Texture& texture = UISkin::texture(iconName);
    m_icon.setTexture(texture, true);
    m_icon.setOrigin(sf::Vector2f(texture.getSize()) / 2.f);
}

void UIRoundButton::set_icon_flipped(bool isFlipped)
{
    m_isIconFlipped = isFlipped;
}

void UIRoundButton::set_active_callback(std::function<bool()> isActive)
{
    m_isActive = isActive;
}

void UIRoundButton::set_state_label(std::function<std::string()> label)
{
    m_stateLabel = std::move(label);
}

void UIRoundButton::update(float deltaTime)
{
    m_isActiveNow = m_isActive && m_isActive();
}

void UIRoundButton::render(sf::RenderWindow& window)
{
    if (!m_isVisible) return;

    const sf::FloatRect rect = get_visual_rect();
    const sf::Vector2f center = rect.position + rect.size / 2.f;

    if (m_isActiveNow)
    {
        m_ring.setPosition(center);
        window.draw(m_ring);
    }

    // The round image fills its whole texture; a soft shadow under it first
    const sf::Vector2f buttonSize(m_button.getTexture().getSize());
    m_button.setScale({ rect.size.x / buttonSize.x, rect.size.y / buttonSize.y });
    m_button.setPosition(rect.position + sf::Vector2f{ 0.f, (m_isPressed ? 0.4f : 1.f) * UISkin::SHADOW_DROP * rect.size.x / 100.f });
    m_button.setColor(UISkin::SHADOW_TINT);
    window.draw(m_button);
    m_button.setPosition(rect.position);
    m_button.setColor(m_isPressed ? m_tint * sf::Color(215, 215, 215) : m_tint);
    window.draw(m_button);

    const float iconScale = rect.size.x * ICON_RATIO / static_cast<float>(m_icon.getTexture().getSize().x);
    m_icon.setScale({ m_isIconFlipped ? -iconScale : iconScale, iconScale });
    m_icon.setPosition(center);
    window.draw(m_icon);

    if (m_stateLabel)
    {
        m_label.setString(m_stateLabel());
        UISkin::set_text_color(m_label, m_isActiveNow ? RING_COLOR : UISkin::TEXT);
        UISkin::set_text_anchor(m_label, { 0.5f, 0.f });
        m_label.setPosition({ center.x, rect.position.y + rect.size.y + 8.f });
        window.draw(m_label);
    }
}
