#pragma once

#include <SFML/Graphics.hpp>

#include <functional>
#include <string>

/*
 * Base class for all UI elements in the game.
 * Handles common properties like position, size, visibility,
 * and provides an interface for update, render, and event handling.
 */

class UIElement
{
public:

    UIElement() = default;

    // Construct a UI element with a specific size and position
    UIElement(sf::Vector2f size, sf::Vector2f position);

    virtual ~UIElement() = default;

    // Update element logic each frame
    virtual void update(float deltaTime) = 0;

    // Render element to the window
    virtual void render(sf::RenderWindow& window) = 0;

    // Handle input events (mouse, keyboard, etc.)
    virtual void handle_event(const sf::Event& event, const sf::RenderWindow& window) {}

    // Setters
    virtual void set_position(sf::Vector2f newPosition);
    virtual void set_size(sf::Vector2f newSize);
    virtual void set_visible(bool isVisible);

    // Getters
    sf::Vector2f get_position() const;
    sf::Vector2f get_size() const;

    // Returns element bounding rectangle
    virtual sf::FloatRect get_bounds() const;
    bool is_visible() const;

    // While true (a modal window like the pause menu), this element gets the input events alone
    virtual bool is_blocking_input() const { return false; }

    // Sounds of the widgets (clicks, selections...): UIManager plugs the AudioManager in, so the widgets
    // don't depend on it. Without a player, play_sound does nothing.
    static void set_sound_player(std::function<void(const std::string&)> player) { s_soundPlayer = std::move(player); }
    static void play_sound(const std::string& name) { if (s_soundPlayer && !name.empty()) s_soundPlayer(name); }

public:

    bool isWorldSpaceUi = false; // Determines if element is rendered in world space or screen space

protected:

    sf::Vector2f m_position = { 0.0f, 0.0f }; // Top-left position of the element
    sf::Vector2f m_size = { 0.0f, 0.0f };     // Width and height of the element

    bool m_isVisible = true; // Whether the element should be drawn

private:

    static inline std::function<void(const std::string&)> s_soundPlayer;
};