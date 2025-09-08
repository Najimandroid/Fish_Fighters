#pragma once

#include "UIElement.h"

class UITextElement : public UIElement
{
public:

	UITextElement(sf::Vector2f size, sf::Vector2f position);
	UITextElement(sf::Vector2f size, sf::Vector2f position, const std::string& text);
	UITextElement(sf::Vector2f size, sf::Vector2f position, const std::string& text, float characterSize);

	void set_text(const std::string& text);
	void set_text_color(const sf::Color& color);
	void set_character_size(unsigned int size);

	void update(float deltaTime) override;
	void render(sf::RenderWindow& window) override;

protected:

	sf::Text m_text;
	sf::Font m_font;
};