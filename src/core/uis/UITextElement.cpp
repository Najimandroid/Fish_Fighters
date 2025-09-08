#include "UITextElement.h"

UITextElement::UITextElement(sf::Vector2f size, sf::Vector2f position):
	m_text(m_font, "Text", 30)
{
	auto success = m_font.openFromFile("assets/fonts/MPLUSRounded1c-Medium.ttf");

	m_text.setPosition(position);
	m_text.setFillColor(sf::Color::White);
	m_text.setOutlineColor(sf::Color::Black);
	m_text.setOutlineThickness(1.5f);
}

UITextElement::UITextElement(sf::Vector2f size, sf::Vector2f position, const std::string& text):
	m_text(m_font, text, 30)
{
	auto success = m_font.openFromFile("assets/fonts/MPLUSRounded1c-Medium.ttf");

	m_text.setPosition(position);
	m_text.setFillColor(sf::Color::White);
	m_text.setOutlineColor(sf::Color::Black);
	m_text.setOutlineThickness(1.5f);
}

UITextElement::UITextElement(sf::Vector2f size, sf::Vector2f position, const std::string& text, float characterSize):
	m_text(m_font, text, characterSize)
{
	auto success = m_font.openFromFile("assets/fonts/MPLUSRounded1c-Medium.ttf");

	m_text.setPosition(position);
	m_text.setFillColor(sf::Color::White);
	m_text.setOutlineColor(sf::Color::Black);
	m_text.setOutlineThickness(1.5f);
}

void UITextElement::update(float deltaTime)
{
}

void UITextElement::render(sf::RenderWindow& window)
{
	if (m_isVisible)
		window.draw(m_text);
}
