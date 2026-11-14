#include "UIBattleIntro.h"
#include "../UISkin.h"

#include <algorithm>
#include <cmath>

namespace
{
    const float COVER = 0.35f;              // The water rising over the screen
    const float UNCOVER = 0.45f;            // The water sweeping up and away (the jingle's end)
    const float TITLE_IN = 0.2f;            // When the title pops in
    const float OFF = 400.f;                // How far past the screen the water's far edge stays

    float ease(float t) { return 0.5f - 0.5f * std::cos(3.14159265f * std::clamp(t, 0.f, 1.f)); }
}

UIBattleIntro::UIBattleIntro(const std::string& stageName, float duration) :
    m_title(UISkin::heavy_font(), "GET READY FOR BATTLE!"),
    m_stageName(UISkin::heavy_font(), stageName),
    m_duration(std::max(COVER + UNCOVER + 0.3f, duration))
{
    UISkin::style_text(m_title, 96, UISkin::NUMBER);
    UISkin::set_text_anchor(m_title, { 0.5f, 0.5f });
    m_title.setPosition({ 960.f, 480.f });
    UISkin::style_text(m_stageName, 46, UISkin::TITLE);
    UISkin::set_text_anchor(m_stageName, { 0.5f, 0.5f });
    m_stageName.setPosition({ 960.f, 590.f });
}

void UIBattleIntro::update(float deltaTime)
{
    if (!m_isVisible) return;
    m_time += deltaTime;
    if (m_time >= m_duration) m_isVisible = false;
}

void UIBattleIntro::render(sf::RenderWindow& window)
{
    if (!m_isVisible) return;

    // The water: its top edge rises over the screen, then its bottom edge follows it up and away
    const float wave = UISkin::WATER_WAVE;
    const float leaving = m_time - (m_duration - UNCOVER);
    float top = -wave - OFF, bottom = 1080.f + wave + OFF;
    if (m_time < COVER) top = (1080.f + wave) - (1080.f + 2.f * wave) * ease(m_time / COVER);
    if (leaving > 0.f) bottom = (1080.f + wave) - (1080.f + 2.f * wave) * ease(leaving / UNCOVER);
    UISkin::draw_water_band(window, top, bottom, m_time);

    // The title pops in (overshoots a little, settles), the stage's name under it; both rise and fade with the water
    if (m_time < TITLE_IN) return;
    const float t = std::min(1.f, (m_time - TITLE_IN) / 0.3f);
    const float pop = t < 0.6f ? t / 0.6f * 1.12f : 1.12f - (t - 0.6f) / 0.4f * 0.12f;
    const float rise = leaving > 0.f ? (1080.f + wave) * ease(leaving / UNCOVER) : 0.f;
    const float fade = leaving > 0.f ? std::clamp(1.f - leaving / (UNCOVER * 0.6f), 0.f, 1.f) : 1.f;
    const std::uint8_t alpha = static_cast<std::uint8_t>(255 * fade);
    if (alpha == 0) return;

    m_title.setScale({ pop, pop });
    m_title.setPosition({ 960.f, 480.f - rise });
    UISkin::set_text_color(m_title, sf::Color(UISkin::NUMBER.r, UISkin::NUMBER.g, UISkin::NUMBER.b, alpha));
    window.draw(m_title);
    if (!m_stageName.getString().isEmpty())
    {
        m_stageName.setPosition({ 960.f, 590.f - rise });
        UISkin::set_text_color(m_stageName, sf::Color(UISkin::TITLE.r, UISkin::TITLE.g, UISkin::TITLE.b, alpha));
        window.draw(m_stageName);
    }
}
