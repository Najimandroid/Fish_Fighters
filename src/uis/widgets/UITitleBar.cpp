#include "UITitleBar.h"
#include "../UISkin.h"

namespace
{
    const float SCREEN_WIDTH = 1920.f;
    const float OVERHANG = 12.f;          // The bar starts above and beside the screen: only its bottom edge shows
    const unsigned int TITLE_SIZE = 64;
    const float TITLE_X = 40.f;
    const float BAR_IMAGE_HEIGHT = 112.f;  // bar_metal.png: drawn at the scale that fits its height
}

UITitleBar::UITitleBar(const std::string& title) :
    UIElement({ SCREEN_WIDTH, HEIGHT }, { 0.f, 0.f }),
    m_title(UISkin::heavy_font(), title)
{
    UISkin::style_text(m_title, TITLE_SIZE, UISkin::TITLE);
    set_title(title);
}

void UITitleBar::set_title(const std::string& title)
{
    m_title.setString(title);
    UISkin::set_text_anchor(m_title, { 0.f, 0.5f });
    m_title.setPosition({ TITLE_X, HEIGHT / 2.f - 4.f });
}

void UITitleBar::render(sf::RenderWindow& window)
{
    if (!m_isVisible) return;

    const sf::FloatRect bar{ { -OVERHANG, -OVERHANG }, { SCREEN_WIDTH + 2.f * OVERHANG, HEIGHT + OVERHANG } };
    UISkin::draw_nine_slice(window, UISkin::BAR_METAL, bar, bar.size.y / BAR_IMAGE_HEIGHT);
    window.draw(m_title);
}
