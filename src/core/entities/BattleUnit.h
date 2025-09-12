#pragma once

#include <SFML/System/Vector2.hpp>
#include <SFML/Graphics/Rect.hpp>

#include "BattleEntity.h"

#include <memory>
#include <vector>

/*
 * BattleUnit
 * ----------
 * Represents a unit that has been spawned on the battlefield.
 *
 * Responsibilities:
 *  - Initializes itself from EntityData (stats, sprite, attack range...)
 *  - Updates its state each frame (movement, attack, knockback...)
 *  - Handles position and hitbox updates
 *
 * Derived from BattleEntity, so it shares animation, targeting,
 * and state machine logic with enemies.
 */

class BattleUnit : public BattleEntity
{
public:
    BattleUnit(std::shared_ptr<EntityData> data_, sf::Vector2f magnification_);
    ~BattleUnit();

    // ===== Overrides =====
    void update(float deltaTime, const std::map<int, std::vector<std::shared_ptr<BattleEntity>>>& entityList) override;

    void update_position() override;
};
