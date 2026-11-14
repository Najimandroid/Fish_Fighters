#include "UIIconTile.h"
#include "../UISkin.h"

#include <algorithm>
#include <cmath>

namespace
{
    const float CORNER_SCALE = 0.7f;
    const float LABEL_ROOM = 0.26f;          // The bottom part of the tile its name takes
    const unsigned int LABEL_SIZE = 30;
    const sf::Color DISABLED_TINT{ 165, 165, 170 };

    sf::Color faded(sf::Color color, std::uint8_t alpha) { color.a = static_cast<std::uint8_t>(color.a * alpha / 255); return color; }
}

UIIconTile::UIIconTile(sf::Vector2f size, sf::Vector2f position, const std::string& label, Icon icon, const std::string& texturePath) :
    UIButtonElement(size, position, label),
    m_icon(icon),
    m_texturePath(texturePath)
{
    // The name fits the tile: smaller for a long one on a narrow tile (ENCYCLOPEDIA on PC's three tiles)
    unsigned int labelSize = LABEL_SIZE;
    set_text_size(labelSize);
    while (labelSize > 20 && m_text.getLocalBounds().size.x > size.x - 22.f) set_text_size(--labelSize);
    set_style(Style::Secondary);

    // An image is fitted by what's drawn on it, not its transparent margins (read once)
    if (icon == Icon::TEXTURE)
    {
        const sf::Texture& texture = UISkin::texture_file(texturePath);
        const sf::Image image = texture.copyToImage();
        const sf::Vector2u imageSize = image.getSize();
        unsigned int left = imageSize.x, top = imageSize.y, right = 0, bottom = 0;
        for (unsigned int y = 0; y < imageSize.y; ++y)
            for (unsigned int x = 0; x < imageSize.x; ++x)
                if (image.getPixel({ x, y }).a > 16)
                {
                    left = std::min(left, x); right = std::max(right, x);
                    top = std::min(top, y); bottom = std::max(bottom, y);
                }
        m_source = right >= left && bottom >= top
            ? sf::IntRect{ { static_cast<int>(left), static_cast<int>(top) }, { static_cast<int>(right - left + 1), static_cast<int>(bottom - top + 1) } }
            : sf::IntRect{ { 0, 0 }, sf::Vector2i(imageSize) };
    }
}

void UIIconTile::update(float deltaTime)
{
    UIButtonElement::update(deltaTime);
    // The name along the bottom
    const sf::Vector2f size = m_shape.getSize();
    m_text.setPosition(m_shape.getPosition() + sf::Vector2f{ size.x / 2.f, size.y * (1.f - LABEL_ROOM / 2.f) - 4.f });
}

void UIIconTile::render(sf::RenderWindow& window)
{
    if (!m_isVisible) return;
    const float scale = get_visual_scale();
    if (scale < 0.02f) return;

    const sf::FloatRect rect = get_visual_rect();
    const float drop = (m_isPressed ? 0.4f : 1.f) * UISkin::SHADOW_DROP * scale;
    UISkin::draw_nine_slice(window, UISkin::BUTTON_STEEL, { rect.position + sf::Vector2f{ 0.f, drop }, rect.size }, CORNER_SCALE, UISkin::SHADOW_TINT);
    sf::Color tint = m_isEnabled ? m_tint : DISABLED_TINT;
    if (m_isPressed) tint = tint * sf::Color(215, 215, 215);
    UISkin::draw_nine_slice(window, UISkin::BUTTON_STEEL, rect, CORNER_SCALE, tint);

    // The picture in the part above the name (an image gets more of it: a wide clam would be tiny otherwise)
    const float inset = std::min(rect.size.x, rect.size.y) * (m_icon == Icon::TEXTURE ? 0.08f : 0.14f);
    const sf::FloatRect area{ { rect.position.x + inset, rect.position.y + inset * 0.8f },
        { rect.size.x - 2.f * inset, rect.size.y * (1.f - LABEL_ROOM) - inset * 0.8f } };
    draw_icon(window, area, scale, m_isEnabled ? 255 : 120);

    // The name, around the visual center (the tile scales around its middle)
    const sf::Vector2f center = rect.position + rect.size / 2.f;
    const sf::Vector2f offset = m_text.getPosition() - (m_shape.getPosition() + m_shape.getSize() / 2.f);
    const sf::Vector2f restore = m_text.getPosition();
    m_text.setScale({ scale, scale });
    m_text.setPosition(center + offset * scale);
    const sf::Color color = m_text.getFillColor();
    if (!m_isEnabled) UISkin::set_text_color(m_text, color * sf::Color(190, 190, 190));
    window.draw(m_text);
    UISkin::set_text_color(m_text, color);
    m_text.setPosition(restore);
}

/*
 * The pictures, drawn in the game's style (thick dark outlines, flat colors) inside area
 */
