#pragma once

#include "UIElement.h"
#include "../UISkin.h"

/*
 * UIPanel
 * -------
 * A box drawn from a 9-slice image of the skin: dark or light panel, driftwood bar, slot...
 * Other widgets draw it behind their content (the pause menu, later the menu headers).
 *
 * Example usage:
 *   UIPanel panel({ 640.f, 440.f }, { 640.f, 320.f }, UISkin::PANEL_DARK);
 */

class UIPanel : public UIElement
{
public:

    // cornerScale sizes the corners of the image on screen
    UIPanel(sf::Vector2f size, sf::Vector2f position,
            const UISkin::NineSlice& look = UISkin::PANEL_DARK, float cornerScale = 1.f);

    void set_tint(sf::Color tint);

    void update(float deltaTime) override {}
    void render(sf::RenderWindow& window) override;

private:

    UISkin::NineSlice m_look;
    float m_cornerScale;
    sf::Color m_tint = sf::Color::White;
};
