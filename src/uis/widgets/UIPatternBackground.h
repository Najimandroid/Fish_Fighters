#pragma once

#include "UIElement.h"

/*
 * UIPatternBackground
 * -------------------
 * Fills the screen with a small image repeated in every direction (the wave pattern of the menus),
 * like the patterned wallpapers behind The Battle Cats' menus.
 *
 * Example usage:
 *   auto background = std::make_shared<UIPatternBackground>("pattern_waves");
 */

class UIPatternBackground : public UIElement
{
public:

    // patternName: an image of assets/images/textures/ui that tiles seamlessly
    UIPatternBackground(const std::string& patternName, sf::Color tint = sf::Color::White);

    void update(float deltaTime) override {}
    void render(sf::RenderWindow& window) override;

    // Not part of the clickable UI: the battlefield / menus behind stay usable
    sf::FloatRect get_bounds() const override;

private:

    sf::Texture m_texture;  // Own copy: it has to be repeated
    sf::Sprite m_sprite;
};
