#pragma once

#include "UIElement.h"

/*
 * UITextureElement
 * ----------------
 * A UI element that displays a textured image (sprite) on the screen.
 * Supports scaling to fit a given size while maintaining aspect ratio.
 * Can be positioned in screen-space or world-space.
 */

class UITextureElement : public UIElement
{
public:

    // Constructor: creates a texture element with given size, position, and texture path
    UITextureElement(sf::Vector2f size, sf::Vector2f position, const std::string& texturePath);

    // Update logic called every frame (no-op for static textures)
    void update(float deltaTime) override {}

    // Render the texture element to the given window
    void render(sf::RenderWindow& window) override;

    // Change the texture of this element and rescale accordingly
    void set_texture(const std::string& texturePath);

    // Set the size of the element and rescale the sprite
    void set_size(sf::Vector2f newSize) override;

    // Get the bounding rectangle of the sprite (used for interaction detection)
    sf::FloatRect get_bounds() const override;

protected:

    sf::Texture m_texture;
    sf::Sprite m_sprite;     // Sprite used to render the texture

protected:

    // Apply scaling to the sprite based on m_size while preserving aspect ratio
    void apply_scale();
};
