#include "UICallout.h"
#include "../UISkin.h"

#include <algorithm>
#include <cmath>

namespace
{
    const unsigned int TEXT_SIZE = 26;
    const sf::Vector2f PADDING{ 20.f, 10.f };
    const float TAIL = 16.f;            // How far the tail sticks out
    const float BOB = 4.f;              // Up and down (DOWN) or sideways (LEFT), in pixels
    const float POP_SECONDS = 0.3f, POP_DELAY = 0.45f;   // It pops in once its screen is in
    const sf::Color FILL{ 255, 214, 64 };
}

UICallout::UICallout(const std::string& text, sf::Vector2f tip, Tail tail) :
    m_text(UISkin::heavy_font(), text), m_tip(tip), m_tail(tail)
{
    UISkin::style_text(m_text, TEXT_SIZE, UISkin::OUTLINE);
    m_text.setOutlineThickness(0.f);
}

void UICallout::update(float deltaTime)
{
    m_time += deltaTime;
}

void UICallout::render(sf::RenderWindow& window)
{
    if (!m_isVisible) return;
    const float popT = std::clamp((m_time - POP_DELAY) / POP_SECONDS, 0.f, 1.f);
    if (popT <= 0.f) return;
    const float pop = 1.f + 0.15f * std::sin(popT * 3.14159265f) * (1.f - popT) + (popT < 1.f ? (popT - 1.f) * 0.3f : 0.f);
    const float bob = std::sin(m_time * 3.f) * BOB;

    // The pill beside or above the tip, its tail on the tip
    const sf::FloatRect textBounds = m_text.getLocalBounds();
    const sf::Vector2f size = (textBounds.size + PADDING * 2.f) * pop;
    sf::Vector2f tip = m_tip;
    sf::Vector2f pillPosition;
    if (m_tail == Tail::LEFT)
    {
        tip.x += bob;
        pillPosition = { tip.x + TAIL, tip.y - size.y / 2.f };
    }
    else
    {
        tip.y += bob;
        pillPosition = { tip.x - size.x / 2.f, tip.y - TAIL - size.y };
    }
    const sf::FloatRect pill{ pillPosition, size };

    // Its shadow, then the tail and the pill (one dark outline round both: the tail drawn over the pill's edge)
    sf::ConvexShape shadow = UISkin::rounded_rect({ pill.position + sf::Vector2f{ UISkin::SHADOW_SHIFT, UISkin::SHADOW_DROP } * 0.6f, pill.size }, size.y / 2.f);
    shadow.setFillColor(UISkin::SHADOW_TINT);
    window.draw(shadow);

    sf::ConvexShape tail(3);
    if (m_tail == Tail::LEFT)
    {
        tail.setPoint(0, tip);
        tail.setPoint(1, { pill.position.x + 6.f, pill.position.y + size.y / 2.f - 11.f });
        tail.setPoint(2, { pill.position.x + 6.f, pill.position.y + size.y / 2.f + 11.f });
    }
    else
    {
        tail.setPoint(0, tip);
        tail.setPoint(1, { tip.x - 12.f, pill.position.y + size.y - 6.f });
        tail.setPoint(2, { tip.x + 12.f, pill.position.y + size.y - 6.f });
    }
    tail.setFillColor(FILL);
    tail.setOutlineThickness(4.f);
    tail.setOutlineColor(UISkin::OUTLINE);
    window.draw(tail);

    sf::ConvexShape shape = UISkin::rounded_rect(pill, size.y / 2.f);
    shape.setFillColor(FILL);
    shape.setOutlineThickness(4.f);
    shape.setOutlineColor(UISkin::OUTLINE);
    window.draw(shape);

    // The tail's root over the pill's outline, so both read as one shape
    sf::ConvexShape root(3);
    for (std::size_t i = 0; i < 3; ++i) root.setPoint(i, tail.getPoint(i));
    root.setScale({ 0.82f, 0.82f });
    root.setOrigin(tail.getPoint(0));
    root.setPosition(tail.getPoint(0));
    root.setFillColor(FILL);
    window.draw(root);

    UISkin::set_text_anchor(m_text, { 0.5f, 0.5f });
    m_text.setScale({ pop, pop });
    m_text.setPosition(pill.position + pill.size / 2.f - sf::Vector2f{ 0.f, 2.f });
    window.draw(m_text);
}
