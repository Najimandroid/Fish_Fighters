#include "UITextElement.h"

// Constructor with size and position, default text "Text"
UITextElement::UITextElement(sf::Vector2f size, sf::Vector2f position)
    : m_text(m_font, "Text", 30) // default character size 30
{
    // Load font from file
    auto success = m_font.openFromFile("assets/fonts/MPLUSRounded1c-Medium.ttf");

    // Configure text appearance
    m_text.setPosition(position);
    m_text.setFillColor(sf::Color::White);
    m_text.setOutlineColor(sf::Color::Black);
    m_text.setOutlineThickness(1.5f);
}

// Constructor with custom text
UITextElement::UITextElement(sf::Vector2f size, sf::Vector2f position, const std::string& text)
    : m_text(m_font, text, 30) // default character size 30
{
    // Load font from file
    auto success = m_font.openFromFile("assets/fonts/MPLUSRounded1c-Medium.ttf");

    // Configure text appearance
    m_text.setPosition(position);
    m_text.setFillColor(sf::Color::White);
    m_text.setOutlineColor(sf::Color::Black);
    m_text.setOutlineThickness(1.5f);
}

// Constructor with custom text and custom character size
UITextElement::UITextElement(sf::Vector2f size, sf::Vector2f position, const std::string& text, float characterSize)
    : m_text(m_font, text, characterSize)
{
    // Load font from file
    auto success = m_font.openFromFile("assets/fonts/MPLUSRounded1c-Medium.ttf");

    // Configure text appearance
    m_text.setPosition(position);
    m_text.setFillColor(sf::Color::White);
    m_text.setOutlineColor(sf::Color::Black);
    m_text.setOutlineThickness(1.5f);
}

// Set the displayed text string
void UITextElement::set_text(const std::string& text)
{
    m_text.setString(text);
}

// Set the fill color of the text
void UITextElement::set_text_color(const sf::Color& color)
{
    m_text.setFillColor(color);

    if(color.a == 0)
        m_text.setOutlineColor(sf::Color(0, 0, 0, 0));
}

// Set the character size of the text
void UITextElement::set_character_size(unsigned int size)
{
    m_text.setCharacterSize(size);
}

// Update function called every frame
void UITextElement::update(float deltaTime)
{
    m_text.setPosition(m_position);
}

// Render the text if visible
void UITextElement::render(sf::RenderWindow& window)
{
    if (m_isVisible)
        window.draw(m_text);
}

sf::Text& UITextElement::get_text()
{
    return m_text;
}
