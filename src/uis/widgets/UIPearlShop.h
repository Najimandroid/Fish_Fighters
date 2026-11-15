#pragma once

#include "UIElement.h"
#include "UIButtonElement.h"
#include "UIOfferButton.h"

#include <memory>
#include <string>
#include <vector>

class DataLoader;
class UIConfirmDialog;
class UIAdOverlay;

/*
 * UIPearlShop
 * -----------
 * The Pearl Shop screen's offers, like The Battle Cats' cat food shop: Pearls are the rare currency. Two rows of
 * cards (a title, a picture, a line under it, a button):
 *   - PEARLS: 5 for a short video (phones, PlayerData::SHOP_ADS_PER_DAY a day: the screen's UIAdOverlay, a stand-in
 *     ad for now), then packs for real money (30 / 110 / 300), shown but not sold yet: "coming soon"
 *   - SHINY SCALES: 1 for 25 Pearls, 10 for 225 (asked first: the screen's UIConfirmDialog)
 * UIManager::generate_shop_uis builds the screen around it (title bar with the counters, bottom bar, back button to
 * the screen it was opened from); the dialog and the ad are their own elements, over the bars.
 */
class UIPearlShop : public UIElement
{
public:
    UIPearlShop(std::shared_ptr<DataLoader> dataLoader, std::weak_ptr<UIConfirmDialog> confirm, std::weak_ptr<UIAdOverlay> ad);

    void update(float deltaTime) override;
    void render(sf::RenderWindow& window) override;
    void handle_event(const sf::Event& event, const sf::RenderWindow& window) override;

private:
    struct Card
    {
        sf::FloatRect rect;
        std::string title;
        const char* icon = "icon_pearl";
        int iconCount = 1;                        // A pile of them, for the bigger packs
        std::string line;                         // Under the picture
        sf::Color lineColor;
        std::unique_ptr<UIButtonElement> button;
    };

    void buy_scales(int count);                   // Asks, then pays

    std::shared_ptr<DataLoader> m_dataLoader;
    std::weak_ptr<UIConfirmDialog> m_confirm;
    std::weak_ptr<UIAdOverlay> m_ad;
    std::vector<Card> m_cards;                    // The free one, the packs, then the Shiny Scales
    sf::Text m_text;
};
