#include "UIBattleIntro.h"
#include "../UISkin.h"

#include <algorithm>
#include <cmath>
#include <random>

namespace
{
    const float SCREEN_HEIGHT = 1080.f;
    const float WALL_HEIGHT = 1500.f;       // The wall of bubbles: taller than the screen, so it covers all of it mid-way
    const float CELL = 110.f;               // One bubble per cell of the wall (jittered): no holes
    const float MIN_SIZE = 150.f, MAX_SIZE = 260.f;
    const float TITLE_IN = 0.12f;           // When the title pops in
    const float TITLE_OUT = 0.35f;          // Its last seconds fade
    const char* BUBBLE_TEXTURE = "assets/images/textures/ui/bubble.png";
}

UIBattleIntro::UIBattleIntro(const std::string& stageName, float duration) :
    m_title(UISkin::heavy_font(), "GET READY FOR BATTLE!"),
    m_stageName(UISkin::heavy_font(), stageName),
    m_duration(std::max(0.5f, duration))
{
    UISkin::style_text(m_title, 96, UISkin::NUMBER);
    UISkin::set_text_anchor(m_title, { 0.5f, 0.5f });
    m_title.setPosition({ 960.f, 470.f });
    UISkin::style_text(m_stageName, 44, UISkin::TITLE);
    UISkin::set_text_anchor(m_stageName, { 0.5f, 0.5f });
    m_stageName.setPosition({ 960.f, 580.f });

    // The wall starts right above the screen and has fully gone past the bottom when the jingle ends:
    // the slowest bubble (the smallest, 10% slower) travels the wall and the screen in the duration
    const float distance = SCREEN_HEIGHT + WALL_HEIGHT + MAX_SIZE;
    const float baseSpeed = distance / (m_duration * 0.9f);

    std::mt19937 random{ std::random_device{}() };
    auto between = [&random](float from, float to) { return std::uniform_real_distribution<float>(from, to)(random); };
    for (float y = 0.f; y < WALL_HEIGHT; y += CELL)
    {
        for (float x = -CELL / 2.f; x < 1920.f + CELL / 2.f; x += CELL)
        {
            Bubble bubble;
            bubble.size = between(MIN_SIZE, MAX_SIZE);
            bubble.x = x + between(-0.35f, 0.35f) * CELL - bubble.size / 2.f;
            bubble.y = -y - bubble.size + between(-0.35f, 0.35f) * CELL;
            // Bigger ones a bit faster (nearer), never slower than 0.9 of the base: all gone in time
            bubble.speed = baseSpeed * (0.9f + 0.2f * (bubble.size - MIN_SIZE) / (MAX_SIZE - MIN_SIZE));
            bubble.sway = between(0.f, 6.28f);
            m_bubbles.push_back(bubble);
        }
    }
    // The small (far) ones drawn first
    std::sort(m_bubbles.begin(), m_bubbles.end(), [](const Bubble& a, const Bubble& b) { return a.size < b.size; });
}

void UIBattleIntro::update(float deltaTime)
{
    if (!m_isVisible) return;
    m_time += deltaTime;
    for (Bubble& bubble : m_bubbles) bubble.y += bubble.speed * deltaTime;
    if (m_time >= m_duration) m_isVisible = false;
}

void UIBattleIntro::render(sf::RenderWindow& window)
{
    if (!m_isVisible) return;

    const sf::Texture& texture = UISkin::texture_file(BUBBLE_TEXTURE);
    const float textureSize = static_cast<float>(std::max(1u, texture.getSize().x));
    sf::Sprite sprite(texture);
    for (const Bubble& bubble : m_bubbles)
    {
        if (bubble.y > SCREEN_HEIGHT || bubble.y + bubble.size < 0.f) continue;
        const float wobble = std::sin(m_time * 5.f + bubble.sway) * bubble.size * 0.06f;
        sprite.setScale({ bubble.size / textureSize, bubble.size / textureSize });
        sprite.setPosition({ bubble.x + wobble, bubble.y });
        window.draw(sprite);
    }

    // The title pops in (overshoots a little, settles), the stage's name under it, both fade out at the end
    if (m_time >= TITLE_IN)
    {
        const float t = std::min(1.f, (m_time - TITLE_IN) / 0.3f);
        const float pop = t < 0.6f ? t / 0.6f * 1.15f : 1.15f - (t - 0.6f) / 0.4f * 0.15f;
        const float fade = std::clamp((m_duration - m_time) / TITLE_OUT, 0.f, 1.f);
        const std::uint8_t alpha = static_cast<std::uint8_t>(255 * fade);
        m_title.setScale({ pop, pop });
        UISkin::set_text_color(m_title, sf::Color(UISkin::NUMBER.r, UISkin::NUMBER.g, UISkin::NUMBER.b, alpha));
        window.draw(m_title);
        if (!m_stageName.getString().isEmpty())
        {
            UISkin::set_text_color(m_stageName, sf::Color(UISkin::TITLE.r, UISkin::TITLE.g, UISkin::TITLE.b, alpha));
            window.draw(m_stageName);
        }
    }
}
