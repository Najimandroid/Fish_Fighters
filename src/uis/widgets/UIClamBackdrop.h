#pragma once

#include "UIElement.h"

/*
 * UIClamBackdrop
 * --------------
 * The Clam Room's background: kept simple so the clam is the show (like a capsule machine's plain stage). A deep
 * violet-blue gradient, one beam of light from above onto the clam, a soft round stage of pale sand under it, and a
 * few motes drifting in the beam. Nothing to click.
 *
 * Example usage:
 *   add_ui_element(std::make_shared<UIClamBackdrop>());   // First: under the clam
 */
class UIClamBackdrop : public UIElement
{
public:
    UIClamBackdrop();

    void update(float deltaTime) override;
    void render(sf::RenderWindow& window) override;
    sf::FloatRect get_bounds() const override { return {}; }

private:
    float m_time = 0.f;
};
