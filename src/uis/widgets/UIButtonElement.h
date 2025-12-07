#pragma once

#include "UIElement.h"
#include <functional>
#include <optional>

/*
 * UIButtonElement represents a clickable button in the UI.
 * Inherits from UIElement and adds:
 *  - Rectangle shape
 *  - Text label
 *  - Callback function for click events
 */

class UIButtonElement : public UIElement
{
public:

    // Constructors
    UIButtonElement(sf::Vector2f size, sf::Vector2f position);
    UIButtonElement(sf::Vector2f size, sf::Vector2f position, const std::string& text);

    // Set the function to call when the button is clicked
    void set_callback(std::function<void()> callback);

    // Centers the text inside the button rectangle
    void center_text();

    // Text setters
    void set_text(const std::string& text);
    void set_text_size(unsigned int size);
    void set_text_color(const sf::Color& color);

    // Shape appearance setters
    void set_fill_color(const sf::Color& color);
    void set_outline(float thickness, const sf::Color& color);

    // Override UIElement functions
    void update(float deltaTime) override;
    void render(sf::RenderWindow& window) override;
    void handle_event(const sf::Event& event, const sf::RenderWindow& window) override;
    sf::FloatRect get_bounds() const override;

protected:

    sf::RectangleShape m_shape;         // Button background rectangle
    sf::Text m_text;                    // Button label text
    sf::Font m_font;                    // Font used for the text
    std::function<void()> m_callback;   // Function to execute on click
};
