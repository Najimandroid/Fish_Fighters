#pragma once

#include "UIElement.h"

#include <functional>
#include <optional>

class UIButtonElement : public UIElement
{
public:

	UIButtonElement(sf::Vector2f size, sf::Vector2f position);

	void update(float deltaTime) override;
	void render(sf::RenderWindow& window) override;
	void handle_event(const sf::Event& event, const sf::RenderWindow& window) override;

	void set_callback(std::function<void()> callback);

protected:

	sf::RectangleShape m_shape;

	std::function<void()> m_callback;
};