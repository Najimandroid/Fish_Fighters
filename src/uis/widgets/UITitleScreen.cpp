#include "UITitleScreen.h"

#include "../UISkin.h"

#include <cmath>
#include <random>

namespace
{
    const int BUBBLE_COUNT = 34;
    const sf::Color BUBBLE_FILL{ 200, 235, 255, 40 };
    const sf::Color BUBBLE_EDGE{ 220, 245, 255, 150 };
    const sf::Color VEIL{ 6, 22, 40, 110 };              // Darkens the fish tank behind the title
    const float TITLE_TOP_Y = 270.f;
    const float TITLE_BOTTOM_Y = 440.f;
}

UITitleScreen::UITitleScreen(float beatsPerMinute) :
    UIElement({ 1920.f, 1080.f }, { 0.f, 0.f }),
    m_beatSeconds(60.f / beatsPerMinute)
{
    // Bubbles spread over the whole screen at first, then they come back from the bottom
    std::mt19937 random(7);
    std::uniform_real_distribution<float> x(0.f, 1920.f), y(0.f, 1080.f), radius(4.f, 16.f), speed(40.f, 140.f), phase(0.f, 6.28f);
    for (int i = 0; i < BUBBLE_COUNT; ++i)
        m_bubbles.push_back({ { x(random), y(random) }, radius(random), speed(random), phase(random) });
}

void UITitleScreen::set_start_callback(std::function<void()> callback)
{
    m_startCallback = std::move(callback);
}

void UITitleScreen::update(float deltaTime)
{
    m_time += deltaTime;
    for (auto& bubble : m_bubbles)
    {
        bubble.position.y -= bubble.speed * deltaTime;
        if (bubble.position.y < -bubble.radius * 2.f) bubble.position.y = 1080.f + bubble.radius * 2.f;
    }
}

// Any key starts the game, like the START button (the mouse goes through the button)
void UITitleScreen::handle_event(const sf::Event& event, const sf::RenderWindow&)
{
    if (event.is<sf::Event::KeyPressed>() && m_startCallback)
        m_startCallback();
}

/*
 * One line of the title: yellow letters, thick navy outline, a shadow, swelling on every beat of the music
 */
void UITitleScreen::draw_title_line(sf::RenderWindow& window, const char* text, float centerY, unsigned int size, float bounce)
{
    sf::Text line(UISkin::heavy_font(), text, size);
    UISkin::style_text(line, size, UISkin::NUMBER);
    line.setOutlineThickness(size * 0.09f);
    UISkin::set_text_anchor(line, { 0.5f, 0.5f });
    line.setScale({ bounce, bounce });
    line.setRotation(sf::degrees(std::sin(m_time * 1.3f) * 2.f));

    sf::Text shadow = line;
    shadow.setFillColor(sf::Color(0, 0, 0, 110));
    shadow.setOutlineColor(sf::Color(0, 0, 0, 110));
    shadow.setPosition({ 960.f + 10.f, centerY + 14.f });
    window.draw(shadow);

    line.setPosition({ 960.f, centerY });
    window.draw(line);
}

void UITitleScreen::render(sf::RenderWindow& window)
{
    sf::RectangleShape veil({ 1920.f, 1080.f });
    veil.setFillColor(VEIL);
    window.draw(veil);

    // Bubbles, wobbling side to side
    sf::CircleShape circle;
    circle.setFillColor(BUBBLE_FILL);
    circle.setOutlineColor(BUBBLE_EDGE);
    for (const auto& bubble : m_bubbles)
    {
        circle.setRadius(bubble.radius);
        circle.setOrigin({ bubble.radius, bubble.radius });
        circle.setOutlineThickness(std::max(1.5f, bubble.radius * 0.18f));
        circle.setPosition({ bubble.position.x + std::sin(m_time * 2.f + bubble.phase) * 10.f, bubble.position.y });
        window.draw(circle);
    }

    // The title swells on each beat and settles back (the title theme starts with the screen)
    const float beatPhase = std::fmod(m_time, m_beatSeconds) / m_beatSeconds;
    const float bounce = 1.f + 0.045f * std::exp(-beatPhase * 6.f);
    const float bob = std::sin(m_time * 1.7f) * 8.f;
    draw_title_line(window, "FISH", TITLE_TOP_Y + bob, 190, bounce);
    draw_title_line(window, "FIGHTERS", TITLE_BOTTOM_Y + bob, 210, bounce);

    // Blinking hint under the START button
    sf::Text hint(UISkin::heavy_font(), "or press any key", 34);
    UISkin::style_text(hint, 34, UISkin::TITLE);
    UISkin::set_text_anchor(hint, { 0.5f, 0.5f });
    hint.setPosition({ 960.f, 905.f });
    const std::uint8_t alpha = static_cast<std::uint8_t>(140 + 115 * (0.5f + 0.5f * std::sin(m_time * 4.f)));
    hint.setFillColor(sf::Color(UISkin::TITLE.r, UISkin::TITLE.g, UISkin::TITLE.b, alpha));
    hint.setOutlineColor(sf::Color(UISkin::OUTLINE.r, UISkin::OUTLINE.g, UISkin::OUTLINE.b, alpha));
    window.draw(hint);

    // Credit
    sf::Text credit(UISkin::regular_font(), "by Najimandroid", 26);
    UISkin::style_text(credit, 26, UISkin::TITLE);
    UISkin::set_text_anchor(credit, { 1.f, 1.f });
    credit.setPosition({ 1920.f - 30.f, 1080.f - 24.f });
    window.draw(credit);
}
