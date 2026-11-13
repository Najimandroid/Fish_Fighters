#pragma once

#include "UIElement.h"

#include <random>
#include <vector>

/*
 * UIAmbience
 * ----------
 * Life over a menu's still background: shafts of light from the water above sway slowly across the hull, specks
 * drift in them, and a soft vignette pulls the eye to the middle of the screen. Nothing to click.
 *
 * Example usage:
 *   add_ui_element(background);
 *   add_ui_element(std::make_shared<UIAmbience>());   // Right over the background, under everything else
 */
class UIAmbience : public UIElement
{
public:
    // hasLight: the swaying light shafts and specks (off: the vignette alone)
    explicit UIAmbience(bool hasLight = true);

    void update(float deltaTime) override;
    void render(sf::RenderWindow& window) override;
    sf::FloatRect get_bounds() const override { return {}; }   // Not part of the clickable UI

private:
    struct Speck { sf::Vector2f position; float radius = 2.f; float speed = 10.f; float phase = 0.f; };

    bool m_hasLight = true;
    std::vector<Speck> m_specks;
    std::mt19937 m_random{ std::random_device{}() };
    float m_time = 0.f;
};
