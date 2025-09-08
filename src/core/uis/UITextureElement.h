#pragma once

#include "UIElement.h"

class UITextureElement : public UIElement
{
public:

    UITextureElement(sf::Vector2f size, sf::Vector2f position, const std::string& texturePath);

    void update(float deltaTime) override {}
    void render(sf::RenderWindow& window) override;

    void set_texture(const std::string& texturePath);
    void set_size(sf::Vector2f newSize) override;

protected:

    sf::Texture m_texture;
    sf::Sprite m_sprite;

    void apply_scale();
};