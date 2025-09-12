#pragma once

#include <SFML/System/Vector2.hpp>
#include "BattleEntity.h"

#include <memory>
#include <vector>

/*
 * BattleBase
 * ----------
 * Represents a base in a stage (either the player's base or the enemy's base).
 *
 * Responsibilities:
 *  - Tracks its own health (max + current).
 *  - Provides collision hitbox for when units attack it.
 *  - Displays a static sprite (no animation).
 *  - Updates its position for rendering and debug rectangles.
 *
 * Unlike units or enemies, bases:
 *  - Do not move
 *  - Do not attack
 *  - Only serve as a "target" to defend/destroy
 */

class BattleBase : public BattleEntity
{
public:
    // Construct a base with a given amount of health and a texture
    BattleBase(float health_, std::string texture_);

    // ===== Overrides =====
    void update(float deltaTime, const std::map<int, std::vector<std::shared_ptr<BattleEntity>>>& entityList) override;

    // Bases use a static sprite (no animation)
    void update_sprite() override {};

public:
    // Maximum health (used to reset or display a health bar)
    float maxHealth = 1.0f;

private:
    // Internal position updater (keeps sprite & hitboxes aligned)
    void update_position() override;
};
