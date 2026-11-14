#include "UITextureElement.h"
#include "../UISkin.h"

#include <algorithm>
#include <iostream>

/*
 * Creates a UITextureElement with a given size, position, and texture path.
 * Loads the texture, sets up the sprite, positions it, and applies scaling to fit the desired size.
 */
UITextureElement::UITextureElement(sf::Vector2f size, sf::Vector2f position, const std::string& texturePath) :
    UIElement(size, position),
    m_sprite(m_texture) // initialize sprite with texture
{
    // Load the texture from file
    if (!m_texture.loadFromFile(texturePath))
    {
        std::cerr << "Failed to load texture: " << texturePath << "\n";
    }
    m_texture.setSmooth(true);   // Scaled pictures stay smooth instead of stepping

    // Set the texture rectangle to cover the full texture
    m_sprite.setTextureRect(sf::IntRect(
        { 0, 0 },
        { static_cast<int>(m_texture.getSize().x), static_cast<int>(m_texture.getSize().y) }
    ));

    // Assign the texture to the sprite and set its initial position
    m_sprite.setTexture(m_texture);
    m_sprite.setPosition(m_position);

    // Scale sprite to match desired size while preserving aspect ratio
    apply_scale();
}

/*
 * Draws the sprite to the window if it is visible.
 */
void UITextureElement::render(sf::RenderWindow& window)
{
    if (!m_isVisible) return;
    window.draw(m_sprite);

    // A full-screen background on a wider screen (a phone): each side carries on with the image's edge strip,
    // mirrored, then straight, then mirrored... so the wall and the floor line run on seamlessly (only the edge:
    // what's further in, like the hub's porthole, never shows up twice)
    const sf::FloatRect screen = UISkin::screen_rect();
    const bool isBackground = m_position == sf::Vector2f{ 0.f, 0.f } && m_size == sf::Vector2f{ 1920.f, 1080.f };
    if (!isBackground || screen.size.x <= m_size.x + 1.f) return;

    const float EDGE = 150.f;   // On screen: the strip of each edge that's only wall and floor
    const sf::IntRect source = m_sprite.getTextureRect();
    const sf::Vector2f scale = m_sprite.getScale();
    const int edge = std::max(1, static_cast<int>(EDGE / scale.x));
    for (const bool isRight : { false, true })
    {
        const sf::IntRect strip = isRight
            ? sf::IntRect{ { source.position.x + source.size.x - edge, source.position.y }, { edge, source.size.y } }
            : sf::IntRect{ source.position, { edge, source.size.y } };
        sf::Sprite piece(m_texture, strip);
        const float width = edge * scale.x;
        const float extra = isRight ? screen.position.x + screen.size.x - m_size.x : -screen.position.x;
        for (int i = 0; i * width < extra; ++i)
        {
            const bool isMirrored = i % 2 == 0;   // The first copy mirrored: it meets the image at its own edge
            const float start = isRight ? m_size.x + i * width : -(i + 1) * width;   // The copy's left side
            piece.setScale({ isMirrored ? -scale.x : scale.x, scale.y });
            piece.setPosition({ isMirrored ? start + width : start, m_position.y });
            window.draw(piece);
        }
    }
}

/*
 * Loads a new texture from a file and reapplies scaling.
 */
void UITextureElement::set_texture(const std::string& texturePath)
{
    if (!m_texture.loadFromFile(texturePath))
    {
        std::cerr << "Failed to load texture: " << texturePath << "\n";
        return;
    }
    m_texture.setSmooth(true);

    m_sprite.setTexture(m_texture);
    apply_scale();
}

/*
 * Updates the size of the element and rescales the sprite to fit.
 */
void UITextureElement::set_size(sf::Vector2f newSize)
{
    m_size = newSize;
    apply_scale();
}

/*
 * Returns the global bounding rectangle of the sprite (for interaction detection).
 */
sf::FloatRect UITextureElement::get_bounds() const
{
    return m_sprite.getGlobalBounds();
}

/*
 * Calculates the scaling factor for the sprite to fit within m_size while preserving aspect ratio.
 */
void UITextureElement::apply_scale()
{
    sf::Vector2u texSize = m_texture.getSize();
    if (texSize.x == 0 || texSize.y == 0) return; // prevent division by zero

    float scaleX = m_size.x / static_cast<float>(texSize.x);
    float scaleY = m_size.y / static_cast<float>(texSize.y);

    // Use the smaller factor to maintain aspect ratio
    float scale = std::min(scaleX, scaleY);
    m_sprite.setScale({ scale, scale });

    // Ensure the texture rectangle still covers the entire texture
    m_sprite.setTextureRect(sf::IntRect(
        { 0, 0 },
        { static_cast<int>(m_texture.getSize().x), static_cast<int>(m_texture.getSize().y) }
    ));
}
