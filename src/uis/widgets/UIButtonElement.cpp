#include "UIButtonElement.h"
#include "../UISkin.h"
#include "SFML/Window/Event.hpp"

#include <algorithm>
#include <cmath>

namespace
{
    const float HOVER_SCALE = 1.04f;              // Size under the mouse
    const float PRESSED_SCALE = 0.94f;            // Size while pressed
    const float CORNER_SCALE = 0.7f;              // Corners of the yellow image on screen (outline about 5 px)
    const sf::Color PRESSED_TINT{ 215, 215, 215 };
    const sf::Color DISABLED_TINT{ 165, 165, 170 };      // On the steel image: a flat grey
    const sf::Color DISABLED_TEXT{ 190, 190, 190 };

    const float ENTRANCE = 0.36f;                 // The pop-in
    const float BREATH = 0.025f;                  // The main action breathes this much (scale)
    const float SHINE_EVERY = 3.6f, SHINE_SWEEP = 0.55f;   // A light across the main action

    // Seconds since the game started: the pop-ins and the attention look run on it (subclasses skip update())
    float clock_seconds()
    {
        static sf::Clock clock;
        return clock.getElapsedTime().asSeconds();
    }

    float ease_out_back(float t)
    {
        const float c = 1.70158f;
        t -= 1.f;
        return 1.f + (c + 1.f) * t * t * t + c * t * t;
    }
}

void UIButtonElement::set_entrance(float delay)
{
    m_entranceStart = clock_seconds() + std::max(0.f, delay);
}

void UIButtonElement::set_attention(bool hasAttention)
{
    m_hasAttention = hasAttention;
}

float UIButtonElement::entrance_scale() const
{
    if (m_entranceStart < 0.f) return 1.f;
    const float t = (clock_seconds() - m_entranceStart) / ENTRANCE;
    if (t <= 0.f) return 0.f;
    if (t >= 1.f) return 1.f;
    return 0.8f + 0.2f * ease_out_back(t);   // From 80% of its size, a touch past full, back (a light pop)
}

/*
 * A light sweeping across the button now and then (the main action): a slanted white band, clipped to the button
 * (a scissor rectangle on a copy of the view)
 */
