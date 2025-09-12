#pragma once

#include "UIElement.h"

/*
 * UITextElement
 * -------------
 * A UI element that displays text on the screen.
 * Supports changing text, color, and character size.
 * Can be positioned in screen-space or world-space.
 */

class UITextElement : public UIElement
{
public:

    // Constructor: default text "Text" and character size 30
    UITextElement(sf::Vector2f size, sf::Vector2f position);

    // Constructor: custom text with default character size 30
    UITextElement(sf::Vector2f size, sf::Vector2f position, const std::string& text);

    // Constructor: custom text with custom character size
    UITextElement(sf::Vector2f size, sf::Vector2f position, const std::string& text, float characterSize);

    // Set the displayed text string
    void set_text(const std::string& text);

    // Set the text fill color
    void set_text_color(const sf::Color& color);

    // Set the character size of the text
    void set_character_size(unsigned int size);

    // Called every frame to update the element
    void update(float deltaTime) override;

    // Draw the text to the given window
    void render(sf::RenderWindow& window) override;

protected:

    sf::Text m_text;
    sf::Font m_font;
};
