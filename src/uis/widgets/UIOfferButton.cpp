#include "UIOfferButton.h"
#include "../UISkin.h"

namespace
{
    const float LABEL_RISE = 0.16f;           // The label sits this much of the height above the middle
    const float COST_DROP = 0.24f;            // The cost row this much below it
    const sf::Color AD_TINT{ 150, 236, 214 }; // The free-for-an-ad offers: teal
}

UIOfferButton::UIOfferButton(sf::Vector2f size, sf::Vector2f position, const std::string& label, Kind kind,
    const std::string& costIcon, int cost) :
    UIButtonElement(size, position, label),
    m_kind(kind),
    m_costIcon(costIcon),
    m_cost(UISkin::heavy_font(), kind == Kind::REWARDED_AD ? "AD" : std::to_string(cost))
{
    UISkin::style_text(m_cost, static_cast<unsigned int>(size.y * 0.26f), kind == Kind::REWARDED_AD ? UISkin::TEXT : UISkin::NUMBER);
    if (kind == Kind::REWARDED_AD)
    {
        set_style(Style::Secondary);
        set_fill_color(AD_TINT);
    }
}

void UIOfferButton::update(float deltaTime)
{
    UIButtonElement::update(deltaTime);
    m_text.setPosition(m_text.getPosition() - sf::Vector2f{ 0.f, m_shape.getSize().y * LABEL_RISE });
}

void UIOfferButton::render(sf::RenderWindow& window)
{
    if (!m_isVisible) return;
    UIButtonElement::render(window);
    const float scale = get_visual_scale();
    if (scale < 0.02f) return;

    // The cost row, centered under the label: [icon] amount, or the ad tag
    const sf::FloatRect rect = get_visual_rect();
    const float rowY = rect.position.y + rect.size.y / 2.f + rect.size.y * COST_DROP;
    const float iconSize = rect.size.y * 0.3f;
    UISkin::set_text_anchor(m_cost, { 0.f, 0.5f });
    m_cost.setScale({ scale, scale });
    const float textWidth = m_cost.getGlobalBounds().size.x;
    const float gap = 8.f * scale;
    const float startX = rect.position.x + rect.size.x / 2.f - (iconSize + gap + textWidth) / 2.f;
    const std::uint8_t alpha = m_isEnabled ? 255 : 130;

    if (m_kind == Kind::REWARDED_AD)
    {
        // A small "play video" tag: a dark rounded square with a white triangle
        sf::ConvexShape tag = UISkin::rounded_rect({ { startX, rowY - iconSize / 2.f }, { iconSize, iconSize } }, iconSize * 0.22f);
        tag.setFillColor(sf::Color(UISkin::OUTLINE.r, UISkin::OUTLINE.g, UISkin::OUTLINE.b, alpha));
        window.draw(tag);
        sf::ConvexShape play(3);
        play.setPoint(0, { startX + iconSize * 0.36f, rowY - iconSize * 0.24f });
        play.setPoint(1, { startX + iconSize * 0.36f, rowY + iconSize * 0.24f });
        play.setPoint(2, { startX + iconSize * 0.74f, rowY });
        play.setFillColor(sf::Color(255, 255, 255, alpha));
        window.draw(play);
    }
    else if (!m_costIcon.empty())
    {
        UISkin::draw_fitted(window, UISkin::texture(m_costIcon), { { startX, rowY - iconSize / 2.f }, { iconSize, iconSize } },
            sf::Color(255, 255, 255, alpha));
    }
    const sf::Color color = m_cost.getFillColor();
    if (!m_isEnabled) UISkin::set_text_color(m_cost, sf::Color(color.r, color.g, color.b, alpha));
    m_cost.setPosition({ startX + iconSize + gap, rowY - 2.f * scale });
    window.draw(m_cost);
    UISkin::set_text_color(m_cost, color);
}
