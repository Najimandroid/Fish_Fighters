#pragma once

#include <SFML/Graphics.hpp>
#include <string>

/*
 * UISkin
 * ------
 * The look shared by every UI widget: the chunky style of The Battle Cats (dark outlines, glossy yellow
 * buttons, a heavy rounded font, outlined numbers) inside a submarine (riveted blue steel, scratches, stains).
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
    inline const sf::Color OUTLINE{ 16, 24, 34 };         // Outline of every texture and text (dark navy)
    inline const sf::Color TEXT{ 255, 255, 255 };         // Labels
    inline const sf::Color NUMBER{ 255, 222, 48 };        // Cash and costs
    inline const sf::Color NUMBER_DISABLED{ 150, 136, 84 };// Costs the player can't pay yet
    inline const sf::Color COOLDOWN{ 72, 224, 255 };      // Deploy cooldown bar
    inline const sf::Color SHADE{ 0, 0, 0, 140 };         // Darkens what can't be used
    inline const sf::Color EMPTY_SLOT_TINT{ 150, 170, 180 };// A slot with no unit in it
    inline const sf::Color TITLE{ 226, 240, 255 };        // Screen titles on the steel bar (pale blue)
    inline const sf::Color EXTRA{ 205, 150, 255 };        // The Clam Room's extra levels ("LEVEL 7 +3"), its purple

    // ----- Fonts -----
    const sf::Font& heavy_font();    // M PLUS Rounded 1c Black: titles, buttons, numbers
    const sf::Font& regular_font();  // M PLUS Rounded 1c Medium: longer texts

    // ----- Textures -----
    // A texture of assets/images/textures/ui, by file name without the extension ("button_yellow")
    const sf::Texture& texture(const std::string& name);

    // Any image of the game by its path ("assets/images/textures/icons/types.png"), loaded once
    const sf::Texture& texture_file(const std::string& path);

    // Cell of an icon sheet made of square cells, read left to right then top to bottom
    sf::IntRect icon_cell(const sf::Texture& sheet, int index, int cellSize);

    // A fish's swimming frames (0 .. knockback frame - 1 of its sheet), cropped to the box its pixels use in all of
    // them: the whole fish, still while it swims (icons are close-ups cut at the edges, and a sheet's frame leaves
    // room for the attack effects). Worked out once per sheet. Frame k: { k * frameWidth + box.x, box.y, box.size }
    struct SwimFrames { sf::IntRect box; int frameWidth = 0; int count = 1; };
    const SwimFrames& swim_frames(const std::string& sheetPath, int frameCount, int knockbackFrameIndex);

    // ----- 9-slice -----
    // An image cut in 3 x 3: the corners keep their shape, the edges and the middle stretch
    struct NineSlice
    {
        const char* texture;  // Name in assets/images/textures/ui
        float margin;         // Size of the corners in the image (px)
        bool repeatsX = false;// The middle columns repeat instead of stretching (rivets keep their shape)
        bool hasGrime = false;// Scratches and stains (grime.png) laid over it, pinned to the screen so they never stretch
    };

    // Submarine look: riveted steel panels and bars, yellow buttons, light blue screens (slots)
    inline constexpr NineSlice BUTTON_YELLOW{ "button_yellow", 40.f };   // The screen's main action
    inline constexpr NineSlice BUTTON_STEEL{ "button_steel", 40.f };     // Secondary actions, choices not selected
    inline constexpr NineSlice PANEL_DARK{ "panel_dark", 40.f, false, true };
    inline constexpr NineSlice PANEL_LIGHT{ "panel_light", 40.f, false, true };
    inline constexpr NineSlice BAR_METAL{ "bar_metal", 48.f, true, true };
    inline constexpr NineSlice SLOT{ "slot", 24.f };
    inline constexpr NineSlice SLOT_FRAME{ "slot_frame", 24.f };  // Outline of SLOT alone, drawn over its content
    inline constexpr float SLOT_OUTLINE = 6.f;                    // Outline width of SLOT in its image (px)

    // Draws the image over rect. cornerScale sizes the corners on screen (they never exceed half the rect)
    void draw_nine_slice(sf::RenderTarget& target, const NineSlice& slice, const sf::FloatRect& rect,
                         float cornerScale = 1.f, sf::Color tint = sf::Color::White);

    // Scratches and stains over rect (inset px from its edges), pinned to the screen (draw_nine_slice uses it)
    void draw_grime(sf::RenderTarget& target, const sf::FloatRect& rect, float inset, sf::Color tint = sf::Color::White);

    // ----- Text -----
    // Heavy font (or font), fill color and a dark outline that grows with the size (thinner for the regular font)
    void style_text(sf::Text& text, unsigned int size, sf::Color fill = TEXT, const sf::Font& font = heavy_font());

    // Body text: the regular font (descriptions, explanations, tips)
    inline void style_body_text(sf::Text& text, unsigned int size, sf::Color fill = TEXT) { style_text(text, size, fill, regular_font()); }

    // Changes the fill color and keeps the outline as transparent as the fill
    void set_text_color(sf::Text& text, sf::Color fill);

    // Puts the origin of the text on one of its points: (0.5, 0.5) is the center, (1, 0) the top right...
    // Vertically, the text is the font's lines (top of the capitals to the last baseline): descenders hang below
    void set_text_anchor(sf::Text& text, sf::Vector2f anchor);

    // Breaks the text into lines no wider than maxWidth (in font at that size); existing line breaks are kept
    std::string wrap_text(const std::string& text, unsigned int size, float maxWidth, const sf::Font& font = heavy_font());

    // ----- Images -----
    // The texture cropped to the shape of area and stretched to fill it (unit icons in slots and cards)
    void draw_cropped(sf::RenderTarget& target, const sf::Texture& texture, const sf::FloatRect& area, sf::Color tint = sf::Color::White);
    void draw_cropped(sf::RenderTarget& target, const sf::Texture& texture, const sf::IntRect& source, const sf::FloatRect& area, sf::Color tint = sf::Color::White);

    // The texture fitted inside area (keeps its ratio), centered
    void draw_fitted(sf::RenderTarget& target, const sf::Texture& texture, const sf::FloatRect& area, sf::Color tint = sf::Color::White);
    void draw_fitted(sf::RenderTarget& target, const sf::Texture& texture, const sf::IntRect& source, const sf::FloatRect& area, sf::Color tint = sf::Color::White);

    // Inside of a SLOT drawn over rect: the part the outline doesn't cover
    sf::FloatRect slot_inner(const sf::FloatRect& rect, float cornerScale);

    // ----- Screen shape -----
    // The part of the UI coordinates the window shows: 1920 x 1080 on a 16:9 screen, wider on a phone (x from below 0
    // to past 1920), taller on a 4:3 tablet. The layout stays in 1920 x 1080 (clear of a phone's camera cutout);
    // backgrounds, bars, dims and transitions fill this. Game sets it with the views
    sf::FloatRect screen_rect();
    void set_screen_rect(const sf::FloatRect& rect);

    // A color over the whole screen (veils, flashes, fades)
    void fill_screen(sf::RenderTarget& target, sf::Color color);

    // The menus' frame on a screen wider than 16:9 (a phone): steel hull pillars fill the extra width on both sides,
    // so the menus stay a compact 16:9 instead of stretching (the battle has none: its field fills the screen)
    void draw_side_frame(sf::RenderTarget& target);

    // A rounded rectangle shape (bubbles, pills, vector icons)
    sf::ConvexShape rounded_rect(const sf::FloatRect& rect, float radius);

    // ----- Depth -----
    // The soft shadow under a raised element (buttons, tiles): its own shape, dark, a little lower
    inline constexpr float SHADOW_DROP = 6.f;
    inline const sf::Color SHADOW_TINT{ 0, 0, 0, 75 };

    // ----- Transitions -----
    // A band of deep water across the screen from top to bottom (UI coordinates), solid (nothing shows through it):
    // wavy edges with a pale foam line, darker at the bottom, a few bubbles rising inside. The screen transitions
    // (a new menu screen, the battle start) sweep it over the screen
    inline constexpr float WATER_WAVE = 18.f;     // How far its edges wave above and below top / bottom
    void draw_water_band(sf::RenderTarget& target, float top, float bottom, float time);
}
