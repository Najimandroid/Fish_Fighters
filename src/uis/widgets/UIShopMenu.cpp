#include "UIShopMenu.h"
#include "../UISkin.h"
#include "../../core/DataLoader.h"
#include "../../core/Platform.h"
#include "../../data/PlayerData.h"

#include <algorithm>

namespace
{
    const sf::FloatRect WINDOW{ { 340.f, 130.f }, { 1240.f, 820.f } };
    const float LEFT = WINDOW.position.x + 60.f;                  // The rows' names
    const float RIGHT = WINDOW.position.x + WINDOW.size.x - 60.f; // Their offers end here
    const float HAVE_Y = WINDOW.position.y + UIPopupFrame::HEADER + 52.f;   // What the player has
    const float ROWS_TOP = WINDOW.position.y + 230.f;
    const float ROW_HEIGHT = 190.f;
    const sf::Vector2f OFFER{ 300.f, 120.f };
    const sf::Vector2f PACK{ 210.f, 80.f };
    const float FLASH_SECONDS = 2.2f;

    // The packs: Pearls and their price (not sold yet)
    struct Pack { int pearls; const char* price; };
    const Pack PACKS[] = { { 30, "$0.99" }, { 110, "$2.99" }, { 300, "$6.99" } };

    float row_top(int row) { return ROWS_TOP + ROW_HEIGHT * static_cast<float>(row); }
}

UIShopMenu::UIShopMenu(std::shared_ptr<DataLoader> dataLoader) :
    UIElement({ 1920.f, 1080.f }, { 0.f, 0.f }),
    m_dataLoader(std::move(dataLoader)),
    m_frame(WINDOW, "Pearl Shop"),
    m_adOffer(OFFER, { RIGHT - OFFER.x, row_top(0) + 20.f }, "+" + std::to_string(PlayerData::PEARLS_PER_AD), UIOfferButton::Kind::REWARDED_AD),
    m_oneScale(OFFER, { RIGHT - 2.f * OFFER.x - 30.f, row_top(2) + 20.f }, "1 SCALE", UIOfferButton::Kind::CURRENCY, "icon_pearl", PlayerData::PEARLS_FOR_SCALE),
    m_tenScales(OFFER, { RIGHT - OFFER.x, row_top(2) + 20.f }, "10 SCALES", UIOfferButton::Kind::CURRENCY, "icon_pearl", PlayerData::PEARLS_FOR_TEN_SCALES),
    m_confirm("", "", "BUY", "CANCEL"),
    m_text(UISkin::heavy_font())
{
    m_frame.set_on_close([this]() { close(); });

    // Free Pearls: a video (phones)
    m_adOffer.set_text_size(44);
    m_adOffer.set_click_sound("");
    m_adOffer.set_callback([this]()
        {
            if (m_ad.is_playing()) return;
            m_ad.play([this]()
                {
                    auto player = m_dataLoader->get_player_data().lock();
                    if (!player || !player->watch_shop_ad()) return;
                    m_dataLoader->save_progress();
                    play_sound("purchase");
                    flash("+" + std::to_string(PlayerData::PEARLS_PER_AD) + " Pearls! Thanks for watching");
                });
        });

    // The packs: their price, not sold yet
    for (size_t i = 0; i < std::size(PACKS); ++i)
    {
        const float x = RIGHT - (std::size(PACKS) - i) * (PACK.x + 24.f) + 24.f;
        auto button = std::make_unique<UIButtonElement>(PACK, sf::Vector2f{ x, row_top(1) + 78.f }, PACKS[i].price);
        button->set_text_size(34);
        button->set_enabled(false);
        m_packs.push_back(std::move(button));
    }

    // Shiny Scales for Pearls
    m_oneScale.set_text_size(40);
    m_tenScales.set_text_size(40);
    m_oneScale.set_click_sound("");
    m_tenScales.set_click_sound("");
    m_oneScale.set_callback([this]() { buy_scales(1); });
    m_tenScales.set_callback([this]() { buy_scales(10); });
    m_confirm.set_confirm_tint(sf::Color::White);   // A purchase: yellow
    m_confirm.set_confirm_sound("");

    m_isVisible = false;
}

void UIShopMenu::open()
{
    m_isVisible = true;
    m_isPressedWhileOpen = false;
    m_flashTime = 0.f;
}

void UIShopMenu::close()
{
    m_isVisible = false;
    m_confirm.close();
}