void UIButtonElement::render_shine(sf::RenderWindow& window, const sf::FloatRect& rect)
{
    const float offset = m_shape.getPosition().x * 0.0013f;   // Not every main action at once
    const float cycle = std::fmod(clock_seconds() + offset, SHINE_EVERY);
    if (cycle > SHINE_SWEEP) return;
    const float progress = cycle / SHINE_SWEEP;

    // Inside the outline
    const float inset = std::min(rect.size.x, rect.size.y) * 0.12f;
    const sf::FloatRect inner{ rect.position + sf::Vector2f{ inset, inset }, rect.size - sf::Vector2f{ 2.f * inset, 2.f * inset } };
    const sf::View base = window.getView();
    const sf::Vector2i topLeft = window.mapCoordsToPixel(inner.position, base);
    const sf::Vector2i bottomRight = window.mapCoordsToPixel(inner.position + inner.size, base);
    const sf::Vector2f target(window.getSize());
    if (target.x <= 0.f || target.y <= 0.f || bottomRight.x <= topLeft.x || bottomRight.y <= topLeft.y) return;
    // Inside the view's own scissor too (a screen transition clips the screens)
    const sf::FloatRect button{ { static_cast<float>(topLeft.x) / target.x, static_cast<float>(topLeft.y) / target.y },
        { static_cast<float>(bottomRight.x - topLeft.x) / target.x, static_cast<float>(bottomRight.y - topLeft.y) / target.y } };
    const std::optional<sf::FloatRect> visible = button.findIntersection(base.getScissor());
    if (!visible || visible->size.x <= 0.f || visible->size.y <= 0.f) return;
    sf::View clipped = base;
    clipped.setScissor(*visible);
    window.setView(clipped);

    const float slant = rect.size.y * 0.6f;
    const float x = rect.position.x - slant - 50.f + (rect.size.x + slant + 100.f) * progress;
    for (const auto& [width, alpha] : { std::pair{ 46.f, 70 }, std::pair{ 16.f, 90 } })
    {
        sf::ConvexShape band(4);
        band.setPoint(0, { x + slant, rect.position.y });
        band.setPoint(1, { x + slant + width, rect.position.y });
        band.setPoint(2, { x + width, rect.position.y + rect.size.y });
        band.setPoint(3, { x, rect.position.y + rect.size.y });
        band.setFillColor(sf::Color(255, 255, 255, static_cast<std::uint8_t>(alpha)));
        window.draw(band);
    }
    window.setView(base);
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

void UIButtonElement::set_click_sound(const std::string& soundName)
{
    m_clickSound = soundName;
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

void UIButtonElement::set_label(const sf::String& text)
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

void UIButtonElement::set_style(Style style)
{
    m_style = style;
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
    float scale = 1.f;
    if (m_isEnabled)
    {
        if (m_isPressed) scale = PRESSED_SCALE;
        else if (m_isHovered) scale = HOVER_SCALE;
        else if (m_hasAttention) scale = 1.f + BREATH * (0.5f + 0.5f * std::sin(clock_seconds() * 3.2f));   // Breathing
    }
    return scale * entrance_scale();
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
    const float scale = get_visual_scale();
    if (scale < 0.02f) return;   // Not popped in yet

    // Disabled: the steel image greyed (a yellow image tinted grey turns olive, which reads as "not selected")
    sf::Color tint = m_tint;
    if (!m_isEnabled) tint = DISABLED_TINT;
    else if (m_isPressed) tint = tint * PRESSED_TINT;
    const bool isSteel = !m_isEnabled || m_style == Style::Secondary;

    const sf::FloatRect rect = get_visual_rect();
    // A soft shadow under it (pressed: it sinks onto it)
    const float drop = (m_isPressed ? 0.4f : 1.f) * UISkin::SHADOW_DROP * scale;
    UISkin::draw_nine_slice(window, UISkin::BUTTON_STEEL, { rect.position + sf::Vector2f{ 0.f, drop }, rect.size }, CORNER_SCALE, UISkin::SHADOW_TINT);
    UISkin::draw_nine_slice(window, isSteel ? UISkin::BUTTON_STEEL : UISkin::BUTTON_YELLOW, rect, CORNER_SCALE, tint);
    if (m_hasAttention && m_isEnabled && !isSteel && entrance_scale() >= 1.f) render_shine(window, rect);

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
            if (m_firesOnPress) activate();
        }
    }
    else if (const auto* mouseButtonReleased = event.getIf<sf::Event::MouseButtonReleased>())
    {
        // A click counts when the button is released over the button it was pressed on:
        // sliding off before letting go cancels it
        if (mouseButtonReleased->button == sf::Mouse::Button::Left)
        {
            const bool isClick = m_isPressed && !m_firesOnPress
                && get_bounds().contains(window.mapPixelToCoords(mouseButtonReleased->position));
            m_isPressed = false;
            if (isClick) activate();
        }
    }
    else if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>())
    {
        if (m_hotkey && keyPressed->code == *m_hotkey) activate();
    }
}

// A click or the hotkey: the error sound if disabled, else the click sound and the callback
void UIButtonElement::activate()
{
    if (!m_isEnabled)
    {
        play_sound("ui_error");
    }
    else if (m_callback)
    {
        play_sound(m_clickSound);
        m_callback();
    }
}

void UIButtonElement::set_fires_on_press(bool firesOnPress)
{
    m_firesOnPress = firesOnPress;
}

void UIButtonElement::set_hotkey(std::optional<sf::Keyboard::Key> key)
{
    m_hotkey = key;
}

// Return the button's bounding rectangle for collision or interaction checks
sf::FloatRect UIButtonElement::get_bounds() const
{
    return m_shape.getGlobalBounds();
}
