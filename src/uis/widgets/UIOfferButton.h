#pragma once

#include "UIButtonElement.h"

#include <string>

/*
 * UIOfferButton
 * -------------
 * A shop's offer: what it does on top (OPEN, OPEN x10), what it costs under it ([Shiny Scale icon] 1), so the price
 * is read at a glance (the yellow one is the main offer). Two kinds:
 *   - CURRENCY: paid with something the player has (the cost row: its icon and the amount)
 *   - REWARDED_AD: free for watching an advertisement, phones only (Platform::HAS_REWARDED_ADS): teal instead of
 *     yellow, a "play video" tag and "AD" in place of the price, so it can't be taken for a purchase. No offer uses
 *     it yet: there's no ad SDK; a shop adds one next to its other offers when there is.
 *
 * Example usage:
 *   UIOfferButton open({ 360.f, 120.f }, { 580.f, 920.f }, "OPEN", UIOfferButton::Kind::CURRENCY, "icon_shiny_scale", 1);
 */
class UIOfferButton : public UIButtonElement
{
public:
    enum class Kind { CURRENCY, REWARDED_AD };

    UIOfferButton(sf::Vector2f size, sf::Vector2f position, const std::string& label, Kind kind,
        const std::string& costIcon = "", int cost = 0);

    // The price shown (a negative one hides the cost row: MAX LEVEL)
    void set_cost(int cost);

    void update(float deltaTime) override;
    void render(sf::RenderWindow& window) override;

private:
    Kind m_kind;
    std::string m_costIcon;
    sf::Text m_cost;
    bool m_hasCost = true;
};