void UIShopMenu::buy_scales(int count)
{
    auto player = m_dataLoader->get_player_data().lock();
    if (!player) return;
    const int price = count >= 10 ? PlayerData::PEARLS_FOR_TEN_SCALES : PlayerData::PEARLS_FOR_SCALE * count;
    if (player->pearls < price)
    {
        play_sound("ui_error");
        flash("Not enough Pearls");
        return;
    }
    const std::string what = count == 1 ? "1 Shiny Scale" : std::to_string(count) + " Shiny Scales";
    m_confirm.set_texts("Buy " + what + "?", "For " + std::to_string(price) + " Pearls.", "BUY");
    m_confirm.open([this, count, what]()
        {
            auto player = m_dataLoader->get_player_data().lock();
            if (!player || !player->buy_shiny_scales(count)) return;
            m_dataLoader->save_progress();
            play_sound("purchase");
            flash("+" + what + "!");
        });
}

void UIShopMenu::flash(const std::string& message)
{
    m_flash = message;
    m_flashTime = FLASH_SECONDS;
}

void UIShopMenu::update(float deltaTime)
{
    m_ad.update(deltaTime);
    if (!m_isVisible) return;
    m_frame.update(deltaTime);
    m_flashTime = std::max(0.f, m_flashTime - deltaTime);

    auto player = m_dataLoader->get_player_data().lock();
    const int pearls = player ? player->pearls : 0;
    m_adOffer.set_enabled(Platform::HAS_REWARDED_ADS && player && player->shop_ads_left_today() > 0);
    m_oneScale.set_enabled(pearls >= PlayerData::PEARLS_FOR_SCALE);
    m_tenScales.set_enabled(pearls >= PlayerData::PEARLS_FOR_TEN_SCALES);
    m_adOffer.update(deltaTime);
    m_oneScale.update(deltaTime);
    m_tenScales.update(deltaTime);
    for (auto& pack : m_packs) pack->update(deltaTime);
    m_confirm.update(deltaTime);
}

