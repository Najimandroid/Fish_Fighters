#include "UIButtonElement.h"
#include "SFML/Window/Event.hpp"
#include <iostream>

// Constructor with size and position, default text
UIButtonElement::UIButtonElement(sf::Vector2f size, sf::Vector2f position)
    : m_shape(size),
    m_text(m_font, "Button", 30) // default text "Button"
{
    // Load font from file
    auto success = m_font.openFromFile("assets/fonts/MPLUSRounded1c-Medium.ttf");

    // Configure text appearance
    m_text.setPosition(position);
    m_text.setFillColor(sf::Color::White);
    m_text.setOutlineColor(sf::Color::Black);
    m_text.setOutlineThickness(1.5f);

    // Configure button shape
    m_shape.setPosition(position);
    m_shape.setFillColor(sf::Color(255, 225, 145));

    // Center the text inside the button
    center_text();
}

// Constructor with size, position, and custom text
UIButtonElement::UIButtonElement(sf::Vector2f size, sf::Vector2f position, const std::string& text)
    : m_shape(size),
    m_text(m_font, text, 30)
{
    // Load font from file
    auto success = m_font.openFromFile("assets/fonts/MPLUSRounded1c-Medium.ttf");

    // Configure text appearance
    m_text.setPosition(position);
    m_text.setFillColor(sf::Color::White);
    m_text.setOutlineColor(sf::Color::Black);
    m_text.setOutlineThickness(1.5f);

    // Configure button shape
    m_shape.setPosition(position);
    m_shape.setFillColor(sf::Color(255, 225, 145));

    // Center the text inside the button
    center_text();
}

// Set the callback to be executed on button click
void UIButtonElement::set_callback(std::function<void()> callback)
{
    m_callback = callback;
}

// Center the text inside the button
// --- FUNCTION IS LEFT COMPLETELY AS-IS ---
void UIButtonElement::center_text()
{
    sf::FloatRect textBounds = m_text.getLocalBounds();

    m_text.setOrigin({
        textBounds.position.x + textBounds.size.x / 2.f,
        textBounds.position.y + textBounds.size.y / 2.f
        });

    m_text.setPosition({
        m_shape.getPosition().x + m_shape.getSize().x / 2.f,
        m_shape.getPosition().y + m_shape.getSize().y / 2.f
        });
}

// Set the button's text string
void UIButtonElement::set_text(const std::string& text)
{
    m_text.setString(text);
}

// Set the text size (character size)
void UIButtonElement::set_text_size(unsigned int size)
{
    m_text.setCharacterSize(size);
}

// Set the text fill color
void UIButtonElement::set_text_color(const sf::Color& color)
{
    m_text.setFillColor(color);
}

// Set the button's fill color
void UIButtonElement::set_fill_color(const sf::Color& color)
{
    m_shape.setFillColor(color);
}

// Set the button's outline thickness and color
void UIButtonElement::set_outline(float thickness, const sf::Color& color)
{
    m_shape.setOutlineThickness(thickness);
    m_shape.setOutlineColor(color);
}

// Update function called every frame (currently ensures text stays centered)
void UIButtonElement::update(float deltaTime)
{
    center_text();
}

// Render the button (shape and text)
void UIButtonElement::render(sf::RenderWindow& window)
{
    if (m_isVisible)
    {
        window.draw(m_shape);
        window.draw(m_text);
    }
}

// Handle input events (detect mouse clicks on the button)
void UIButtonElement::handle_event(const sf::Event& event, const sf::RenderWindow& window)
{
    if (const auto* mouseButtonPressed = event.getIf<sf::Event::MouseButtonPressed>())
    {
        if (mouseButtonPressed->button == sf::Mouse::Button::Left)
        {
            // Convert pixel position to world coordinates
            sf::Vector2f worldPos = window.mapPixelToCoords(mouseButtonPressed->position);

            // Check if click is inside the button's bounds
            if (m_shape.getGlobalBounds().contains(static_cast<sf::Vector2f>(worldPos)))
            {
                std::cout << "Button clicked\n";
                if (m_callback) m_callback(); // execute callback if set
            }
        }
    }
}

// Return the button's bounding rectangle for collision or interaction checks
sf::FloatRect UIButtonElement::get_bounds() const
{
    return m_shape.getGlobalBounds();
}
