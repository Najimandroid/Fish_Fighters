#pragma once

#include "UIElement.h"
#include "UIPopupFrame.h"
#include "UIOfferButton.h"
#include "UIConfirmDialog.h"
#include "UIAdOverlay.h"

#include <memory>
#include <string>
#include <vector>

class DataLoader;

/*
 * UIShopMenu
 * ----------
 * The Pearl Shop, like The Battle Cats' cat food shop: Pearls are the rare currency. A window over the dimmed screen
 * (UIPopupFrame) with what the player has at the top, then three rows:
 *   - FREE PEARLS: a short video for PEARLS_PER_AD Pearls, a few a day (phones: UIAdOverlay, a stand-in ad for now)
 *   - PEARL PACKS: Pearls for real money, shown but not sold yet (no payment system): "coming soon"
 *   - SHINY SCALES: 1 or 10 Shiny Scales for Pearls (asked first: UIConfirmDialog)
 * The + on the Pearls counter of every bottom bar opens it (UIManager::add_bottom_bar). It closes with its X, a tap
 * outside it or Escape. It's always added last (set_topmost): over everything while open.
 */
class UIShopMenu : public UIElement
{
public:
    explicit UIShopMenu(std::shared_ptr<DataLoader> dataLoader);

    void open();
    void close();
    bool is_open() const { return m_isVisible; }

    void update(float deltaTime) override;
    void render(sf::RenderWindow& window) override;
    void handle_event(const sf::Event& event, const sf::RenderWindow& window) override;
    sf::FloatRect get_bounds() const override;   // The whole screen while open
    bool is_blocking_input() const override;

private:
    void buy_scales(int count);                   // Asks, then pays
    void flash(const std::string& message);       // A line under the counters for a moment ("+5 Pearls!")

    std::shared_ptr<DataLoader> m_dataLoader;
    UIPopupFrame m_frame;
    UIOfferButton m_adOffer;
    std::vector<std::unique_ptr<UIButtonElement>> m_packs;   // Not sold yet
    UIOfferButton m_oneScale;
    UIOfferButton m_tenScales;
    UIConfirmDialog m_confirm;
    UIAdOverlay m_ad;
    sf::Text m_text;                    // Reused for the labels
    std::string m_flash;
    float m_flashTime = 0.f;            // Counts down
    bool m_isPressedWhileOpen = false;  // A tap outside closes it only if it began while it was open
};
