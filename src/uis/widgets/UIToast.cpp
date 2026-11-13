#include "UIToast.h"
#include "../UISkin.h"
#include "../../data/PlayerData.h"

#include <algorithm>
#include <cmath>

namespace
{
    const float SHOWN = 2.6f;          // Seconds each one stays
    const float SLIDE = 0.25f;         // In and out
    const sf::Vector2f SIZE{ 720.f, 120.f };
    const float TOP = 120.f;           // Under the title bar
    const sf::Color TITLE_COLOR{ 205, 150, 255 };   // UISkin::EXTRA: the Clam Room's purple
}

UIToast::UIToast() :
    UIElement({ 0.f, 0.f }, { 0.f, 0.f })
{
}

std::deque<UIToast::News>& UIToast::queue()
{
    static std::deque<News> news;
    return news;
}

void UIToast::show(const std::string& title, const std::string& line, const std::string& icon)
{
    queue().push_back({ title, line, icon });
}

void UIToast::announce_shoal_rewards(PlayerData& player)
{
    const int scales = player.claim_shoal_rewards();
    if (scales <= 0) return;
    show("SHOAL POWER " + std::to_string(player.get_shoal_power()) + "!",
        "+" + std::to_string(scales) + " Shiny Scale" + (scales > 1 ? "s" : "") + " for the Clam Room", "icon_shiny_scale");
}

void UIToast::update(float deltaTime)
{
    if (queue().empty()) { m_time = 0.f; return; }
    if (!m_hasSounded)
    {
        m_hasSounded = true;
        play_sound("unlock");
    }
    m_time += deltaTime;
    if (m_time >= SHOWN)
    {
        queue().pop_front();
        m_time = 0.f;
        m_hasSounded = false;
    }
}

void UIToast::render(sf::RenderWindow& window)
{
    if (queue().empty()) return;
    const News& news = queue().front();

    // Slides down, waits, slides back up
    const float in = std::clamp(m_time / SLIDE, 0.f, 1.f), out = std::clamp((SHOWN - m_time) / SLIDE, 0.f, 1.f);
    const float shown = std::min(in, out);
    const float eased = 1.f - (1.f - shown) * (1.f - shown);
    const sf::FloatRect box{ { (1920.f - SIZE.x) / 2.f, TOP - (1.f - eased) * (SIZE.y + TOP) }, SIZE };
    UISkin::draw_nine_slice(window, UISkin::PANEL_DARK, box, 1.f);

    float textLeft = box.position.x + 30.f;
    if (!news.icon.empty())
    {
        UISkin::draw_fitted(window, UISkin::texture(news.icon), { { box.position.x + 24.f, box.position.y + 20.f }, { 80.f, 80.f } });
        textLeft += 96.f;
    }
    sf::Text text(UISkin::heavy_font(), news.title);
    UISkin::style_text(text, 40, TITLE_COLOR);
    UISkin::set_text_anchor(text, { 0.f, 0.5f });
    text.setPosition({ textLeft, box.position.y + 40.f });
    window.draw(text);
    text.setString(news.line);
    UISkin::style_text(text, 28, UISkin::TEXT);
    UISkin::set_text_anchor(text, { 0.f, 0.5f });
    text.setPosition({ textLeft, box.position.y + 84.f });
    window.draw(text);
}
