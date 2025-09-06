#pragma once

#include <SFML/Graphics.hpp>

class UIElement
{
public:

	UIElement() = default;
	UIElement(sf::Vector2f size, sf::Vector2f position);
	virtual ~UIElement() = default;

	virtual void update(float deltaTime) = 0;
	virtual void render(sf::RenderWindow& window) = 0;
	virtual void handle_event(const sf::Event& event, const sf::RenderWindow& window) {}

protected:

	sf::Vector2f m_position = { 0.0f, 0.0f };
	sf::Vector2f m_size = { 0.0f, 0.0f };

	bool m_isVisible = true;
};