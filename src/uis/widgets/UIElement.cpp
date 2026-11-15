#include "UIElement.h"
#include "../UISkin.h"
#include "../../core/Platform.h"

#include <algorithm>

namespace
{
    // On a phone, the corners stay a little further in than on PC (its rounded corners)
    const float CORNER_INSET = Platform::IS_MOBILE ? 16.f : 0.f;
}

float UIElement::shift_for(ScreenAnchor anchor)
{
    const sf::FloatRect screen = UISkin::screen_rect();
    if (anchor == ScreenAnchor::LEFT) return std::min(0.f, screen.position.x + CORNER_INSET);
    if (anchor == ScreenAnchor::RIGHT) return std::max(0.f, screen.position.x + screen.size.x - 1920.f - CORNER_INSET);
    return 0.f;
}

sf::FloatRect UIElement::get_screen_bounds() const
{
    sf::FloatRect bounds = get_bounds();
    bounds.position.x += screen_shift();
    return bounds;
}

// Constructor: initialize size and position
UIElement::UIElement(sf::Vector2f size, sf::Vector2f position)
    : m_size(size), m_position(position)
{
}

// Set the position of the element
void UIElement::set_position(sf::Vector2f newPosition)
{
    m_position = newPosition;
}

// Set the size of the element
void UIElement::set_size(sf::Vector2f newSize)
{
    m_size = newSize;
}

// Set visibility (true = visible, false = hidden)
void UIElement::set_visible(bool isVisible)
{
    m_isVisible = isVisible;
}

// Return current position
sf::Vector2f UIElement::get_position() const
{
    return m_position;
}

// Return current size
sf::Vector2f UIElement::get_size() const
{
    return m_size;
}

// Return bounding rectangle (used for collision / mouse checks)
sf::FloatRect UIElement::get_bounds() const
{
    return sf::FloatRect{ m_position, m_size };
}

// Check if element is visible
bool UIElement::is_visible() const
{
    return m_isVisible;
}
