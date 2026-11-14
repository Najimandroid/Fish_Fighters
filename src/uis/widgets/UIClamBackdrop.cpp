#include "UIClamBackdrop.h"
#include "../UISkin.h"

#include <cmath>

namespace
{
    const sf::Color TOP{ 40, 30, 82 }, BOTTOM{ 12, 10, 34 };
    const sf::Color BEAM{ 200, 186, 255 };
    const sf::Vector2f STAGE{ 960.f, 862.f };   // Under the clam (UIClamRoom's CLAM_BASE)
}

UIClamBackdrop::UIClamBackdrop() :
    UIElement({ 1920.f, 1080.f }, { 0.f, 0.f })
{
}

void UIClamBackdrop::update(float deltaTime)
{
    m_time += deltaTime;
}

void UIClamBackdrop::render(sf::RenderWindow& window)
{
    if (!m_isVisible) return;
    const sf::FloatRect screen = UISkin::screen_rect();
    const float left = screen.position.x, right = left + screen.size.x, top = screen.position.y, bottom = top + screen.size.y;

    // The room: dark violet, darker at the bottom
    sf::VertexArray room(sf::PrimitiveType::TriangleStrip, 4);
    room[0] = sf::Vertex{ { left, top }, TOP };
    room[1] = sf::Vertex{ { right, top }, TOP };
    room[2] = sf::Vertex{ { left, bottom }, BOTTOM };
    room[3] = sf::Vertex{ { right, bottom }, BOTTOM };
    window.draw(room);

    // The stage under the clam: a soft pale disc, its rim a little darker
    for (int ring = 0; ring < 3; ++ring)
    {
        const float grow = 1.f + 0.18f * static_cast<float>(2 - ring);
        sf::CircleShape stage(420.f * grow);
        stage.setScale({ 1.f, 0.16f });
        stage.setOrigin({ 420.f * grow, 420.f * grow });
        stage.setPosition(STAGE);
        stage.setFillColor(sf::Color(110, 94, 170, static_cast<std::uint8_t>(ring == 2 ? 120 : 40)));
        window.draw(stage);
    }

    // One beam of light from above onto the clam (added light, breathing slowly)
    sf::RenderStates additive;
    additive.blendMode = sf::BlendAdd;
    const float breathe = 0.85f + 0.15f * std::sin(m_time * 0.8f);
    const sf::Color bright(BEAM.r, BEAM.g, BEAM.b, static_cast<std::uint8_t>(52.f * breathe));
    const sf::Color clear(BEAM.r, BEAM.g, BEAM.b, 0);
    sf::VertexArray beam(sf::PrimitiveType::TriangleStrip, 4);
    beam[0] = sf::Vertex{ { 960.f - 120.f, top }, bright };
    beam[1] = sf::Vertex{ { 960.f + 120.f, top }, bright };
    beam[2] = sf::Vertex{ { 960.f - 470.f, STAGE.y + 40.f }, clear };
    beam[3] = sf::Vertex{ { 960.f + 470.f, STAGE.y + 40.f }, clear };
    window.draw(beam, additive);

    // A few motes drifting down through it
    sf::CircleShape mote(3.f);
    mote.setOrigin({ 3.f, 3.f });
    for (int i = 0; i < 14; ++i)
    {
        const float speed = 18.f + static_cast<float>((i * 37) % 22);
        const float y = top + std::fmod(m_time * speed + static_cast<float>(i) * 97.f, STAGE.y - top);
        const float spread = 120.f + (y - top) / (STAGE.y - top) * 330.f;   // Inside the widening beam
        const float x = 960.f + spread * std::sin(static_cast<float>(i) * 2.39f) + std::sin(m_time * 0.6f + i) * 12.f;
        mote.setFillColor(sf::Color(230, 220, 255, static_cast<std::uint8_t>(70 + (i * 23) % 80)));
        mote.setPosition({ x, y });
        window.draw(mote, additive);
    }
}
