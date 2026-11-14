#include "UIInfoButton.h"
#include "../UISkin.h"
#include "../../core/Platform.h"

namespace
{
    const float SIZE = Platform::IS_MOBILE ? 72.f : 56.f;
    const float TOUCH_MARGIN = Platform::IS_MOBILE ? 18.f : 0.f;
    const sf::Color FILL{ 86, 192, 236 };
}

UIInfoButton::UIInfoButton(sf::Vector2f center) :
    UIButtonElement({ SIZE, SIZE }, center - sf::Vector2f{ SIZE, SIZE } / 2.f, "i")
{
    set_text_size(static_cast<unsigned int>(SIZE * 0.72f));
    set_click_sound("ui_click");
}

void UIInfoButton::render(sf::RenderWindow& window)
{
    if (!m_isVisible) return;
    const float scale = get_visual_scale();
    if (scale < 0.02f) return;

    const sf::FloatRect rect = get_visual_rect();
    const float radius = rect.size.x / 2.f;
    sf::CircleShape circle(radius);
    circle.setOrigin({ radius, radius });
    const sf::Vector2f center = rect.position + rect.size / 2.f;

    circle.setPosition(center + sf::Vector2f{ 0.f, (m_isPressed ? 0.4f : 1.f) * UISkin::SHADOW_DROP * 0.7f * scale });
    circle.setFillColor(UISkin::SHADOW_TINT);
    window.draw(circle);

    circle.setPosition(center);
    circle.setFillColor(m_isPressed ? FILL * sf::Color(215, 215, 215) : FILL);
    circle.setOutlineThickness(4.f * scale);
    circle.setOutlineColor(UISkin::OUTLINE);
    window.draw(circle);

    m_text.setScale({ scale, scale });
    m_text.setPosition(center + sf::Vector2f{ 0.f, -radius * 0.06f });
    window.draw(m_text);
}

sf::FloatRect UIInfoButton::get_bounds() const
{
    const sf::FloatRect bounds = m_shape.getGlobalBounds();
    return { bounds.position - sf::Vector2f{ TOUCH_MARGIN, TOUCH_MARGIN }, bounds.size + sf::Vector2f{ TOUCH_MARGIN, TOUCH_MARGIN } * 2.f };
}
