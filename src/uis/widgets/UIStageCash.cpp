#include "UIStageCash.h"
#include "../UISkin.h"
#include "../../core/stages/Stage.h"

#include <algorithm>
#include <cmath>

namespace
{
    const sf::FloatRect PLATE{ { 1456.f, 22.f }, { 440.f, 104.f } };
    const float AMOUNT_RIGHT = PLATE.position.x + 250.f;     // The cash is right-aligned here (digits don't push the layout)
    const float AMOUNT_Y = PLATE.position.y + 46.f;
    const sf::FloatRect BAR{ { PLATE.position.x + 24.f, PLATE.position.y + PLATE.size.y - 26.f }, { PLATE.size.x - 48.f, 9.f } };
    const sf::Color PLATE_TINT{ 255, 255, 255, 220 };
    const sf::Color BAR_TRACK{ 40, 56, 74 };
    const sf::Color FULL_COLOR{ 72, 224, 255 };             // Cyan: the wallet is full
}

UIStageCash::UIStageCash(std::shared_ptr<Stage> stage) :
    UITextElement({ 1.f, 1.f }, { 1.f, 1.f }),
    m_stage(stage),
    m_maxText(UISkin::heavy_font()),
    m_fullText(UISkin::heavy_font(), "FULL")
{
    UISkin::style_text(m_text, 60, UISkin::NUMBER);
    UISkin::style_text(m_maxText, 34, UISkin::TITLE);
    UISkin::style_text(m_fullText, 26, FULL_COLOR);
}

void UIStageCash::update(float deltaTime)
{
    m_time += deltaTime;
    auto stage = m_stage.lock();
    if (!stage) return;

    const int cash = stage->get_cash(), max = std::max(1, stage->get_max_cash());
    m_text.setString(std::to_string(cash));
    m_maxText.setString("/ " + std::to_string(max) + "$");
    m_fill = std::clamp(static_cast<float>(cash) / max, 0.f, 1.f);

    UISkin::set_text_anchor(m_text, { 1.f, 0.5f });
    m_text.setPosition({ AMOUNT_RIGHT, AMOUNT_Y });
    UISkin::set_text_anchor(m_maxText, { 0.f, 0.5f });
    m_maxText.setPosition({ AMOUNT_RIGHT + 14.f, AMOUNT_Y + 6.f });
}

void UIStageCash::render(sf::RenderWindow& window)
{
    if (!m_isVisible) return;

    UISkin::draw_nine_slice(window, UISkin::PANEL_DARK, PLATE, 0.8f, PLATE_TINT);
    window.draw(m_text);
    window.draw(m_maxText);

    // How full the wallet is: yellow, cyan when full
    const bool isFull = m_fill >= 1.f;
    sf::RectangleShape track(BAR.size);
    track.setPosition(BAR.position);
    track.setFillColor(BAR_TRACK);
    window.draw(track);
    sf::RectangleShape fill({ BAR.size.x * m_fill, BAR.size.y });
    fill.setPosition(BAR.position);
    fill.setFillColor(isFull ? FULL_COLOR : UISkin::NUMBER);
    window.draw(fill);

    // "FULL", pulsing under the plate: spend it, or Cash Up
    if (isFull)
    {
        const float pulse = 1.f + 0.06f * std::sin(m_time * 2.f * 3.14159265f / 0.8f);
        UISkin::set_text_anchor(m_fullText, { 1.f, 0.f });
        m_fullText.setScale({ pulse, pulse });
        m_fullText.setPosition({ PLATE.position.x + PLATE.size.x - 12.f, PLATE.position.y + PLATE.size.y + 8.f });
        window.draw(m_fullText);
    }
}
