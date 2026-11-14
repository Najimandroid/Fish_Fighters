#include "UIBottomBar.h"
#include "../UISkin.h"

#include <algorithm>

UIBottomBar::UIBottomBar(UITitleBar::Material material) :
    UIElement({ 1920.f, 1080.f - UISkin::CONTENT_BOTTOM }, { 0.f, UISkin::CONTENT_BOTTOM }),
    m_material(material)
{
}

/*
 * The title bar's band upside down: a shadow it casts up on the screen, a dark edge and a bright bevel at its top,
 * darker at the bottom, two rivets at each end (the back button covers the left ones)
 */
void UIBottomBar::render(sf::RenderWindow& window)
{
    if (!m_isVisible) return;

    const sf::FloatRect screen = UISkin::screen_rect();
    const float left = std::min(0.f, screen.position.x), right = std::max(1920.f, screen.position.x + screen.size.x);
    const float top = UISkin::CONTENT_BOTTOM, bottom = std::max(1080.f, screen.position.y + screen.size.y);
    const UITitleBar::Look look = UITitleBar::look_of(m_material);

    sf::VertexArray shadow(sf::PrimitiveType::TriangleStrip, 4);
    shadow[0] = sf::Vertex{ { left, top - 18.f }, sf::Color(0, 0, 0, 0) };
    shadow[1] = sf::Vertex{ { right, top - 18.f }, sf::Color(0, 0, 0, 0) };
    shadow[2] = sf::Vertex{ { left, top }, sf::Color(0, 0, 0, 90) };
    shadow[3] = sf::Vertex{ { right, top }, sf::Color(0, 0, 0, 90) };
    window.draw(shadow);

    sf::VertexArray band(sf::PrimitiveType::TriangleStrip, 4);
    band[0] = sf::Vertex{ { left, top }, look.top };
    band[1] = sf::Vertex{ { right, top }, look.top };
    band[2] = sf::Vertex{ { left, bottom }, look.bottom };
    band[3] = sf::Vertex{ { right, bottom }, look.bottom };
    window.draw(band);

    sf::RectangleShape line({ right - left, 5.f });
    line.setPosition({ left, top });
    line.setFillColor(UISkin::OUTLINE);
    window.draw(line);
    line.setSize({ right - left, 3.f });
    line.setPosition({ left, top + 6.f });
    line.setFillColor(look.bevel);
    window.draw(line);

    sf::CircleShape rivet(6.f);
    rivet.setOrigin({ 6.f, 6.f });
    rivet.setFillColor(look.rivet);
    rivet.setOutlineThickness(2.5f);
    rivet.setOutlineColor(UISkin::OUTLINE);
    for (const float x : { 14.f, 34.f, 1920.f - 34.f, 1920.f - 14.f })
    {
        rivet.setPosition({ x, top + 26.f });
        window.draw(rivet);
    }
}
