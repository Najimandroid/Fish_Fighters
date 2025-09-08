#include "UITextureElement.h"
#include <iostream>

UITextureElement::UITextureElement(sf::Vector2f size, sf::Vector2f position, const std::string& texturePath):
    UIElement(size, position),
    m_sprite(m_texture)
{
    if (!m_texture.loadFromFile(texturePath))
    {
        std::cerr << "Failed to load texture: " << texturePath << "\n";
    }

    m_sprite.setTextureRect(sf::IntRect(
        { 0, 0 },
        { static_cast<int>(m_texture.getSize().x), static_cast<int>(m_texture.getSize().y)
    }));

    m_sprite.setTexture(m_texture);
    m_sprite.setPosition(m_position);

    apply_scale();
}

void UITextureElement::render(sf::RenderWindow& window)
{
    if (m_isVisible)
    {
        window.draw(m_sprite);
    }
}

void UITextureElement::set_texture(const std::string& texturePath)
{
    if (!m_texture.loadFromFile(texturePath))
    {
        std::cerr << "Failed to load texture: " << texturePath << "\n";
        return;
    }

    m_sprite.setTexture(m_texture);
    apply_scale();
}

void UITextureElement::set_size(sf::Vector2f newSize)
{
    m_size = newSize;
    apply_scale();
}

void UITextureElement::apply_scale()
{
    sf::Vector2u texSize = m_texture.getSize();
    if (texSize.x == 0 || texSize.y == 0) return;

    float scaleX = m_size.x / static_cast<float>(texSize.x);
    float scaleY = m_size.y / static_cast<float>(texSize.y);

    m_sprite.setScale({ scaleX, scaleY });
    m_sprite.setPosition(m_position);
}
