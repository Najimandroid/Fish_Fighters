#pragma once

#include "UIElement.h"
#include <functional>
#include <optional>

/*
 * UIButtonElement represents a clickable button in the UI.
 * Inherits from UIElement and adds:
 *  - A glossy yellow button image (UISkin, 9-slice) that grows under the mouse and shrinks when pressed
 *  - Text label (heavy outlined font)
 *  - Callback function for click events
 *
 * Subclasses with their own look (UIBattleIcon, UICashUp, UIRoundButton) override render()
 * and reuse the hover / press state and get_visual_rect().
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
    void set_label(const sf::String& text);   // Same, with any character (arrows...)
    void set_text_size(unsigned int size);
    void set_text_color(const sf::Color& color);

    // Tints the button image (white keeps the yellow)
    void set_fill_color(const sf::Color& color);

    // Yellow for the screen's main action, steel blue for the others (and choices not selected)
    enum class Style { Primary, Secondary };
    void set_style(Style style);

    // A disabled button is greyed out and ignores clicks (a click on it plays the error sound)
    void set_enabled(bool isEnabled);
    bool is_enabled() const;

    // Sound of a click (sfx.json name, "ui_click" by default; empty: silent, the callback plays its own)
    void set_click_sound(const std::string& soundName);

    // Menu buttons fire when released over the button (sliding off cancels the click);
    // battle buttons (deploy slots, Cash Up) fire as soon as they're pressed
    void set_fires_on_press(bool firesOnPress);

    // A key that works like a click (F for the speed button, 1-0 for the deploy slots...)
    void set_hotkey(std::optional<sf::Keyboard::Key> key);

    // Override UIElement functions
    void update(float deltaTime) override;
    void render(sf::RenderWindow& window) override;
    void handle_event(const sf::Event& event, const sf::RenderWindow& window) override;
    sf::FloatRect get_bounds() const override;

protected:

    // Size factor of the drawing: bigger under the mouse, smaller while pressed
    float get_visual_scale() const;

    // The button rectangle scaled by get_visual_scale() around its center
    sf::FloatRect get_visual_rect() const;

    // Runs a click: the error sound when disabled, else the click sound and the callback
    void activate();

    sf::RectangleShape m_shape;         // Button rectangle (position, size and clicks; not drawn)
    sf::Text m_text;                    // Button label text
    std::function<void()> m_callback;   // Function to execute on click

    sf::Color m_tint = sf::Color::White; // Color multiplied with the button image
    bool m_isHovered = false;            // The mouse is over the button
    bool m_isPressed = false;            // The left button went down on it and isn't released yet
    bool m_isEnabled = true;
    Style m_style = Style::Primary;
    std::string m_clickSound = "ui_click";
    bool m_firesOnPress = false;
    std::optional<sf::Keyboard::Key> m_hotkey;
};
