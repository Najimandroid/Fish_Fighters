#include "UITextElement.h"
#include "../UISkin.h"

// Constructor with size and position, default text "Text"
UITextElement::UITextElement(sf::Vector2f size, sf::Vector2f position)
    : UITextElement(size, position, "Text", 30.f) // default character size 30
{
}

// Constructor with custom text
UITextElement::UITextElement(sf::Vector2f size, sf::Vector2f position, const std::string& text)
    : UITextElement(size, position, text, 30.f) // default character size 30
{
}

// Constructor with custom text and custom character size
// The text uses the heavy font of the skin, white with a dark outline
UITextElement::UITextElement(sf::Vector2f size, sf::Vector2f position, const std::string& text, float characterSize)
    : UIElement(size, position),
    m_text(UISkin::heavy_font(), text)
{
    UISkin::style_text(m_text, static_cast<unsigned int>(characterSize));
    m_text.setPosition(position);
}

// Set the displayed text string
void UITextElement::set_text(const std::string& text)
{
    m_text.setString(text);
}

// Set the fill color of the text
// The outline gets the same transparency (a hidden text hides its outline too)
void UITextElement::set_text_color(const sf::Color& color)
{
    UISkin::set_text_color(m_text, color);
}

// Set the character size of the text; the outline follows
void UITextElement::set_character_size(unsigned int size)
{
    UISkin::style_text(m_text, size, m_text.getFillColor());
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
