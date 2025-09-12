#pragma once

#include <SFML/System/Vector2.hpp>

#include "BattleEntity.h"

#include <memory>
#include <vector>

/*
 * BattleEnemy
 * -----------
 * Represents an enemy spawned during a stage.
 *
 * Responsibilities:
 *  - Initializes itself from EntityData (stats, sprite, attack range...)
 *  - Updates its state each frame (movement, attack, knockback...)
 *  - Handles position and hitbox updates
 *
 * Like BattleUnit, but positioned at the left side of the battlefield
 * and with slightly different sprite offset logic.
 */

class BattleEnemy : public BattleEntity
{
public:
    BattleEnemy(std::shared_ptr<EntityData> data_, sf::Vector2f magnification_);
    ~BattleEnemy();

    // ===== Overrides =====
    void update(float deltaTime, const std::map<int, std::vector<std::shared_ptr<BattleEntity>>>& entityList) override;

    void update_position() override;
};
