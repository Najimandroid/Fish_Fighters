#pragma once

#include "BaseState.h"

/*
 * ParalyzedState
 * --------------
 * A forced idle while the entity has the PARALYZED status.
 *
 * Responsibilities:
 *  - Entered when the StatusManager applies PARALYZED (any attack in progress is cancelled).
 *  - Doesn't move, doesn't look for targets, doesn't attack.
 *  - Checks every frame whether the PARALYZED status is still active: the status is the
 *    only source of truth for how long the paralysis lasts.
 *  - Transitions:
 *      - Health <= 0.0f or knockback threshold -> KnockbackState
 *        (KnockbackState comes back here if the entity is still paralyzed)
 *      - PARALYZED status expired -> IdleState
 *
 * Uses the idle animation frame.
 */
class ParalyzedState : public BaseState
{
public:
    explicit ParalyzedState(StateMachine* machine);

    void enter() override;                       // Clears targets (cancels the current attack)
    void perform(float deltaTime) override;      // Waits for the status to end
    void exit() override;

    std::string get_state_id() const override;   // Always returns "PARALYZED"
};
