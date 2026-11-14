#pragma once

#include "UIButtonElement.h"

#include <string>

/*
 * UIIconTile
 * ----------
 * A square menu button that shows what it opens with a picture, its name small along its bottom (like the Cat Base's
 * Menu / Storage / Capsule icons): no background, the picture stands on its own with its silhouette as a crisp shadow, and its
 * whole rectangle is the button (an easy target on a phone). For secondary destinations; the screen's main
 * actions stay big text buttons. The picture is an image (the clam) or one of a few drawn icons.
 *
 * Example usage:
 *   auto tile = std::make_shared<UIIconTile>(sf::Vector2f{ 170.f, 170.f }, position, "GUIDE", UIIconTile::Icon::BOOK);
 */
class UIIconTile : public UIButtonElement
{
public:
    enum class Icon { TEXTURE, BOOK, GEAR, POWER };

    // texturePath: the picture for Icon::TEXTURE
    UIIconTile(sf::Vector2f size, sf::Vector2f position, const std::string& label, Icon icon, const std::string& texturePath = "");

    void update(float deltaTime) override;
    void render(sf::RenderWindow& window) override;

private:
    // silhouette: draw every shape in that one color (the shadow)
    void draw_icon(sf::RenderWindow& window, const sf::FloatRect& area, float scale, std::uint8_t alpha, const sf::Color* silhouette = nullptr) const;

    Icon m_icon;
    std::string m_texturePath;
    sf::IntRect m_source;   // The image's visible part (its transparent margins cropped)
};
