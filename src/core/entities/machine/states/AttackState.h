#pragma once

#include "BaseState.h"

/*
 * AttackState
 * -----------
 * Handles the attack logic for a BattleEntity (unit or enemy).
 *
 * Responsibilities:
 *  - Manages foreswing (wind-up), attack execution, and backswing (recovery).
 *  - Finds valid targets in the entity's damage zone.
 *  - Deals damage according to attack type (single-target or area).
 *  - Transitions back to Idle or Walk after finishing the attack.
 *
 * State transitions:
 *  - If cooldown not ready -> switch to IdleState
 *  - If health <= 0 or knockback threshold reached -> switch to KnockbackState
 *  - If no targets in range -> switch to WalkState
 *  - Otherwise, execute attack and return to IdleState after animation
 */

class AttackState : public BaseState
{
public:
    explicit AttackState(std::shared_ptr<StateMachine> machine);

    void enter() override;                       // Called once when state starts
    void perform(float deltaTime) override;      // Handles attack timing and damage
    void exit() override;                        // Called once when state ends

    std::string get_state_id() const override;   // Always returns "ATTACK"

private:
    // Attack timing
    bool m_isAttackReady = false;                // True once foreswing is finished
    bool m_hasAttacked = false;                  // Prevents multiple hits per swing
    float m_currentAttackSwingTime = 0.0f;       // Tracks foreswing + backswing progress
};
