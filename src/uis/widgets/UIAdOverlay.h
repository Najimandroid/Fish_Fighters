#pragma once

#include "UIElement.h"

#include <functional>

/*
 * UIAdOverlay
 * -----------
 * A rewarded advertisement, over the whole screen (phones: Platform::HAS_REWARDED_ADS). There's no ad SDK yet, so it
 * plays a stand-in: a card that says an ad is playing, a bar filling for AD_SECONDS, then the reward is given. An SDK's
 * rewarded ad goes in play() later; the offers that call it don't change.
 * Add it last (it takes every tap while it plays).
 *
 * Example usage:
 *   adOverlay->play([this]() { player->watch_ad(); });
 */
class UIAdOverlay : public UIElement
{
public:
    static constexpr float AD_SECONDS = 2.f;

    UIAdOverlay();

    // Plays an ad; onReward runs once it has been watched
    void play(std::function<void()> onReward);
    bool is_playing() const { return m_isVisible; }

    void update(float deltaTime) override;
    void render(sf::RenderWindow& window) override;
    void handle_event(const sf::Event& event, const sf::RenderWindow& window) override {}   // Nothing goes through
    sf::FloatRect get_bounds() const override;
    bool is_blocking_input() const override { return m_isVisible; }

private:
    std::function<void()> m_onReward;
    float m_time = 0.f;
};
