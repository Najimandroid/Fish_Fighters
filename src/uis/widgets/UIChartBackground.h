#pragma once

#include "UIElement.h"

/*
 * UIChartBackground
 * -----------------
 * The Encyclopedia's backdrop: a research chart (deep teal, a fine grid, a stronger one every few squares, darker at
 * the edges), so the field guide has its own place instead of the hull wall of the other menus. Nothing to click.
 *
 * Example usage:
 *   add_ui_element(std::make_shared<UIChartBackground>());   // First
 */
class UIChartBackground : public UIElement
{
public:
    UIChartBackground();

    void update(float deltaTime) override {}
    void render(sf::RenderWindow& window) override;
    sf::FloatRect get_bounds() const override { return {}; }
};
