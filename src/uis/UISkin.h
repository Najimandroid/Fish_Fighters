#pragma once

#include <SFML/Graphics.hpp>
#include <string>

/*
 * UISkin
 * ------
 * The look shared by every UI widget, in the style of The Battle Cats:
 * chunky dark outlines, glossy yellow buttons, a heavy rounded font, outlined numbers.
 *
 * Fonts and the textures of assets/images/textures/ui are loaded once and shared by all widgets
 * (a widget never loads its own copy).
 *
 * Example usage:
 *   UISkin::style_text(m_text, 40, UISkin::NUMBER);
 *   UISkin::draw_nine_slice(window, UISkin::PANEL_DARK, { {100.f, 100.f}, {600.f, 400.f} });
 */

namespace UISkin
{
    // ----- Colors -----
    inline const sf::Color OUTLINE{ 33, 22, 14 };         // Outline of every texture and text (dark brown)
    inline const sf::Color TEXT{ 255, 255, 255 };         // Labels
    inline const sf::Color NUMBER{ 255, 222, 48 };        // Cash and costs
    inline const sf::Color NUMBER_DISABLED{ 150, 136, 84 };// Costs the player can't pay yet
    inline const sf::Color COOLDOWN{ 72, 224, 255 };      // Deploy cooldown bar
    inline const sf::Color SHADE{ 0, 0, 0, 140 };         // Darkens what can't be used

    // ----- Fonts -----
    const sf::Font& heavy_font();    // M PLUS Rounded 1c Black: titles, buttons, numbers
    const sf::Font& regular_font();  // M PLUS Rounded 1c Medium: longer texts

    // ----- Textures -----
    // A texture of assets/images/textures/ui, by file name without the extension ("button_yellow")
    const sf::Texture& texture(const std::string& name);

    // ----- 9-slice -----
    // An image cut in 3 x 3: the corners keep their shape, the edges and the middle stretch
    struct NineSlice
    {
        const char* texture;  // Name in assets/images/textures/ui
        float margin;         // Size of the corners in the image (px)
    };

    inline constexpr NineSlice BUTTON_YELLOW{ "button_yellow", 40.f };
    inline constexpr NineSlice PANEL_DARK{ "panel_dark", 30.f };
    inline constexpr NineSlice PANEL_LIGHT{ "panel_light", 30.f };
    inline constexpr NineSlice BAR_WOOD{ "bar_wood", 24.f };
    inline constexpr NineSlice SLOT{ "slot", 24.f };
    inline constexpr NineSlice SLOT_FRAME{ "slot_frame", 24.f };  // Outline of SLOT alone, drawn over its content

    // Draws the image over rect. cornerScale sizes the corners on screen (they never exceed half the rect)
    void draw_nine_slice(sf::RenderTarget& target, const NineSlice& slice, const sf::FloatRect& rect,
                         float cornerScale = 1.f, sf::Color tint = sf::Color::White);

    // ----- Text -----
    // Heavy font, fill color and a dark outline that grows with the size
    void style_text(sf::Text& text, unsigned int size, sf::Color fill = TEXT);

    // Changes the fill color and keeps the outline as transparent as the fill
    void set_text_color(sf::Text& text, sf::Color fill);

    // Puts the origin of the text on one of its points: (0.5, 0.5) is the center, (1, 0) the top right...
    void set_text_anchor(sf::Text& text, sf::Vector2f anchor);
}
