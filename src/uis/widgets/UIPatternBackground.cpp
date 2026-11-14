#include "UIPatternBackground.h"
#include "../UISkin.h"

#include <cmath>

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
    if (!m_isVisible) return;
    // The whole screen, a phone's extra width too (the pattern keeps its place under the layout)
    const sf::FloatRect screen = UISkin::screen_rect();
    const sf::Vector2i origin{ static_cast<int>(std::floor(screen.position.x)), static_cast<int>(std::floor(screen.position.y)) };
    m_sprite.setTextureRect(sf::IntRect(origin, { static_cast<int>(std::ceil(screen.size.x)) + 2, static_cast<int>(std::ceil(screen.size.y)) + 2 }));
    m_sprite.setPosition(sf::Vector2f(origin));
    window.draw(m_sprite);
}

sf::FloatRect UIPatternBackground::get_bounds() const
{
    return sf::FloatRect{ { 0.f, 0.f }, { 0.f, 0.f } };
}
