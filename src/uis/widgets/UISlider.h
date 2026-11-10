#pragma once

#include "UIElement.h"

#include <functional>

/*
 * UISlider
 * --------
 * A horizontal slider from 0 to 1 (the Options volumes): a dark track with a yellow fill and a round
 * yellow knob, outlined in navy like the rest of the UI, and its value in % on the right.
 * Click anywhere on the track to jump there, drag to adjust; the arrow keys move it while hovered.
 */
class UISlider : public UIElement
{
public:
    // position: top-left of the track; width: track length
    UISlider(sf::Vector2f position, float width, float value);

    // Called with the new value (0 to 1) while it changes
    void set_change_callback(std::function<void(float)> callback);
    // Called once when the player lets go (a preview sound, saving...)
    void set_release_callback(std::function<void(float)> callback);

    float get_value() const;
    void set_value(float value);

    void update(float deltaTime) override {}
    void render(sf::RenderWindow& window) override;
    void handle_event(const sf::Event& event, const sf::RenderWindow& window) override;

    // The track and the knob (what can be clicked)
    sf::FloatRect get_bounds() const override;

private:
    void set_from_x(float x);

    float m_value = 1.f;
    bool m_isDragging = false;
    bool m_isHovered = false;
    sf::Text m_valueText;
    std::function<void(float)> m_onChange;
    std::function<void(float)> m_onRelease;
};
