#include "UITitleBar.h"
#include "../UISkin.h"

#include <algorithm>

namespace
{
    const float SCREEN_WIDTH = 1920.f;
    const unsigned int TITLE_SIZE = 56;
    const float BAND_BOTTOM = 104.f;       // The band's bottom edge (the screen's content starts under HEIGHT)
    const float TITLE_X = 40.f;
}

UITitleBar::UITitleBar(const std::string& title, Material material) :
    UIElement({ SCREEN_WIDTH, HEIGHT }, { 0.f, 0.f }),
    m_title(UISkin::heavy_font(), title),
    m_material(material)
{
    UISkin::style_text(m_title, TITLE_SIZE, UISkin::TITLE);
    set_title(title);
}

void UITitleBar::set_title(const std::string& title)
{
    m_title.setString(title);
    UISkin::set_text_anchor(m_title, { 0.f, 0.5f });
    m_title.setPosition({ TITLE_X, BAND_BOTTOM / 2.f - 6.f });
}

void UITitleBar::render(sf::RenderWindow& window)
{
    if (!m_isVisible) return;

    // A plain steel band across the whole screen (the side frame covers its ends on a phone): lighter at the top,
    // a bright bevel and a dark edge at the bottom, a shadow on the screen under it, two rivets at each end
    const sf::FloatRect screen = UISkin::screen_rect();
    const float left = std::min(0.f, screen.position.x), right = std::max(SCREEN_WIDTH, screen.position.x + screen.size.x);
    const float top = screen.position.y;
    sf::VertexArray band(sf::PrimitiveType::TriangleStrip, 4);
    // Its colors: top, bottom, the bevel, the rivets (pearls on mother-of-pearl)
    struct Look { sf::Color top, bottom, bevel, rivet; };
    const Look look = m_material == Material::NACRE ? Look{ { 255, 232, 244 }, { 236, 186, 218 }, { 255, 252, 255 }, { 255, 250, 255 } }
        : m_material == Material::VIOLET ? Look{ { 78, 60, 126 }, { 46, 34, 86 }, { 152, 130, 204 }, { 132, 112, 182 } }
        : Look{ { 82, 100, 126 }, { 54, 68, 90 }, { 132, 152, 178 }, { 120, 138, 162 } };
    band[0] = sf::Vertex{ { left, top }, look.top };
    band[1] = sf::Vertex{ { right, top }, look.top };
    band[2] = sf::Vertex{ { left, BAND_BOTTOM }, look.bottom };
    band[3] = sf::Vertex{ { right, BAND_BOTTOM }, look.bottom };
    window.draw(band);

    sf::RectangleShape line({ right - left, 3.f });
    line.setPosition({ left, BAND_BOTTOM - 9.f });
    line.setFillColor(look.bevel);
    window.draw(line);
    line.setSize({ right - left, 5.f });
    line.setPosition({ left, BAND_BOTTOM - 5.f });
    line.setFillColor(UISkin::OUTLINE);
    window.draw(line);
    sf::VertexArray shadow(sf::PrimitiveType::TriangleStrip, 4);
    shadow[0] = sf::Vertex{ { left, BAND_BOTTOM }, sf::Color(0, 0, 0, 90) };
    shadow[1] = sf::Vertex{ { right, BAND_BOTTOM }, sf::Color(0, 0, 0, 90) };
    shadow[2] = sf::Vertex{ { left, BAND_BOTTOM + 18.f }, sf::Color(0, 0, 0, 0) };
    shadow[3] = sf::Vertex{ { right, BAND_BOTTOM + 18.f }, sf::Color(0, 0, 0, 0) };
    window.draw(shadow);

    sf::CircleShape rivet(6.f);
    rivet.setOrigin({ 6.f, 6.f });
    rivet.setFillColor(look.rivet);
    rivet.setOutlineThickness(2.5f);
    rivet.setOutlineColor(UISkin::OUTLINE);
    for (const float x : { 14.f, 34.f, SCREEN_WIDTH - 34.f, SCREEN_WIDTH - 14.f })
    {
        rivet.setPosition({ x, BAND_BOTTOM - 24.f });
        window.draw(rivet);
    }
    window.draw(m_title);
}

sf::Vector2f UITitleBar::get_info_position() const
{
    const sf::FloatRect bounds = m_title.getGlobalBounds();
    return { bounds.position.x + bounds.size.x + 46.f, BAND_BOTTOM / 2.f - 4.f };
}
