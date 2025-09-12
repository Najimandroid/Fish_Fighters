#pragma once

#include "BaseState.h"

/*
 * IdleState
 * ---------
 * Represents a waiting state for a BattleEntity.
 *
 * Responsibilities:
 *  - Stand still when there are no immediate actions.
 *  - Scan surroundings for enemies in attack range.
 *  - Transition to the correct state depending on conditions:
 *      - Health <= 0.0f or knockback threshold -> KnockbackState
 *      - Enemy target available and cooldown ready -> AttackState
 *      - No enemies in range -> WalkState
 *      - Otherwise -> remain Idle
 */

class IdleState : public BaseState
{
public:
    explicit IdleState(std::shared_ptr<StateMachine> machine);

    void enter() override;                       // Called when entering idle
    void perform(float deltaTime) override;      // Main idle behavior logic
    void exit() override;                        // Called when leaving idle

    std::string get_state_id() const override;   // Always returns "IDLE"

private:
    bool m_areEntitiesOnRange = false;           // Cache: whether enemies are nearby
};
