#include "UIPanel.h"

UIPanel::UIPanel(sf::Vector2f size, sf::Vector2f position, const UISkin::NineSlice& look, float cornerScale) :
    UIElement(size, position),
    m_look(look),
    m_cornerScale(cornerScale)
{
}

void UIPanel::set_tint(sf::Color tint)
{
    m_tint = tint;
}

void UIPanel::render(sf::RenderWindow& window)
{
    if (!m_isVisible) return;
    UISkin::draw_nine_slice(window, m_look, get_bounds(), m_cornerScale, m_tint);
}
