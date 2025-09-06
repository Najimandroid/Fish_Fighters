#include "UIButtonElement.h"

#include "SFML/Window/Event.hpp"

#include <iostream>

UIButtonElement::UIButtonElement(sf::Vector2f size, sf::Vector2f position)
	:m_shape(size)
{
	m_shape.setPosition(position);
	m_shape.setFillColor(sf::Color(255, 225, 145));
}

void UIButtonElement::update(float deltaTime)
{
}

void UIButtonElement::render(sf::RenderWindow& window)
{
	if(m_isVisible)
		window.draw(m_shape);
}

void UIButtonElement::set_callback(std::function<void()> callback)
{
	m_callback = callback;
}

void UIButtonElement::handle_event(const sf::Event& event, const sf::RenderWindow& window)
{
    if (const auto* mouseButtonPressed = event.getIf<sf::Event::MouseButtonPressed>())
    {
        if (mouseButtonPressed->button == sf::Mouse::Button::Left) {

            sf::Vector2f worldPos = window.mapPixelToCoords(mouseButtonPressed->position);

            if (m_shape.getGlobalBounds().contains(static_cast<sf::Vector2f>(worldPos)))
            {
				std::cout << "Button clicked\n";    
                if (m_callback) m_callback();
            }
        }
    }
}
