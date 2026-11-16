#include "UIEnergyCost.h"
#include "../UISkin.h"
#include "../../core/DataLoader.h"
#include "../../data/PlayerData.h"
#include "../../data/StageData.h"

namespace
{
    const float HEIGHT = 58.f;
    const float ICON_SIZE = 42.f;
    const float PADDING = 16.f;
    const sf::Color SHORT{ 255, 110, 100 };   // Not that much energy
    const sf::Color FREE{ 140, 236, 120 };
}

UIEnergyCost::UIEnergyCost(std::shared_ptr<DataLoader> dataLoader, std::function<int()> stageUid, sf::Vector2f center) :
    UIElement({ 0.f, HEIGHT }, center),
    m_dataLoader(std::move(dataLoader)),
    m_stageUid(std::move(stageUid)),
    m_center(center),
    m_text(UISkin::heavy_font())
{
    UISkin::style_text(m_text, 36, UISkin::NUMBER);
}

void UIEnergyCost::update(float deltaTime)
{
    auto player = m_dataLoader->get_player_data().lock();
    auto stage = m_stageUid ? m_dataLoader->get_stage_data(m_stageUid()) : nullptr;
    m_isVisible = player && stage;
    if (!m_isVisible) return;

    m_isFree = stage->energyCost <= 0;
    m_text.setString(m_isFree ? "FREE" : std::to_string(stage->energyCost));
    UISkin::set_text_color(m_text, m_isFree ? FREE : player->get_energy() < stage->energyCost ? SHORT : UISkin::NUMBER);

    // Centered on m_center: the icon (not for FREE), then the number
    const float textWidth = m_text.getLocalBounds().size.x;
    const float contentWidth = (m_isFree ? 0.f : ICON_SIZE + 6.f) + textWidth;
    m_size = { contentWidth + 2.f * PADDING, HEIGHT };
    m_position = m_center - m_size / 2.f;
    UISkin::set_text_anchor(m_text, { 1.f, 0.5f });
    m_text.setPosition({ m_position.x + m_size.x - PADDING, m_center.y - 2.f });
}

void UIEnergyCost::render(sf::RenderWindow& window)
{
    if (!m_isVisible) return;

    UISkin::draw_nine_slice(window, UISkin::PANEL_DARK, get_bounds(), 0.7f);
    if (!m_isFree)
        UISkin::draw_fitted(window, UISkin::texture("icon_energy"),
            { { m_position.x + PADDING, m_center.y - ICON_SIZE / 2.f }, { ICON_SIZE, ICON_SIZE } });
    window.draw(m_text);
}
