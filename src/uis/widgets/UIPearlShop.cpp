#include "UIPearlShop.h"
#include "UIConfirmDialog.h"
#include "UIAdOverlay.h"
#include "UIToast.h"
#include "../UISkin.h"
#include "../../core/DataLoader.h"
#include "../../core/Platform.h"
#include "../../data/PlayerData.h"

#include <algorithm>

namespace
{
    const sf::Vector2f CARD{ 340.f, 320.f };
    const float CARD_GAP = 40.f;
    const float ROW_PEARLS = 196.f, ROW_SCALES = 626.f;        // The rows' tops (their titles above them)
    const float ROWS_LEFT = (1920.f - 4.f * CARD.x - 3.f * CARD_GAP) / 2.f;
    const sf::Vector2f BUTTON{ 300.f, 92.f };
    const sf::Color FADED{ 180, 190, 205 }, GOOD{ 110, 220, 120 }, CORAL{ 255, 128, 104 };

    struct Pack { int pearls; const char* price; int pile; };
    const Pack PACKS[] = { { 30, "$0.99", 2 }, { 110, "$2.99", 3 }, { 300, "$6.99", 5 } };

    sf::FloatRect card_at(float top, int index)
    {
        return { { ROWS_LEFT + index * (CARD.x + CARD_GAP), top }, CARD };
    }
    sf::Vector2f button_at(const sf::FloatRect& card)
    {
        return { card.position.x + (card.size.x - BUTTON.x) / 2.f, card.position.y + card.size.y - BUTTON.y - 14.f };
    }
}

UIPearlShop::UIPearlShop(std::shared_ptr<DataLoader> dataLoader, std::weak_ptr<UIConfirmDialog> confirm, std::weak_ptr<UIAdOverlay> ad) :
    UIElement({ 1920.f, 1080.f }, { 0.f, 0.f }),
    m_dataLoader(std::move(dataLoader)),
    m_confirm(std::move(confirm)),
    m_ad(std::move(ad)),
    m_text(UISkin::heavy_font())
{
    // Free Pearls: a short video (phones)
    {
        Card card;
        card.rect = card_at(ROW_PEARLS, 0);
        card.title = std::to_string(PlayerData::PEARLS_PER_AD) + " PEARLS";
        auto button = std::make_unique<UIOfferButton>(BUTTON, button_at(card.rect), "FREE", UIOfferButton::Kind::REWARDED_AD);
        button->set_text_size(40);
        button->set_click_sound("");
        button->set_callback([this]()
            {
                auto ad = m_ad.lock();
                if (!ad || ad->is_playing()) return;
                ad->play([this]()
                    {
                        auto player = m_dataLoader->get_player_data().lock();
                        if (!player || !player->watch_shop_ad()) return;
                        m_dataLoader->save_progress();
                        play_sound("purchase");
                        UIToast::show("+" + std::to_string(PlayerData::PEARLS_PER_AD) + " PEARLS", "Thanks for watching!", "icon_pearl");
                    });
            });
        card.button = std::move(button);
        m_cards.push_back(std::move(card));
    }

    // The packs: their price, not sold yet
    for (int i = 0; i < static_cast<int>(std::size(PACKS)); ++i)
    {
        Card card;
        card.rect = card_at(ROW_PEARLS, i + 1);
        card.title = std::to_string(PACKS[i].pearls) + " PEARLS";
        card.iconCount = PACKS[i].pile;
        card.line = "Coming soon!";
        card.lineColor = FADED;
        auto button = std::make_unique<UIButtonElement>(BUTTON, button_at(card.rect), PACKS[i].price);
        button->set_text_size(40);
        button->set_enabled(false);
        card.button = std::move(button);
        m_cards.push_back(std::move(card));
    }

    // Shiny Scales for Pearls
    for (const int count : { 1, 10 })
    {
        Card card;
        card.rect = card_at(ROW_SCALES, count == 1 ? 0 : 1);
        card.title = count == 1 ? "1 SHINY SCALE" : "10 SHINY SCALES";
        card.icon = "icon_shiny_scale";
        card.iconCount = count == 1 ? 1 : 4;
        card.line = count == 1 ? "To feed the giant clam" : "10 for the price of 9!";
        card.lineColor = count == 1 ? FADED : GOOD;
        auto button = std::make_unique<UIOfferButton>(BUTTON, button_at(card.rect), "BUY", UIOfferButton::Kind::CURRENCY, "icon_pearl",
            count == 1 ? PlayerData::PEARLS_FOR_SCALE : PlayerData::PEARLS_FOR_TEN_SCALES);
        button->set_text_size(36);
        button->set_click_sound("");
        button->set_callback([this, count]() { buy_scales(count); });
        card.button = std::move(button);
        m_cards.push_back(std::move(card));
    }
}

