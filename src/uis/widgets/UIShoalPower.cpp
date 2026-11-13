#include "UIShoalPower.h"
#include "../UISkin.h"
#include "../../core/DataLoader.h"
#include "../../data/PlayerData.h"

#include <algorithm>

namespace
{
    const sf::FloatRect BOX{ { 1330.f, 118.f }, { 560.f, 104.f } };   // Right of where the news (UIToast) pop in
    const sf::Color TRACK{ 40, 56, 74 };
}

UIShoalPower::UIShoalPower(std::shared_ptr<DataLoader> dataLoader) :
    UIElement(BOX.size, BOX.position),
    m_dataLoader(std::move(dataLoader))
{
}

void UIShoalPower::render(sf::RenderWindow& window)
{
    auto player = m_dataLoader->get_player_data().lock();
    if (!m_isVisible || !player) return;

    UISkin::draw_nine_slice(window, UISkin::PANEL_DARK, BOX, 0.9f);
    const int power = player->get_shoal_power();
    const PlayerData::ShoalMilestone next = PlayerData::shoal_milestone(player->shoalRewardsClaimed);
    const int previous = player->shoalRewardsClaimed > 0 ? PlayerData::shoal_milestone(player->shoalRewardsClaimed - 1).power : 0;

    sf::Text text(UISkin::heavy_font(), "SHOAL POWER");
    UISkin::style_text(text, 28, UISkin::TITLE);
    UISkin::set_text_anchor(text, { 0.f, 0.5f });
    text.setPosition({ BOX.position.x + 26.f, BOX.position.y + 34.f });
    window.draw(text);
    const float labelRight = text.getGlobalBounds().position.x + text.getGlobalBounds().size.x;
    text.setString(std::to_string(power));
    UISkin::style_text(text, 40, UISkin::NUMBER);
    UISkin::set_text_anchor(text, { 0.f, 0.5f });
    text.setPosition({ labelRight + 16.f, BOX.position.y + 32.f });
    window.draw(text);

    // Next: [scale] +1 at 50
    const sf::Texture& scale = UISkin::texture("icon_shiny_scale");
    text.setString("+" + std::to_string(next.shinyScales) + " at " + std::to_string(next.power));
    UISkin::style_text(text, 28, UISkin::TEXT);
    UISkin::set_text_anchor(text, { 1.f, 0.5f });
    text.setPosition({ BOX.position.x + BOX.size.x - 26.f, BOX.position.y + 34.f });
    window.draw(text);
    UISkin::draw_fitted(window, scale, { { text.getGlobalBounds().position.x - 48.f, BOX.position.y + 14.f }, { 40.f, 40.f } });

    // The bar to the next milestone
    const sf::FloatRect bar{ { BOX.position.x + 26.f, BOX.position.y + 66.f }, { BOX.size.x - 52.f, 16.f } };
    sf::RectangleShape track(bar.size);
    track.setPosition(bar.position);
    track.setFillColor(TRACK);
    track.setOutlineThickness(3.f);
    track.setOutlineColor(UISkin::OUTLINE);
    window.draw(track);
    const float fraction = next.power > previous ? std::clamp(static_cast<float>(power - previous) / static_cast<float>(next.power - previous), 0.f, 1.f) : 0.f;
    sf::RectangleShape fill({ bar.size.x * fraction, bar.size.y });
    fill.setPosition(bar.position);
    fill.setFillColor(UISkin::EXTRA);
    window.draw(fill);
}
