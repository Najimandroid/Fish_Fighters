#pragma once

#include "UIElement.h"

class Stage;
class BattleBase;

/*
 * UIBaseHealthInfo
 * ----------------
 * A health bar over each base, in world space: teal for the player's fish bowl, coral for the enemy
 * base, with "current / max" inside. A hit flashes the bar white and leaves a yellow chunk that drains
 * after it, so the damage can be seen at a glance.
 */
class UIBaseHealthInfo : public UIElement
{
public:
    UIBaseHealthInfo(std::shared_ptr<Stage> stage);

    // Follows the health (the flash and the draining chunk)
    void update(float deltaTime) override;

    void render(sf::RenderWindow& window) override;

private:
    struct Bar
    {
        float shown = -1.f;          // Health the yellow chunk drains from (-1: not set yet)
        float lastHealth = -1.f;
        float flash = 0.f;           // Seconds of white flash left
        float drainDelay = 0.f;      // Seconds before the chunk starts draining
    };

    void update_bar(Bar& bar, const BattleBase& base, float deltaTime);
    void render_bar(sf::RenderWindow& window, const Bar& bar, const BattleBase& base, sf::Color fill);

    std::weak_ptr<Stage> m_stage;
    Bar m_unitBar;
    Bar m_enemyBar;
    sf::Text m_text;
};
