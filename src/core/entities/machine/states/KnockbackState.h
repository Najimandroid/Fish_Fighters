#pragma once

#include "BaseState.h"

/*
 * KnockbackState
 * --------------
 * Represents when a BattleEntity is knocked back due to damage.
 *
 * Responsibilities:
 *  - Plays a knockback tween animation (X displacement + Y bounce).
 *  - Temporarily disables attacking/movement until the knockback ends.
 *  - Transitions:
 *      - If health <= 0.0f after knockback -> DeadState
 *      - Otherwise -> IdleState
 *
 * Extra:
 *  - Handles "boss shockwave" knockbacks that skip normal knockback tracking.
 */

class KnockbackState : public BaseState
{
public:
    explicit KnockbackState(std::shared_ptr<StateMachine> machine);

    void enter() override;                       // Setup knockback tween
    void perform(float deltaTime) override;      // Count down duration, then transition
    void exit() override;                        // Adjust knockback threshold or clear shockwave

    std::string get_state_id() const override;   // Always returns "KNOCKBACK"

private:
    float m_currentKnockbackCooldown = 0.0f;     // Time spent in knockback so far
    const float m_KNOCKBACK_DURATION = 0.75f;     // Knockback duration in seconds
    const float m_KNOCKBACK_DISTANCE_PX = 200.0f;// Knockback distance in pixels
};
