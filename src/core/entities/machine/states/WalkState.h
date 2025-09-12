#pragma once

#include "BaseState.h"

/*
 * WalkState
 * ---------
 * Handles movement of a BattleEntity while searching for enemies.
 *
 * Responsibilities:
 *  - Move forward each frame in the correct direction (units -> left, enemies -> right).
 *  - Continuously scan for targets in attack range.
 *  - Transition to the correct state depending on conditions:
 *      - Health <= 0.0f or knockback threshold -> KnockbackState
 *      - Enemy in range and cooldown ready -> AttackState
 *      - Enemy in range but cooldown not ready -> IdleState
 *      - Otherwise -> keep walking
 */

class WalkState : public BaseState
{
public:
    explicit WalkState(std::shared_ptr<StateMachine> machine);

    void enter() override;                       // Called once when state starts
    void perform(float deltaTime) override;      // Handles movement and transitions
    void exit() override;                        // Called once when state ends

    std::string get_state_id() const override;   // Always returns "WALK"

private:
    bool m_areEntitiesOnRange = false;           // Cache: whether an enemy is in attack range
};
