#pragma once

#include "UIElement.h"

#include <functional>
#include <optional>

class UIButtonElement : public UIElement
{
public:

	UIButtonElement(sf::Vector2f size, sf::Vector2f position);
	UIButtonElement(sf::Vector2f size, sf::Vector2f position, const std::string& text);

	void update(float deltaTime) override;
	void render(sf::RenderWindow& window) override;
	void handle_event(const sf::Event& event, const sf::RenderWindow& window) override;

	void set_callback(std::function<void()> callback);

protected:

	sf::RectangleShape m_shape;

	sf::Text m_text;
	sf::Font m_font;

	std::function<void()> m_callback;
};