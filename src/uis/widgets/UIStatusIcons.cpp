#include "UIStatusIcons.h"

#include "../../core/DataLoader.h"
#include "../../core/stages/Stage.h"
#include "../../core/entities/BattleEntity.h"

#include <cmath>
#include <iostream>

UIStatusIcons::UIStatusIcons(std::shared_ptr<Stage> stage, std::shared_ptr<DataLoader> dataLoader) :
    m_stage(stage),
    m_dataLoader(std::move(dataLoader))
{
    isWorldSpaceUi = true; // Drawn with the stage camera, under the entities

    m_isTextureLoaded = m_iconsTexture.loadFromFile(STATUS_ICONS_TEXTURE);
    if (!m_isTextureLoaded) std::cerr << "UIStatusIcons: can't load " << STATUS_ICONS_TEXTURE << "\n";
    m_iconsTexture.setSmooth(true); // Icons are drawn smaller than the sheet
}

void UIStatusIcons::update(float deltaTime)
{
    m_time += deltaTime;
}

void UIStatusIcons::render(sf::RenderWindow& window)
{
    if (!m_isVisible || !m_isTextureLoaded) return;

    auto stage = m_stage.lock();
    if (!stage || !stage->is_loaded()) return;

    for (const auto& [layer, units] : stage->get_units())
        for (const auto& unit : units) draw_row(window, *unit);

    for (const auto& [layer, enemies] : stage->get_enemies())
        for (const auto& enemy : enemies) draw_row(window, *enemy);
}

void UIStatusIcons::draw_row(sf::RenderWindow& window, const BattleEntity& entity)
{
    const auto& statuses = entity.get_statuses();
    if (statuses.empty() || entity.isDead) return;

    const int columns = std::max(1, static_cast<int>(m_iconsTexture.getSize().x) / STATUS_ICON_CELL_SIZE);
    const float scale = ICON_SIZE / STATUS_ICON_CELL_SIZE;

    // Centered under the sprite's feet (works for units and enemies, during knockback too)
    const sf::FloatRect bounds = entity.sprite.getGlobalBounds();
    const float rowWidth = statuses.size() * ICON_SIZE + (statuses.size() - 1) * ICON_GAP;
    sf::Vector2f position = {
        bounds.position.x + bounds.size.x / 2.f - rowWidth / 2.f,
        bounds.position.y + bounds.size.y + OFFSET_BELOW
    };

    for (const StatusEffect& status : statuses)
    {
        const auto data = m_dataLoader->get_status_data(status.type);
        if (data && data->iconIndex >= 0)
        {
            sf::Sprite icon(m_iconsTexture, sf::IntRect(
                { (data->iconIndex % columns) * STATUS_ICON_CELL_SIZE, (data->iconIndex / columns) * STATUS_ICON_CELL_SIZE },
                { STATUS_ICON_CELL_SIZE, STATUS_ICON_CELL_SIZE }));
            icon.setScale({ scale, scale });
            icon.setPosition(position);

            // Blinks during the last second, so the player sees it's about to end
            if (status.durationRemaining < 1.f)
            {
                const float alpha = 0.35f + 0.65f * std::abs(std::sin(m_time * 10.f));
                icon.setColor(sf::Color(255, 255, 255, static_cast<std::uint8_t>(255 * alpha)));
            }
            window.draw(icon);
        }
        position.x += ICON_SIZE + ICON_GAP;
    }
}
