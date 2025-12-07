#include "UIElement.h"

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
