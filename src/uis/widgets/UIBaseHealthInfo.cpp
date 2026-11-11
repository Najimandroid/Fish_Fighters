#include "UIBaseHealthInfo.h"
#include "../UISkin.h"
#include "../../core/stages/Stage.h"

#include <algorithm>
#include <cmath>

namespace
{
    const sf::Vector2f BAR_SIZE{ 260.f, 26.f };
    const float GAP_ABOVE_BASE = 14.f;                  // Between the top of the base image and the bar
    const float OUTLINE = 4.f;
    const sf::Color TRACK{ 16, 24, 34, 210 };
    const sf::Color PLAYER_FILL{ 60, 200, 220 };        // Teal: the fish bowl
    const sf::Color ENEMY_FILL{ 240, 96, 72 };          // Coral: the enemy base
    const sf::Color CHUNK{ 255, 222, 48 };              // What a hit just took, before it drains
    const unsigned int TEXT_SIZE = 22;
    const float FLASH_SECONDS = 0.08f;
    const float DRAIN_DELAY = 0.15f, DRAIN_SECONDS = 0.4f;
}

UIBaseHealthInfo::UIBaseHealthInfo(std::shared_ptr<Stage> stage) :
    m_stage(stage),
    m_text(UISkin::heavy_font())
{
    isWorldSpaceUi = true; // Drawn with the battlefield camera, over the bases
    UISkin::style_text(m_text, TEXT_SIZE);
}

void UIBaseHealthInfo::update_bar(Bar& bar, const BattleBase& base, float deltaTime)
{
    const float health = std::max(0.f, base.currentHealth);
    if (bar.shown < 0.f) bar.shown = bar.lastHealth = health;

    if (health < bar.lastHealth)
    {
        bar.flash = FLASH_SECONDS;
        bar.drainDelay = DRAIN_DELAY;
    }
    bar.lastHealth = health;

    bar.flash = std::max(0.f, bar.flash - deltaTime);
    if (bar.drainDelay > 0.f) bar.drainDelay -= deltaTime;
    else if (bar.shown > health)
        bar.shown = std::max(health, bar.shown - base.maxHealth * deltaTime / DRAIN_SECONDS);
    if (bar.shown < health) bar.shown = health;   // Healing
}

void UIBaseHealthInfo::update(float deltaTime)
{
    if (auto stage = m_stage.lock())
    {
        if (auto enemyBase = stage->get_enemy_base().lock()) update_bar(m_enemyBar, *enemyBase, deltaTime);
        if (auto unitBase = stage->get_unit_base().lock()) update_bar(m_unitBar, *unitBase, deltaTime);
    }
}

void UIBaseHealthInfo::render_bar(sf::RenderWindow& window, const Bar& bar, const BattleBase& base, sf::Color fill)
{
    const sf::FloatRect baseBounds = base.sprite.getGlobalBounds();
    const sf::Vector2f position{ baseBounds.position.x + (baseBounds.size.x - BAR_SIZE.x) / 2.f,
        baseBounds.position.y - GAP_ABOVE_BASE - BAR_SIZE.y };
    const float max = std::max(1.f, base.maxHealth);
    const float health = std::clamp(base.currentHealth, 0.f, max);

    sf::RectangleShape track(BAR_SIZE);
    track.setPosition(position);
    track.setFillColor(TRACK);
    track.setOutlineThickness(OUTLINE);
    track.setOutlineColor(UISkin::OUTLINE);
    window.draw(track);

    // The chunk a hit just took (yellow), then the health left (white while it flashes)
    sf::RectangleShape chunk({ BAR_SIZE.x * std::clamp(bar.shown, 0.f, max) / max, BAR_SIZE.y });
    chunk.setPosition(position);
    chunk.setFillColor(CHUNK);
    window.draw(chunk);

    sf::RectangleShape healthFill({ BAR_SIZE.x * health / max, BAR_SIZE.y });
    healthFill.setPosition(position);
    healthFill.setFillColor(bar.flash > 0.f ? sf::Color::White : fill);
    window.draw(healthFill);

    m_text.setString(std::to_string(static_cast<int>(std::ceil(health))) + " / " + std::to_string(static_cast<int>(max)));
    UISkin::set_text_anchor(m_text, { 0.5f, 0.5f });
    m_text.setPosition(position + BAR_SIZE / 2.f);
    window.draw(m_text);
}

void UIBaseHealthInfo::render(sf::RenderWindow& window)
{
    if (!m_isVisible) return;
    auto stage = m_stage.lock();
    if (!stage) return;

    if (auto enemyBase = stage->get_enemy_base().lock()) render_bar(window, m_enemyBar, *enemyBase, ENEMY_FILL);
    if (auto unitBase = stage->get_unit_base().lock()) render_bar(window, m_unitBar, *unitBase, PLAYER_FILL);
}
