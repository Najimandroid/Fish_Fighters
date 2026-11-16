#include "UIEnergyMeter.h"
#include "UIButtonElement.h"
#include "../UISkin.h"
#include "../../core/DataLoader.h"
#include "../../data/PlayerData.h"

#include <algorithm>
#include <cmath>

namespace
{
    const float TOP = 18.f;              // Like the shells' pill on the title bar
    const float HEIGHT = 74.f;
    const float PADDING = 18.f;
    const float ICON_SIZE = 62.f;
    const float TRAILING_ROOM = 58.f;    // The "+" inside its right end
    const float ROLL_RATE = 7.f;         // How fast the digits roll to a new amount
    const sf::Color TRACK{ 40, 56, 74 };
    const sf::Color LOW{ 255, 140, 90 }; // The gauge under a quarter
}

UIEnergyMeter::UIEnergyMeter(std::shared_ptr<DataLoader> dataLoader) :
    UIElement({ WIDTH, HEIGHT }, { 1920.f - RIGHT_OFFSET - WIDTH, TOP }),
    m_dataLoader(std::move(dataLoader)),
    m_digits(UISkin::heavy_font()),
    m_timer(UISkin::heavy_font())
{
    set_frame(true);
    UISkin::style_text(m_digits, 34, UISkin::NUMBER);   // "106/106" and "12:05" side by side
    UISkin::style_text(m_timer, 24, UISkin::COOLDOWN);
}

void UIEnergyMeter::set_refill_button(const std::shared_ptr<UIButtonElement>& button)
{
    m_refillButton = button;
}

sf::Vector2f UIEnergyMeter::refill_button_position(float size)
{
    return { 1920.f - RIGHT_OFFSET - 12.f - size, TOP + HEIGHT / 2.f - size / 2.f };
}

std::string UIEnergyMeter::format_time(int seconds)
{
    seconds = std::max(0, seconds);
    const int hours = seconds / 3600, minutes = seconds / 60 % 60, rest = seconds % 60;
    const auto two = [](int value) { return (value < 10 ? "0" : "") + std::to_string(value); };
    return hours > 0 ? std::to_string(hours) + ":" + two(minutes) + ":" + two(rest) : std::to_string(minutes) + ":" + two(rest);
}

void UIEnergyMeter::update(float deltaTime)
{
    auto player = m_dataLoader->get_player_data().lock();
    if (!player) return;

    const int energy = player->get_energy();
    const int max = player->get_max_energy();
    if (m_lastEnergy >= 0 && energy > m_lastEnergy) m_pop = 1.f;   // A point came back (or a refill)
    m_lastEnergy = energy;
    m_pop = std::max(0.f, m_pop - deltaTime * 5.f);

    // The digits roll to a new amount instead of jumping
    const float target = static_cast<float>(energy);
    if (m_shown < 0.f) m_shown = target;
    else if (m_shown != target)
    {
        m_shown += (target - m_shown) * std::min(1.f, deltaTime * ROLL_RATE);
        if (std::abs(target - m_shown) < 0.5f) m_shown = target;
    }
    m_digits.setString(std::to_string(static_cast<int>(std::lround(m_shown))) + "/" + std::to_string(max));
    m_fraction = max > 0 ? std::clamp(m_shown / static_cast<float>(max), 0.f, 1.f) : 0.f;

    // The next point's countdown, or FULL
    const bool isFull = energy >= max;
    m_timer.setString(isFull ? "FULL" : format_time(player->seconds_to_next_energy()));
    UISkin::set_text_color(m_timer, isFull ? UISkin::TITLE : UISkin::COOLDOWN);
    if (auto button = m_refillButton.lock()) button->set_enabled(!isFull);

    const float left = m_position.x + PADDING + ICON_SIZE + 12.f;
    const float right = m_position.x + m_size.x - PADDING - TRAILING_ROOM;
    UISkin::set_text_anchor(m_digits, { 0.f, 0.5f });
    m_digits.setPosition({ left, m_position.y + 30.f });
    UISkin::set_text_anchor(m_timer, { 1.f, 0.5f });
    m_timer.setPosition({ right, m_position.y + 31.f });
    // Never over the digits (a long wait with a big max): the gauge still shows it
    m_isTimerShown = m_digits.getGlobalBounds().position.x + m_digits.getGlobalBounds().size.x + 10.f < m_timer.getGlobalBounds().position.x;
}

void UIEnergyMeter::render(sf::RenderWindow& window)
{
    if (!m_isVisible) return;

    UISkin::draw_nine_slice(window, UISkin::PANEL_DARK, get_bounds());
    const float iconSize = ICON_SIZE * (1.f + 0.3f * m_pop);
    const sf::Vector2f iconCenter{ m_position.x + PADDING + ICON_SIZE / 2.f, m_position.y + m_size.y / 2.f };
    UISkin::draw_fitted(window, UISkin::texture("icon_energy"), { iconCenter - sf::Vector2f{ iconSize, iconSize } / 2.f, { iconSize, iconSize } });
    window.draw(m_digits);
    if (m_isTimerShown) window.draw(m_timer);

    // The gauge under them
    const float left = m_position.x + PADDING + ICON_SIZE + 12.f;
    const float right = m_position.x + m_size.x - PADDING - TRAILING_ROOM;
    const sf::FloatRect bar{ { left, m_position.y + 51.f }, { right - left, 11.f } };
    sf::RectangleShape track(bar.size);
    track.setPosition(bar.position);
    track.setFillColor(TRACK);
    track.setOutlineThickness(3.f);
    track.setOutlineColor(UISkin::OUTLINE);
    window.draw(track);
    sf::RectangleShape fill({ bar.size.x * m_fraction, bar.size.y });
    fill.setPosition(bar.position);
    fill.setFillColor(m_fraction < 0.25f ? LOW : UISkin::NUMBER);
    window.draw(fill);
}
