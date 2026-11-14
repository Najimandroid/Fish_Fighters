#include "UIAmbience.h"
#include "../UISkin.h"

#include <cmath>

namespace
{
    const int SHAFTS = 5;
    const float SHAFT_ALPHA = 30.f;                 // At the top (added light), fading to nothing at the bottom
    const int SPECKS = 22;
    const float VIGNETTE_ALPHA = 120.f;             // At the corners
    const sf::Color LIGHT{ 190, 235, 255 };
    const sf::Color DARK{ 6, 14, 26 };
}

UIAmbience::UIAmbience(bool hasLight) :
    UIElement({ 1920.f, 1080.f }, { 0.f, 0.f }),
    m_hasLight(hasLight)
{
    std::uniform_real_distribution<float> unit(0.f, 1.f);
    for (int i = 0; i < SPECKS; ++i)
        m_specks.push_back({ { unit(m_random) * 1920.f, unit(m_random) * 1080.f }, 1.5f + unit(m_random) * 2.5f,
            8.f + unit(m_random) * 16.f, unit(m_random) * 6.28f });
}

void UIAmbience::update(float deltaTime)
{
    m_time += deltaTime;
    for (Speck& speck : m_specks)
    {
        speck.position.y -= speck.speed * deltaTime;
        speck.position.x += std::sin(m_time * 0.7f + speck.phase) * 6.f * deltaTime;
        const sf::FloatRect screen = UISkin::screen_rect();
        if (speck.position.y < screen.position.y - 10.f)
            speck.position = { std::uniform_real_distribution<float>(screen.position.x, screen.position.x + screen.size.x)(m_random), screen.position.y + screen.size.y + 10.f };
    }
}

void UIAmbience::render(sf::RenderWindow& window)
{
    if (!m_isVisible) return;

    if (m_hasLight)
    {
        // Slanted shafts, wider at the bottom, swaying and breathing each at its own pace (added on the wall)
        sf::RenderStates additive;
        additive.blendMode = sf::BlendAdd;
        for (int i = 0; i < SHAFTS; ++i)
        {
            const float phase = static_cast<float>(i) * 1.9f;
            const sf::FloatRect screen = UISkin::screen_rect();   // Spread over a phone's extra width too
            const float x = screen.position.x + screen.size.x * (0.1f + 0.21f * static_cast<float>(i)) + std::sin(m_time * 0.13f + phase) * 70.f;
            const float topWidth = 90.f + 50.f * static_cast<float>(i % 2);
            const float alpha = SHAFT_ALPHA * (0.6f + 0.4f * std::sin(m_time * 0.5f + phase * 1.3f));
            const sf::Color top(LIGHT.r, LIGHT.g, LIGHT.b, static_cast<std::uint8_t>(alpha));
            const sf::Color bottom(LIGHT.r, LIGHT.g, LIGHT.b, 0);
            sf::VertexArray shaft(sf::PrimitiveType::TriangleStrip, 4);
            shaft[0] = sf::Vertex{ { x, -20.f }, top };
            shaft[1] = sf::Vertex{ { x + topWidth, -20.f }, top };
            shaft[2] = sf::Vertex{ { x - 320.f, 1100.f }, bottom };
            shaft[3] = sf::Vertex{ { x - 320.f + topWidth * 2.4f, 1100.f }, bottom };
            window.draw(shaft, additive);
        }

        // Specks drifting up through the light
        sf::CircleShape dot;
        for (const Speck& speck : m_specks)
        {
            dot.setRadius(speck.radius);
            dot.setOrigin({ speck.radius, speck.radius });
            dot.setPosition(speck.position);
            const float twinkle = 0.5f + 0.5f * std::sin(m_time * 1.7f + speck.phase);
            dot.setFillColor(sf::Color(LIGHT.r, LIGHT.g, LIGHT.b, static_cast<std::uint8_t>(40.f + 50.f * twinkle)));
            window.draw(dot, sf::RenderStates(sf::BlendAdd));
        }
    }

    // The vignette: a ring from clear (an ellipse around the middle) to dark (past the corners)
    const int segments = 48;
    const sf::Vector2f center{ 960.f, 560.f };
    sf::VertexArray ring(sf::PrimitiveType::TriangleStrip, (segments + 1) * 2);
    for (int i = 0; i <= segments; ++i)
    {
        const float angle = static_cast<float>(i) / segments * 6.2831853f;
        const sf::Vector2f direction{ std::cos(angle), std::sin(angle) };
        ring[i * 2] = sf::Vertex{ center + sf::Vector2f{ direction.x * 760.f, direction.y * 400.f }, sf::Color(DARK.r, DARK.g, DARK.b, 0) };
        const float wide = UISkin::screen_rect().size.x / 1920.f;   // Past a phone's corners too
        ring[i * 2 + 1] = sf::Vertex{ center + sf::Vector2f{ direction.x * 1380.f * wide, direction.y * 820.f },
            sf::Color(DARK.r, DARK.g, DARK.b, static_cast<std::uint8_t>(VIGNETTE_ALPHA)) };
    }
    window.draw(ring);
}
