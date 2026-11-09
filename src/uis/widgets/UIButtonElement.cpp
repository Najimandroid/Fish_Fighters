#include "UIButtonElement.h"
#include "../UISkin.h"
#include "SFML/Window/Event.hpp"

namespace
{
    const float HOVER_SCALE = 1.04f;              // Size under the mouse
    const float PRESSED_SCALE = 0.94f;            // Size while pressed
    const float CORNER_SCALE = 0.7f;              // Corners of the yellow image on screen (outline about 5 px)
    const sf::Color PRESSED_TINT{ 215, 215, 215 };
    const sf::Color DISABLED_TINT{ 140, 140, 140 };
    const sf::Color DISABLED_TEXT{ 190, 190, 190 };
}

UIButtonElement::UIButtonElement(sf::Vector2f size, sf::Vector2f position)
    : UIButtonElement(size, position, "Button") // default text "Button"
{
}

UIButtonElement::UIButtonElement(sf::Vector2f size, sf::Vector2f position, const std::string& text)
    : UIElement(size, position),
    m_shape(size),
    m_text(UISkin::heavy_font(), text, 30)
{
    UISkin::style_text(m_text, 30);
    m_shape.setPosition(position);
    center_text();
}

void UIButtonElement::set_callback(std::function<void()> callback)
{
    m_callback = callback;
}

void UIButtonElement::center_text()
{
    UISkin::set_text_anchor(m_text, { 0.5f, 0.5f });
    m_text.setPosition({
        m_shape.getPosition().x + m_shape.getSize().x / 2.f,
        m_shape.getPosition().y + m_shape.getSize().y / 2.f
        });
}

void UIButtonElement::set_text(const std::string& text)
{
    m_text.setString(text);
}

// Set the text size; the outline follows
void UIButtonElement::set_text_size(unsigned int size)
{
    UISkin::style_text(m_text, size, m_text.getFillColor());
}

void UIButtonElement::set_text_color(const sf::Color& color)
{
    UISkin::set_text_color(m_text, color);
}

void UIButtonElement::set_fill_color(const sf::Color& color)
{
    m_tint = color;
}

void UIButtonElement::set_enabled(bool isEnabled)
{
    m_isEnabled = isEnabled;
}

bool UIButtonElement::is_enabled() const
{
    return m_isEnabled;
}

// Update function called every frame (currently ensures text stays centered)
void UIButtonElement::update(float deltaTime)
{
    center_text();
}

float UIButtonElement::get_visual_scale() const
{
    if (!m_isEnabled) return 1.f;
    if (m_isPressed) return PRESSED_SCALE;
    if (m_isHovered) return HOVER_SCALE;
    return 1.f;
}

sf::FloatRect UIButtonElement::get_visual_rect() const
{
    const float scale = get_visual_scale();
    const sf::Vector2f size = m_shape.getSize() * scale;
    const sf::Vector2f center = m_shape.getPosition() + m_shape.getSize() / 2.f;
    return sf::FloatRect{ center - size / 2.f, size };
}

// Render the yellow button image, then the label at the same scale
void UIButtonElement::render(sf::RenderWindow& window)
{
    if (!m_isVisible) return;

    sf::Color tint = m_tint;
    if (!m_isEnabled) tint = tint * DISABLED_TINT;
    else if (m_isPressed) tint = tint * PRESSED_TINT;

    UISkin::draw_nine_slice(window, UISkin::BUTTON_YELLOW, get_visual_rect(), CORNER_SCALE, tint);

    const float scale = get_visual_scale();
    const sf::Color textColor = m_text.getFillColor();
    if (!m_isEnabled) UISkin::set_text_color(m_text, textColor * DISABLED_TEXT);
    m_text.setScale({ scale, scale });
    window.draw(m_text);
    UISkin::set_text_color(m_text, textColor);
}

// Track the mouse (hover / press look) and run the callback when the button is clicked
void UIButtonElement::handle_event(const sf::Event& event, const sf::RenderWindow& window)
{
    if (!m_isVisible) return;

    if (const auto* mouseMoved = event.getIf<sf::Event::MouseMoved>())
    {
        // Convert pixel position to world coordinates
        m_isHovered = get_bounds().contains(window.mapPixelToCoords(mouseMoved->position));
        if (!m_isHovered) m_isPressed = false;
    }
    else if (const auto* mouseButtonPressed = event.getIf<sf::Event::MouseButtonPressed>())
    {
        if (mouseButtonPressed->button == sf::Mouse::Button::Left
            && get_bounds().contains(window.mapPixelToCoords(mouseButtonPressed->position)))
        {
            m_isPressed = true;
            if (m_isEnabled && m_callback) m_callback(); // execute callback if set
        }
    }
    else if (const auto* mouseButtonReleased = event.getIf<sf::Event::MouseButtonReleased>())
    {
        if (mouseButtonReleased->button == sf::Mouse::Button::Left) m_isPressed = false;
    }
}

// Return the button's bounding rectangle for collision or interaction checks
sf::FloatRect UIButtonElement::get_bounds() const
{
    return m_shape.getGlobalBounds();
}
