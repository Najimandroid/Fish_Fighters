#pragma once

#include "UIElement.h"

#include <functional>
#include <vector>

/*
 * UITitleScreen
 * -------------
 * The title screen's animation: the "FISH FIGHTERS" title bouncing to the beat of the title theme, bubbles rising
 * over the fish tank, a blinking "press any key" under the START button (UIManager adds the button).
 * Any key starts the game, like the button.
 *
 * Example usage:
 *   auto title = std::make_shared<UITitleScreen>(168.f);
 *   title->set_start_callback([this]() { start(); });
 */
class UITitleScreen : public UIElement
{
public:
    // beatsPerMinute: the title theme's tempo, for the title's bounce
    explicit UITitleScreen(float beatsPerMinute);

    void set_start_callback(std::function<void()> callback);

    void update(float deltaTime) override;
    void render(sf::RenderWindow& window) override;
    void handle_event(const sf::Event& event, const sf::RenderWindow& window) override;

    // Nothing to click on: the START button under it gets the clicks
    sf::FloatRect get_bounds() const override { return {}; }

private:
    struct Bubble
    {
        sf::Vector2f position;
        float radius = 6.f;
        float speed = 60.f;      // Pixels per second, upward
        float phase = 0.f;       // Side to side wobble
    };

    void draw_title_line(sf::RenderWindow& window, const char* text, float centerY, unsigned int size, float bounce);

    std::function<void()> m_startCallback;
    std::vector<Bubble> m_bubbles;
    float m_time = 0.f;
    float m_beatSeconds = 0.5f;
};