void UIShopMenu::render(sf::RenderWindow& window)
{
    if (!m_isVisible)
    {
        m_ad.render(window);   // (Nothing when not playing)
        return;
    }
    sf::RectangleShape dim({ 1920.f, 1080.f });
    dim.setFillColor(sf::Color(0, 0, 0, 150));
    window.draw(dim);
    m_frame.render(window);

    auto player = m_dataLoader->get_player_data().lock();
    auto drawText = [&](const std::string& text, unsigned int size, sf::Color color, sf::Vector2f position, sf::Vector2f anchor,
        const sf::Font& font = UISkin::heavy_font())
        {
            UISkin::style_text(m_text, size, color, font);
            m_text.setString(sf::String::fromUtf8(text.begin(), text.end()));
            UISkin::set_text_anchor(m_text, anchor);
            m_text.setPosition(position);
            window.draw(m_text);
            return m_text.getGlobalBounds();
        };
    auto drawIcon = [&](const char* name, sf::Vector2f center, float size)
        {
            UISkin::draw_fitted(window, UISkin::texture(name), { center - sf::Vector2f{ size, size } / 2.f, { size, size } });
        };

    // What the player has: their Pearls and Shiny Scales, and what just happened under them
    const float middle = WINDOW.position.x + WINDOW.size.x / 2.f;
    drawIcon("icon_pearl", { middle - 250.f, HAVE_Y }, 56.f);
    drawText(std::to_string(player ? player->pearls : 0), 46, UISkin::NUMBER, { middle - 210.f, HAVE_Y }, { 0.f, 0.5f });
    drawIcon("icon_shiny_scale", { middle + 90.f, HAVE_Y }, 56.f);
    drawText(std::to_string(player ? player->shinyScales : 0), 46, UISkin::NUMBER, { middle + 130.f, HAVE_Y }, { 0.f, 0.5f });
    if (m_flashTime > 0.f)
    {
        const float alpha = std::min(1.f, m_flashTime / 0.4f);
        const sf::Color color(110, 220, 120, static_cast<std::uint8_t>(255 * alpha));
        drawText(m_flash, 30, color, { middle, HAVE_Y + 50.f }, { 0.5f, 0.5f });
    }

    // The rows: a line between them, their name and what they are on the left, the offers on the right
    for (int row = 0; row < 3; ++row)
    {
        sf::RectangleShape line({ WINDOW.size.x - 120.f, 3.f });
        line.setPosition({ LEFT, row_top(row) - 6.f });
        line.setFillColor(sf::Color(255, 255, 255, 40));
        window.draw(line);
    }
    const sf::Color body = UISkin::TEXT, faded(180, 190, 205);

    // Free Pearls
    drawText("FREE PEARLS", 36, UISkin::NUMBER, { LEFT, row_top(0) + 44.f }, { 0.f, 0.5f });
    if constexpr (Platform::HAS_REWARDED_ADS)
    {
        drawText("Watch a short video: " + std::to_string(PlayerData::PEARLS_PER_AD) + " Pearls.", 28, body, { LEFT, row_top(0) + 92.f }, { 0.f, 0.5f }, UISkin::regular_font());
        const int left = player ? player->shop_ads_left_today() : 0;
        drawText(left > 0 ? std::to_string(left) + " left today" : "No more today: come back tomorrow!", 28,
            left > 0 ? faded : sf::Color(255, 128, 104), { LEFT, row_top(0) + 130.f }, { 0.f, 0.5f }, UISkin::regular_font());
    }
    else
        drawText("On phones: a short video gives " + std::to_string(PlayerData::PEARLS_PER_AD) + " Pearls, 3 times a day.", 28, faded,
            { LEFT, row_top(0) + 92.f }, { 0.f, 0.5f }, UISkin::regular_font());
    m_adOffer.render(window);

    // The packs (not sold yet)
    drawText("PEARL PACKS", 36, UISkin::NUMBER, { LEFT, row_top(1) + 44.f }, { 0.f, 0.5f });
    drawText("Coming soon!", 28, faded, { LEFT, row_top(1) + 92.f }, { 0.f, 0.5f }, UISkin::regular_font());
    for (size_t i = 0; i < m_packs.size(); ++i)
    {
        const sf::FloatRect button = m_packs[i]->get_bounds();
        const float centerX = button.position.x + button.size.x / 2.f;
        drawIcon("icon_pearl", { centerX - 40.f, row_top(1) + 40.f }, 46.f);
        drawText(std::to_string(PACKS[i].pearls), 40, UISkin::NUMBER, { centerX - 10.f, row_top(1) + 40.f }, { 0.f, 0.5f });
        m_packs[i]->render(window);
    }

    // Shiny Scales for Pearls
    drawText("SHINY SCALES", 36, UISkin::NUMBER, { LEFT, row_top(2) + 44.f }, { 0.f, 0.5f });
    drawText("To feed the giant clam.", 28, body, { LEFT, row_top(2) + 92.f }, { 0.f, 0.5f }, UISkin::regular_font());
    drawText("10 for the price of 9!", 28, sf::Color(110, 220, 120), { LEFT, row_top(2) + 130.f }, { 0.f, 0.5f }, UISkin::regular_font());
    m_oneScale.render(window);
    m_tenScales.render(window);

    m_confirm.render(window);
    m_ad.render(window);
}

void UIShopMenu::handle_event(const sf::Event& event, const sf::RenderWindow& window)
{
    if (m_ad.is_playing()) return;   // Nothing goes through while it plays
    if (!m_isVisible) return;
    if (m_confirm.is_blocking_input())
    {
        m_confirm.handle_event(event, window);
        return;
    }
    m_frame.handle_event(event, window);
    if (!m_isVisible) return;
    m_adOffer.handle_event(event, window);
    m_oneScale.handle_event(event, window);
    m_tenScales.handle_event(event, window);
    for (auto& pack : m_packs) pack->handle_event(event, window);

    if (const auto* key = event.getIf<sf::Event::KeyPressed>(); key && key->code == sf::Keyboard::Key::Escape)
    {
        play_sound("ui_back");
        close();
    }
    else if (event.is<sf::Event::MouseButtonPressed>())
        m_isPressedWhileOpen = true;
    else if (const auto* released = event.getIf<sf::Event::MouseButtonReleased>())
    {
        // A tap outside the window closes it (only a tap begun while it's open)
        const sf::Vector2f point = window.mapPixelToCoords(released->position);
        const bool wasPressed = m_isPressedWhileOpen;
        m_isPressedWhileOpen = false;
        if (wasPressed && !WINDOW.contains(point) && !m_frame.get_close_bounds().contains(point))
        {
            play_sound("ui_back");
            close();
        }
    }
}

sf::FloatRect UIShopMenu::get_bounds() const
{
    return m_isVisible || m_ad.is_playing() ? sf::FloatRect{ { 0.f, 0.f }, { 1920.f, 1080.f } } : sf::FloatRect{};
}

bool UIShopMenu::is_blocking_input() const
{
    return m_isVisible || m_ad.is_playing();
}
