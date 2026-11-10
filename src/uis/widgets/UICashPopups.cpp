#include "UICashPopups.h"

#include "../UISkin.h"
#include "../../core/stages/Stage.h"

#include <cstdint>

namespace
{
    const unsigned int TEXT_SIZE = 40;
    const float RISE = 70.f;   // Pixels it rises over its lifetime
}

UICashPopups::UICashPopups(std::shared_ptr<Stage> stage) :
    m_stage(stage)
{
    isWorldSpaceUi = true;
}

void UICashPopups::render(sf::RenderWindow& window)
{
    auto stage = m_stage.lock();
    if (!stage || !m_isVisible) return;

    sf::Text text(UISkin::heavy_font());
    UISkin::style_text(text, TEXT_SIZE, UISkin::NUMBER);
    for (const CashPopup& popup : stage->get_cash_popups())
    {
        const float progress = popup.age / Stage::CASH_POPUP_SECONDS;
        const std::uint8_t alpha = static_cast<std::uint8_t>(255.f * (1.f - progress * progress));

        text.setString("+" + std::to_string(popup.amount) + "$");
        UISkin::set_text_anchor(text, { 0.5f, 1.f });
        text.setPosition({ popup.position.x, popup.position.y - RISE * (1.f - (1.f - progress) * (1.f - progress)) });
        text.setFillColor(sf::Color(UISkin::NUMBER.r, UISkin::NUMBER.g, UISkin::NUMBER.b, alpha));
        text.setOutlineColor(sf::Color(UISkin::OUTLINE.r, UISkin::OUTLINE.g, UISkin::OUTLINE.b, alpha));
        window.draw(text);
    }
}
