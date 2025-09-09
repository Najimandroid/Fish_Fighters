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

	virtual void set_position(sf::Vector2f newPosition);
	virtual void set_size(sf::Vector2f newSize);

	sf::Vector2f get_position() const;
	sf::Vector2f get_size() const;
	virtual sf::FloatRect get_bounds() const;

public:

	bool isWorldSpaceUi = false;

protected:

	sf::Vector2f m_position = { 0.0f, 0.0f };
	sf::Vector2f m_size = { 0.0f, 0.0f };

	bool m_isVisible = true;
};