#pragma once

#include "UIElement.h"

#include <functional>
#include <optional>

class UIButtonElement : public UIElement
{
public:

	UIButtonElement(sf::Vector2f size, sf::Vector2f position);
	UIButtonElement(sf::Vector2f size, sf::Vector2f position, const std::string& text);

	void set_callback(std::function<void()> callback);

	void center_text();

	void set_text(const std::string& text);
	void set_text_size(unsigned int size);
	void set_text_color(const sf::Color& color);

	void set_fill_color(const sf::Color& color);
	void set_outline(float thickness, const sf::Color& color);

	void update(float deltaTime) override;
	void render(sf::RenderWindow& window) override;
	void handle_event(const sf::Event& event, const sf::RenderWindow& window) override;

protected:

	sf::RectangleShape m_shape;

	sf::Text m_text;
	sf::Font m_font;

	std::function<void()> m_callback;
};