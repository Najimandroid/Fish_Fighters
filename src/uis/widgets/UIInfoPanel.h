#pragma once

#include "UIElement.h"
#include "UIPopupFrame.h"

#include <string>
#include <vector>

/*
 * UIInfoPanel
 * -----------
 * The help window an "i" opens (UIInfoButton): a title and an explanation of a screen or a feature, on the dark
 * panel over the dimmed screen. It closes with its X, a tap outside it, Escape or Enter.
 * Add it after the screen's other elements (a window: on top, the only one getting input while open).
 *
 * The text is laid out with a little markup:
 *   # Title          a section title (light blue, bigger), with room above it
 *   - item           a bullet point (wrapped lines keep its indent)
 *   *words*          important words, highlighted in yellow
 *   {name|words}     words in a colour: yellow, green, red, blue, cyan, pink, purple, orange, gray, white, rare,
 *                    superrare, and the types' and abilities' own (crustacean, shelled, jelly, strong, weaken...)
 *   (empty line)     a paragraph gap
 * Too long for the screen, it scrolls: a finger drags it, the mouse wheel rolls it, a bar on the right shows where.
 *
 * Example usage:
 *   auto info = std::make_shared<UIInfoPanel>("School Rank", "# What it is\nEvery fish's *level*, added up...");
 *   infoButton->set_callback([weak = std::weak_ptr(info)]() { if (auto panel = weak.lock()) panel->open(); });
 */
class UIInfoPanel : public UIElement
{
public:
    UIInfoPanel(const std::string& title, const std::string& text);

    void open();
    void close();

    void update(float deltaTime) override;
    void render(sf::RenderWindow& window) override;
    void handle_event(const sf::Event& event, const sf::RenderWindow& window) override;
    sf::FloatRect get_bounds() const override;     // The whole screen while open
    bool is_blocking_input() const override;

private:
    // The text laid out from the markup: one run per part of a line in one style, the bullets' centers, its height
    // (content coordinates: from the text area's top left)
    struct Layout
    {
        std::vector<sf::Text> runs;
        std::vector<sf::Vector2f> bullets;
        float height = 0.f;
    };
    static Layout lay_out(const std::string& text);
    static sf::FloatRect box_for(float contentHeight);   // The panel fits its text (up to the screen), centered
    sf::FloatRect content_rect() const;                  // Where the text shows (it scrolls inside)

    Layout m_layout;                    // First: the box and the frame are made from it
    sf::FloatRect m_box;
    UIPopupFrame m_frame;               // The popup look: the title on its header, its X closes it
    float m_scroll = 0.f;               // How far the text is scrolled up
    bool m_isPressedWhileOpen = false;  // A tap outside closes it only if it began while it was open
    bool m_isDragging = false;          // A finger dragging the text
    float m_dragStartY = 0.f, m_dragStartScroll = 0.f;
    bool m_hasDragged = false;
    bool m_isPressOutside = false;      // The press was outside the panel (with no slide, its release closes it)
};
