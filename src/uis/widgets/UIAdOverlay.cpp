#include "UIAdOverlay.h"
#include "../UISkin.h"

#include <algorithm>
#include <cmath>
#include <string>

namespace
{
    const sf::FloatRect CARD{ { 960.f - 380.f, 540.f - 210.f }, { 760.f, 420.f } };
    const sf::Color AD_TEAL{ 90, 214, 190 };   // The rewarded-ad color (UIOfferButton's ad offers)
}

UIAdOverlay::UIAdOverlay() :
    UIElement({ 1920.f, 1080.f }, { 0.f, 0.f })
{
    m_isVisible = false;
}

void UIAdOverlay::play(std::function<void()> onReward)
{
    m_onReward = std::move(onReward);
    m_time = 0.f;
    m_isVisible = true;
}

void UIAdOverlay::update(float deltaTime)
{
    if (!m_isVisible) return;
    m_time += deltaTime;
    if (m_time < AD_SECONDS) return;
    m_isVisible = false;
    if (auto reward = std::move(m_onReward)) reward();
}

void UIAdOverlay::render(sf::RenderWindow& window)
{
    if (!m_isVisible) return;
    UISkin::fill_screen(window, sf::Color(0, 0, 0, 200));

    sf::ConvexShape card = UISkin::rounded_rect(CARD, 36.f);
    card.setFillColor(sf::Color(24, 34, 48));
    card.setOutlineThickness(6.f);
    card.setOutlineColor(AD_TEAL);
    window.draw(card);

    // The play tag, the ad's label, what comes after it
    const sf::Vector2f tag{ 960.f, CARD.position.y + 120.f };
    sf::ConvexShape square = UISkin::rounded_rect({ tag - sf::Vector2f{ 56.f, 56.f }, { 112.f, 112.f } }, 26.f);
    square.setFillColor(AD_TEAL);
    square.setOutlineThickness(5.f);
    square.setOutlineColor(UISkin::OUTLINE);
    window.draw(square);
    sf::ConvexShape play(3);
    play.setPoint(0, tag + sf::Vector2f{ -18.f, -28.f });
    play.setPoint(1, tag + sf::Vector2f{ -18.f, 28.f });
    play.setPoint(2, tag + sf::Vector2f{ 30.f, 0.f });
    play.setFillColor(sf::Color::White);
    window.draw(play);

    sf::Text text(UISkin::heavy_font(), "AD PLAYING", 44);
    UISkin::style_text(text, 44, AD_TEAL);
    UISkin::set_text_anchor(text, { 0.5f, 0.5f });
    text.setPosition({ 960.f, CARD.position.y + 236.f });
    window.draw(text);
    UISkin::style_body_text(text, 28);
    text.setString("Your reward comes right after it");
    UISkin::set_text_anchor(text, { 0.5f, 0.5f });
    text.setPosition({ 960.f, CARD.position.y + 290.f });
    window.draw(text);

    // The bar filling as it plays
    const sf::FloatRect bar{ { CARD.position.x + 80.f, CARD.position.y + 336.f }, { CARD.size.x - 160.f, 26.f } };
    sf::ConvexShape track = UISkin::rounded_rect(bar, 13.f);
    track.setFillColor(sf::Color(10, 16, 24));
    track.setOutlineThickness(3.f);
    track.setOutlineColor(UISkin::OUTLINE);
    window.draw(track);
    const float progress = std::clamp(m_time / AD_SECONDS, 0.f, 1.f);
    if (progress > 0.02f)
    {
        sf::ConvexShape fill = UISkin::rounded_rect({ bar.position, { bar.size.x * progress, bar.size.y } }, 13.f);
        fill.setFillColor(AD_TEAL);
        window.draw(fill);
    }
}

sf::FloatRect UIAdOverlay::get_bounds() const
{
    return m_isVisible ? sf::FloatRect{ { 0.f, 0.f }, { 1920.f, 1080.f } } : sf::FloatRect{};
}
