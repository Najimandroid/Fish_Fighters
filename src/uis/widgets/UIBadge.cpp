#include "UIBadge.h"
#include "../UISkin.h"

#include <cmath>

namespace
{
    const float RADIUS = 24.f;
    const sf::Color BADGE_COLOR{ 255, 128, 104 };   // Coral: something waits here
    const unsigned int TEXT_SIZE = 30;
}

UIBadge::UIBadge(sf::Vector2f center, const std::string& text) :
    m_center(center),
    m_text(UISkin::heavy_font(), text)
{
    UISkin::style_text(m_text, TEXT_SIZE);
    UISkin::set_text_anchor(m_text, { 0.5f, 0.5f });
}

void UIBadge::update(float deltaTime)
{
    m_time += deltaTime;
}

void UIBadge::render(sf::RenderWindow& window)
{
    if (!m_isVisible) return;

    const float pulse = 1.f + 0.07f * std::sin(m_time * 2.f * 3.14159265f / 1.2f);
    const float radius = RADIUS * pulse;
    sf::CircleShape circle(radius);
    circle.setOrigin({ radius, radius });
    circle.setPosition(m_center);
    circle.setFillColor(BADGE_COLOR);
    circle.setOutlineThickness(4.f);
    circle.setOutlineColor(UISkin::OUTLINE);
    window.draw(circle);

    m_text.setScale({ pulse, pulse });
    m_text.setPosition(m_center);
    window.draw(m_text);
}