void UIIconTile::draw_icon(sf::RenderWindow& window, const sf::FloatRect& area, float scale, std::uint8_t alpha) const
{
    const float side = std::min(area.size.x, area.size.y);
    const sf::Vector2f c = area.position + area.size / 2.f;
    const float outline = 5.f * scale;
    const sf::Color dark = faded(UISkin::OUTLINE, alpha);

    switch (m_icon)
    {
    case Icon::TEXTURE:
        UISkin::draw_fitted(window, UISkin::texture_file(m_texturePath), m_source, area, sf::Color(255, 255, 255, alpha));
        break;

    case Icon::BOOK:
    {
        // A closed book, its pages showing on the right, a fish emblem on the cover
        const sf::Vector2f size{ side * 0.66f, side * 0.84f };
        const sf::FloatRect pages{ c - size / 2.f + sf::Vector2f{ side * 0.06f, 0.f }, size };
        sf::ConvexShape back = UISkin::rounded_rect(pages, side * 0.06f);
        back.setFillColor(faded(sf::Color(248, 244, 232), alpha));
        back.setOutlineThickness(outline);
        back.setOutlineColor(dark);
        window.draw(back);
        const sf::FloatRect coverRect{ c - size / 2.f - sf::Vector2f{ side * 0.04f, side * 0.03f }, size - sf::Vector2f{ side * 0.04f, 0.f } };
        sf::ConvexShape cover = UISkin::rounded_rect(coverRect, side * 0.06f);
        cover.setFillColor(faded(sf::Color(222, 88, 76), alpha));
        cover.setOutlineThickness(outline);
        cover.setOutlineColor(dark);
        window.draw(cover);
        sf::RectangleShape spine({ side * 0.08f, coverRect.size.y - outline });
        spine.setPosition(coverRect.position + sf::Vector2f{ side * 0.05f, outline / 2.f });
        spine.setFillColor(faded(sf::Color(170, 56, 50), alpha));
        window.draw(spine);
        // The fish: an oval body and a tail
        const sf::Vector2f fish = coverRect.position + coverRect.size / 2.f + sf::Vector2f{ side * 0.04f, 0.f };
        sf::CircleShape body(side * 0.12f);
        body.setScale({ 1.f, 0.7f });
        body.setOrigin({ side * 0.12f, side * 0.12f });
        body.setPosition(fish);
        body.setFillColor(faded(UISkin::NUMBER, alpha));
        body.setOutlineThickness(outline * 0.8f);
        body.setOutlineColor(dark);
        sf::ConvexShape tail(3);
        tail.setPoint(0, fish + sf::Vector2f{ side * 0.09f, 0.f });
        tail.setPoint(1, fish + sf::Vector2f{ side * 0.2f, -side * 0.08f });
        tail.setPoint(2, fish + sf::Vector2f{ side * 0.2f, side * 0.08f });
        tail.setFillColor(faded(UISkin::NUMBER, alpha));
        tail.setOutlineThickness(outline * 0.8f);
        tail.setOutlineColor(dark);
        window.draw(tail);
        window.draw(body);
        break;
    }

    case Icon::GEAR:
    {
        // Eight teeth around a ring
        const float outer = side * 0.42f, inner = side * 0.32f;
        const int teeth = 8, perTooth = 4;
        sf::ConvexShape gear(teeth * perTooth);
        for (int i = 0; i < teeth * perTooth; ++i)
        {
            const float angle = (static_cast<float>(i) / (teeth * perTooth)) * 6.2831853f;
            const float radius = (i % perTooth == 1 || i % perTooth == 2) ? outer : inner;
            gear.setPoint(static_cast<std::size_t>(i), c + sf::Vector2f{ std::cos(angle), std::sin(angle) } * radius);
        }
        gear.setFillColor(faded(sf::Color(200, 210, 224), alpha));
        gear.setOutlineThickness(outline);
        gear.setOutlineColor(dark);
        window.draw(gear);
        sf::CircleShape hole(side * 0.13f);
        hole.setOrigin({ side * 0.13f, side * 0.13f });
        hole.setPosition(c);
        hole.setFillColor(faded(sf::Color(64, 80, 104), alpha));
        hole.setOutlineThickness(outline);
        hole.setOutlineColor(dark);
        window.draw(hole);
        break;
    }

    case Icon::POWER:
    {
        // The power symbol: a ring open at the top, a bar through the gap (an outline, then the color)
        const float radius = side * 0.3f;
        for (const bool isOutline : { true, false })
        {
            const float thickness = side * 0.11f + (isOutline ? outline * 2.f : 0.f);
            const sf::Color color = isOutline ? dark : faded(sf::Color(255, 140, 118), alpha);
            sf::VertexArray ring(sf::PrimitiveType::TriangleStrip);
            for (int i = 0; i <= 40; ++i)
            {
                const float angle = (-90.f + 40.f + 280.f * static_cast<float>(i) / 40.f) * 3.14159265f / 180.f;
                const sf::Vector2f direction{ std::cos(angle), std::sin(angle) };
                ring.append({ c + direction * (radius - thickness / 2.f), color });
                ring.append({ c + direction * (radius + thickness / 2.f), color });
            }
            window.draw(ring);
            const float barWidth = thickness;
            sf::ConvexShape bar = UISkin::rounded_rect({ { c.x - barWidth / 2.f, c.y - radius - side * 0.12f - (isOutline ? outline : 0.f) },
                { barWidth, radius + side * 0.06f + (isOutline ? outline * 2.f : 0.f) } }, barWidth / 2.f);
            bar.setFillColor(color);
            window.draw(bar);
        }
        break;
    }
    }
}
