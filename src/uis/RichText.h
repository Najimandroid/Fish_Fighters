#pragma once

#include <SFML/Graphics.hpp>

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

/*
 * RichText
 * --------
 * A text where the currencies are drawn as their icons instead of their names: [shell], [pearl], [shiny] (a Shiny
 * Scale), [scale] (a Fish Scale) and [energy] in the string become the counters' icons, a little bigger than the
 * letters and centered on them ("20 [pearl]", "Feed the clam a [shiny]"), so a reader sees the currency at a glance.
 * Wrapped at spaces to a width, its lines aligned left, centered or right, the block placed by an anchor like
 * UISkin::set_text_anchor (from the capitals' top to the last baseline), so it sits where the plain sf::Text was.
 * The words look like a template sf::Text (UISkin::style_text: font, size, colors, outline). It can show only its
 * first characters (an icon counts as one), for text typed out.
 *
 * Example usage:
 *   sf::Text style(UISkin::regular_font());
 *   UISkin::style_body_text(style, 30);
 *   RichText text(style);
 *   text.set_string("A refill costs 20 [pearl].");
 *   text.set_max_width(720.f);
 *   text.set_alignment(0.5f);
 *   text.set_position({ 960.f, 500.f }, { 0.5f, 0.5f });
 *   text.draw(window);
 */
class RichText
{
public:
    RichText();                                   // The heavy font, 30 px, white (set_style to change)
    explicit RichText(const sf::Text& style);

    // The look of the words (font, size, fill and outline colors and thickness, letter and line spacing)
    void set_style(const sf::Text& style);
    // The words' color, the outline keeping the style's but with the fill's alpha (UISkin::set_text_color);
    // the icons take its alpha too (a fade)
    void set_fill_color(sf::Color fill);

    // UTF-8, with \n for new lines and the [icon] tokens (an unknown [word] stays as written)
    void set_string(const std::string& text);
    const std::string& get_string() const { return m_string; }

    void set_max_width(float width);              // Wrapped at spaces past it (0: only at \n)
    void set_alignment(float alignment);          // Each line in the block: 0 left, 0.5 centered, 1 right
    void set_position(sf::Vector2f position, sf::Vector2f anchor = { 0.f, 0.f });   // anchor: the block's point there

    // Only the first count characters show (an icon is one, a space between words is one); npos: all of them
    void set_revealed(std::size_t count);
    std::size_t get_length() const { return m_length; }

    sf::FloatRect get_bounds() const;             // The block on screen (capitals' top to the last baseline)
    sf::Vector2f get_size() const { return m_size; }
    float get_line_height() const;

    void draw(sf::RenderTarget& target, sf::RenderStates states = sf::RenderStates::Default) const;

    // Lays out and draws a string at once, looking like style: for the widgets that draw a reused sf::Text with a
    // string that changes (a few short lines a frame)
    static void draw_text(sf::RenderTarget& target, const sf::Text& style, const std::string& string, sf::Vector2f position,
        sf::Vector2f anchor = { 0.f, 0.f }, float alignment = 0.f, float maxWidth = 0.f,
        sf::RenderStates states = sf::RenderStates::Default);

    // "pearl" -> "icon_pearl" (UISkin::texture): nullptr if token isn't a currency
    static const char* icon_texture(std::string_view token);
    // How the icons sit in a line of characterSize letters: their size, and their center under the line's top
    static float icon_size(unsigned int characterSize);
    static float icon_center_y(const sf::Font& font, unsigned int characterSize);
    // Draws a currency icon fitted in a square centered on center (alpha: the tint's)
    static void draw_icon(sf::RenderTarget& target, const char* texture, sf::Vector2f center, float size,
        sf::RenderStates states = sf::RenderStates::Default, std::uint8_t alpha = 255);

    static constexpr float ICON_SCALE = 1.3f;     // An icon's size, in character sizes
    static constexpr float ICON_PADDING = 0.06f;  // Room on each side of an icon, in character sizes

private:
    void lay_out();

    struct Run { sf::Text text; std::size_t firstChar = 0; };          // Words in one piece of a line
    struct Icon { const char* texture = nullptr; sf::Vector2f center; std::size_t index = 0; };

    sf::Text m_style;
    std::string m_string;
    float m_maxWidth = 0.f;
    float m_alignment = 0.f;
    sf::Vector2f m_position;
    sf::Vector2f m_anchor;
    std::size_t m_revealed = std::string::npos;

    std::vector<Run> m_runs;
    std::vector<Icon> m_icons;
    std::size_t m_length = 0;
    std::size_t m_lineCount = 1;
    sf::Vector2f m_size;                          // The widest line, the lines' height
    float m_top = 0.f, m_bottom = 0.f;            // The block's top (capitals) and bottom (last baseline) in it
};
