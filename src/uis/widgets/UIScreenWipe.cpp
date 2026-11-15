#include "UIScreenWipe.h"
#include "../UISkin.h"

#include <algorithm>

UIScreenWipe::UIScreenWipe(std::function<void()> onCovered) :
    UIElement({ 1920.f, 1080.f }, { 0.f, 0.f }),
    m_onCovered(std::move(onCovered))
{
    set_z(Z_OVERLAY);
}

float UIScreenWipe::progress(float t)
{
    const float x = std::clamp(t / SECONDS, 0.f, 1.f);
    return 1.f - (1.f - x) * (1.f - x);   // Ease out
}

void UIScreenWipe::update(float deltaTime)
{
    m_time += deltaTime;
    if (!m_hasCovered && m_time >= SECONDS)
    {
        m_hasCovered = true;
        play_sound("ui_transition");
        if (m_onCovered) m_onCovered();
    }
}

// Its left edge comes in from the right until the band covers the whole screen
void UIScreenWipe::render(sf::RenderWindow& window)
{
    const sf::FloatRect screen = UISkin::screen_rect();
    const float right = screen.position.x + screen.size.x + UISkin::WIPE_TOOTH;
    const float travel = screen.size.x + 2.f * UISkin::WIPE_TOOTH;
    UISkin::draw_zigzag_band(window, right - travel * progress(m_time), right);
}
