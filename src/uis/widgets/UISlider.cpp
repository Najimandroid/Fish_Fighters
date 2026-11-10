#include "UISlider.h"
#include "../UISkin.h"

#include <algorithm>
#include <cmath>

namespace
{
    const float TRACK_HEIGHT = 22.f;
    const float KNOB_RADIUS = 22.f;
    const float OUTLINE = 4.f;
    const float KEY_STEP = 0.05f;                       // Arrow keys: 5%
    const sf::Color TRACK_COLOR{ 40, 56, 74 };
    const unsigned int VALUE_SIZE = 32;
    const float VALUE_GAP = 40.f;                       // Between the track's end and the % text
}

UISlider::UISlider(sf::Vector2f position, float width, float value) :
    UIElement({ width, TRACK_HEIGHT }, position),
    m_value(std::clamp(value, 0.f, 1.f)),
    m_valueText(UISkin::heavy_font())
{
    UISkin::style_text(m_valueText, VALUE_SIZE, UISkin::NUMBER);
}

void UISlider::set_change_callback(std::function<void(float)> callback)
{
    m_onChange = std::move(callback);
}

void UISlider::set_release_callback(std::function<void(float)> callback)
{
    m_onRelease = std::move(callback);
}

float UISlider::get_value() const
{
    return m_value;
}

void UISlider::set_value(float value)
{
    m_value = std::clamp(value, 0.f, 1.f);
}

void UISlider::set_from_x(float x)
{
    // Steps of 1%, so the shown value is the real one
    const float value = std::round(std::clamp((x - m_position.x) / m_size.x, 0.f, 1.f) * 100.f) / 100.f;
    if (value == m_value) return;
    m_value = value;
    if (m_onChange) m_onChange(m_value);
}

void UISlider::render(sf::RenderWindow& window)
{
    if (!m_isVisible) return;

    // Track, then the filled part
    sf::RectangleShape track(m_size);
    track.setPosition(m_position);
    track.setFillColor(TRACK_COLOR);
    track.setOutlineThickness(OUTLINE);
    track.setOutlineColor(UISkin::OUTLINE);
    window.draw(track);

    sf::RectangleShape fill({ m_size.x * m_value, m_size.y });
    fill.setPosition(m_position);
    fill.setFillColor(UISkin::NUMBER);
    window.draw(fill);

    // Knob: bigger under the mouse or while dragged
    const float radius = KNOB_RADIUS * ((m_isHovered || m_isDragging) ? 1.12f : 1.f);
    sf::CircleShape knob(radius);
    knob.setOrigin({ radius, radius });
    knob.setPosition({ m_position.x + m_size.x * m_value, m_position.y + m_size.y / 2.f });
    knob.setFillColor(UISkin::NUMBER);
    knob.setOutlineThickness(OUTLINE + 1.f);
    knob.setOutlineColor(UISkin::OUTLINE);
    window.draw(knob);

    m_valueText.setString(std::to_string(static_cast<int>(std::round(m_value * 100.f))) + "%");
    UISkin::set_text_anchor(m_valueText, { 1.f, 0.5f });
    m_valueText.setPosition({ m_position.x + m_size.x + VALUE_GAP + 90.f, m_position.y + m_size.y / 2.f });
    window.draw(m_valueText);
}

void UISlider::handle_event(const sf::Event& event, const sf::RenderWindow& window)
{
    if (!m_isVisible) return;

    if (const auto* pressed = event.getIf<sf::Event::MouseButtonPressed>())
    {
        const sf::Vector2f mouse = window.mapPixelToCoords(pressed->position);
        if (pressed->button == sf::Mouse::Button::Left && get_bounds().contains(mouse))
        {
            m_isDragging = true;
            set_from_x(mouse.x);
        }
    }
    else if (const auto* moved = event.getIf<sf::Event::MouseMoved>())
    {
        const sf::Vector2f mouse = window.mapPixelToCoords(moved->position);
        m_isHovered = get_bounds().contains(mouse);
        if (m_isDragging) set_from_x(mouse.x);
    }
    else if (const auto* released = event.getIf<sf::Event::MouseButtonReleased>())
    {
        if (released->button == sf::Mouse::Button::Left && m_isDragging)
        {
            m_isDragging = false;
            if (m_onRelease) m_onRelease(m_value);
        }
    }
    else if (const auto* key = event.getIf<sf::Event::KeyPressed>())
    {
        if (!m_isHovered) return;
        const float step = key->code == sf::Keyboard::Key::Left ? -KEY_STEP : key->code == sf::Keyboard::Key::Right ? KEY_STEP : 0.f;
        if (step == 0.f) return;
        set_from_x(m_position.x + (m_value + step) * m_size.x);
        if (m_onRelease) m_onRelease(m_value);
    }
}

sf::FloatRect UISlider::get_bounds() const
{
    return sf::FloatRect{ m_position - sf::Vector2f{ KNOB_RADIUS, KNOB_RADIUS - TRACK_HEIGHT / 2.f },
        m_size + sf::Vector2f{ KNOB_RADIUS * 2.f, (KNOB_RADIUS - TRACK_HEIGHT / 2.f) * 2.f } };
}
