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

    // Drawn over the side frame (the pillars on a phone's extra width): the modal windows, the battle start's water
    virtual bool is_over_frame() const { return is_blocking_input(); }

    // Darkens what the screen shows outside the 16:9 layout (a phone's sides) as it darkens the layout itself: the
    // modal windows (not the results' see-through band nor the wipe: they span the whole screen themselves)
    virtual bool dims_letterbox() const { return is_blocking_input(); }

    // Part of a menu's frame (the title bar, the bottom bar, the counters, the back button, the bar's shortcuts): it
    // stays still while one menu screen changes into another under it, like The Battle Cats' bars (UIManager)
    void set_frame(bool isFrame) { m_isFrame = isFrame; }
    bool is_frame() const { return m_isFrame; }

    // Its depth on the screen: UIManager keeps the elements sorted by it (the ones added later on top among equals), so
    // the frame stays over the content (the back button over a list) and the windows over both, whatever the order
    // they're added in. The frame gets Z_FRAME when added; windows and overlays set theirs
    static constexpr int Z_CONTENT = 0;    // The screen itself
    static constexpr int Z_FRAME = 10;     // The bars, their counters and buttons, the back button
    static constexpr int Z_FLYING = 20;    // What flies over the bars (scales into a counter)
    static constexpr int Z_WINDOW = 100;   // Popups and dialogs (pause, options, info, confirmations, results)
    static constexpr int Z_OVERLAY = 200;  // Over everything (toasts, an ad, the transitions' water and wipe)
    void set_z(int z) { m_z = z; }
    int get_z() const { return m_z; }

    // The battle HUD's corners: on a screen wider than 16:9 (a phone) the element goes to the screen's far left or
    // right instead of the layout's edge. UIManager draws it and passes it the input shifted (screen_shift)
    enum class ScreenAnchor { NONE, LEFT, RIGHT };
    void set_screen_anchor(ScreenAnchor anchor) { m_screenAnchor = anchor; }
    float screen_shift() const { return shift_for(m_screenAnchor); }
    static float shift_for(ScreenAnchor anchor);   // How far right of its layout place (negative: left)
    sf::FloatRect get_screen_bounds() const;       // get_bounds() where it is on screen

    // Sounds of the widgets (clicks, selections...): UIManager plugs the AudioManager in, so the widgets
    // don't depend on it. Without a player, play_sound does nothing.
    // pitch: a factor (1 = as is); pan: -1 left to 1 right
    static void set_sound_player(std::function<void(const std::string&, float, float)> player) { s_soundPlayer = std::move(player); }
    static void play_sound(const std::string& name, float pitch = 1.f, float pan = 0.f) { if (s_soundPlayer && !name.empty()) s_soundPlayer(name, pitch, pan); }

public:

    bool isWorldSpaceUi = false; // Determines if element is rendered in world space or screen space

protected:

    sf::Vector2f m_position = { 0.0f, 0.0f }; // Top-left position of the element
    sf::Vector2f m_size = { 0.0f, 0.0f };     // Width and height of the element

    bool m_isVisible = true; // Whether the element should be drawn
    bool m_isFrame = false;  // Stays still while the menu screens change (set_frame)
    int m_z = Z_CONTENT;
    ScreenAnchor m_screenAnchor = ScreenAnchor::NONE;

private:

    static inline std::function<void(const std::string&, float, float)> s_soundPlayer;
};