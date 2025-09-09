#include "UIElement.h"

UIElement::UIElement(sf::Vector2f size, sf::Vector2f position)
	: m_size(size), m_position(position)
{}

void UIElement::set_position(sf::Vector2f newPosition)
{
	m_position = newPosition;
}

void UIElement::set_size(sf::Vector2f newSize)
{
	m_size = newSize;
}

sf::Vector2f UIElement::get_position() const
{
	return m_position;
}

sf::Vector2f UIElement::get_size() const
{
	return m_size;
}

sf::FloatRect UIElement::get_bounds() const
{
	return sf::FloatRect{ m_position, m_size };
}
