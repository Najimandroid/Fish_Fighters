#pragma once

#include "UIRoundButton.h"

#include <functional>
#include <string>

/*
 * UIPopupFrame
 * ------------
 * The look of the game's popups (Options, the battle's pause, the "i" windows), after The Battle Cats' Settings: a
 * light header band with the title in big outlined letters and the Fish's face faded on its left, a dark body, a light
 * footer band holding the popup's one big action when it has one, a round yellow X on the top right corner (closes it).
 * Not an element itself: the popup owns one, renders it first (after its dim), then its own content on top.
 *
 * Example usage:
 *   m_frame(sf::FloatRect{ { 460.f, 200.f }, { 1000.f, 700.f } }, "Options");
 *   m_frame.set_on_close([this]() { close(); });
 */
class UIPopupFrame
{
public:
    static constexpr float HEADER = 104.f;

    // footerHeight: 0 for none
    UIPopupFrame(const sf::FloatRect& rect, const std::string& title, float footerHeight = 0.f);

    void set_title(const std::string& title);
    void set_on_close(std::function<void()> onClose);   // The X (with the "back" sound)
    void set_close_sound(const std::string& sound);      // "": the callback plays its own

    sf::FloatRect get_rect() const { return m_rect; }
    sf::FloatRect get_body() const;     // Between the header and the footer
    sf::FloatRect get_footer() const;   // Empty without one
    sf::FloatRect get_close_bounds() const { return m_close.get_bounds(); }

    void update(float deltaTime);
    void render(sf::RenderWindow& window);
    void handle_event(const sf::Event& event, const sf::RenderWindow& window);

private:
    sf::FloatRect m_rect;
    float m_footerHeight;
    sf::Text m_title;
    UIRoundButton m_close;
};