void UIPearlShop::buy_scales(int count)
{
    auto player = m_dataLoader->get_player_data().lock();
    auto confirm = m_confirm.lock();
    if (!player || !confirm) return;
    const int price = count >= 10 ? PlayerData::PEARLS_FOR_TEN_SCALES : PlayerData::PEARLS_FOR_SCALE * count;
    if (player->pearls < price)
    {
        play_sound("ui_error");
        return;
    }
    const std::string what = count == 1 ? "1 Shiny Scale" : std::to_string(count) + " Shiny Scales";
    confirm->set_texts("Buy " + what + "?", "For " + std::to_string(price) + " Pearls.", "BUY");
    confirm->open([this, count, what]()
        {
            auto player = m_dataLoader->get_player_data().lock();
            if (!player || !player->buy_shiny_scales(count)) return;
            m_dataLoader->save_progress();
            play_sound("purchase");
            UIToast::show("+" + std::string(count == 1 ? "1 SHINY SCALE" : std::to_string(count) + " SHINY SCALES"), "To feed the giant clam", "icon_shiny_scale");
        });
}

void UIPearlShop::update(float deltaTime)
{
    auto player = m_dataLoader->get_player_data().lock();
    const int pearls = player ? player->pearls : 0;
    // The free one: on phones, while today's videos last
    const int adsLeft = player ? player->shop_ads_left_today() : 0;
    m_cards[0].line = !Platform::HAS_REWARDED_ADS ? "On phones" : adsLeft > 0 ? std::to_string(adsLeft) + " left today" : "Come back tomorrow!";
    m_cards[0].lineColor = Platform::HAS_REWARDED_ADS && adsLeft > 0 ? GOOD : Platform::HAS_REWARDED_ADS ? CORAL : FADED;
    m_cards[0].button->set_enabled(Platform::HAS_REWARDED_ADS && adsLeft > 0);
    m_cards[4].button->set_enabled(pearls >= PlayerData::PEARLS_FOR_SCALE);
    m_cards[5].button->set_enabled(pearls >= PlayerData::PEARLS_FOR_TEN_SCALES);
    for (auto& card : m_cards) card.button->update(deltaTime);
}

void UIPearlShop::render(sf::RenderWindow& window)
{
    auto drawText = [&](const std::string& text, unsigned int size, sf::Color color, sf::Vector2f position, sf::Vector2f anchor,
        const sf::Font& font = UISkin::heavy_font())
        {
            UISkin::style_text(m_text, size, color, font);
            m_text.setString(sf::String::fromUtf8(text.begin(), text.end()));
            UISkin::set_text_anchor(m_text, anchor);
            m_text.setPosition(position);
            window.draw(m_text);
        };

    drawText("PEARLS", 40, UISkin::TITLE, { ROWS_LEFT, ROW_PEARLS - 30.f }, { 0.f, 0.5f });
    drawText("SHINY SCALES", 40, UISkin::TITLE, { ROWS_LEFT, ROW_SCALES - 30.f }, { 0.f, 0.5f });

    for (auto& card : m_cards)
    {
        const sf::FloatRect& r = card.rect;
        UISkin::draw_nine_slice(window, UISkin::PANEL_DARK, r, 1.f);
        drawText(card.title, 34, UISkin::NUMBER, { r.position.x + r.size.x / 2.f, r.position.y + 34.f }, { 0.5f, 0.5f });

        // The picture: a light slot, the icon (a pile of them for the bigger packs)
        const sf::FloatRect slot{ { r.position.x + 20.f, r.position.y + 62.f }, { r.size.x - 40.f, 110.f } };
        UISkin::draw_nine_slice(window, UISkin::SLOT, slot, 0.8f);
        const float iconSize = card.iconCount > 3 ? 70.f : 84.f;
        const float step = iconSize * 0.62f;
        const float startX = slot.position.x + slot.size.x / 2.f - step * static_cast<float>(card.iconCount - 1) / 2.f;
        for (int i = 0; i < card.iconCount; ++i)
        {
            const float bob = (i % 2 == 0) ? 0.f : -12.f;
            const sf::Vector2f center{ startX + step * static_cast<float>(i), slot.position.y + slot.size.y / 2.f + bob + 4.f };
            UISkin::draw_fitted(window, UISkin::texture(card.icon), { center - sf::Vector2f{ iconSize, iconSize } / 2.f, { iconSize, iconSize } });
        }
        UISkin::draw_nine_slice(window, UISkin::SLOT_FRAME, slot, 0.8f);

        if (!card.line.empty())
            drawText(card.line, 26, card.lineColor, { r.position.x + r.size.x / 2.f, r.position.y + 192.f }, { 0.5f, 0.5f }, UISkin::regular_font());
        card.button->render(window);
    }
}

void UIPearlShop::handle_event(const sf::Event& event, const sf::RenderWindow& window)
{
    for (auto& card : m_cards) card.button->handle_event(event, window);
}
