#include "UIPatternBackground.h"

#include <iostream>

namespace
{
    const sf::Vector2i SCREEN_SIZE{ 1920, 1080 };
}

UIPatternBackground::UIPatternBackground(const std::string& patternName, sf::Color tint) :
    UIElement(sf::Vector2f(SCREEN_SIZE), { 0.f, 0.f }),
    m_sprite(m_texture)
{
    if (!m_texture.loadFromFile("assets/images/textures/ui/" + patternName + ".png"))
        std::cerr << "UIPatternBackground: can't load " << patternName << "\n";
    m_texture.setRepeated(true);
    m_texture.setSmooth(true);

    // A texture rect bigger than the texture repeats it
    m_sprite.setTexture(m_texture);
    m_sprite.setTextureRect(sf::IntRect({ 0, 0 }, SCREEN_SIZE));
    m_sprite.setColor(tint);
}

void UIPatternBackground::render(sf::RenderWindow& window)
{
    if (m_isVisible) window.draw(m_sprite);
}

sf::FloatRect UIPatternBackground::get_bounds() const
{
    return sf::FloatRect{ { 0.f, 0.f }, { 0.f, 0.f } };
}
